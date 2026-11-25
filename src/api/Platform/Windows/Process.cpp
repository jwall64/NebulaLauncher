/*
 * Copyright 2026 jwall64
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/**
 * \file Platform/Windows/Process.cpp
 * \author jwall64
 * \brief Partial implementation of Platform/Process.hpp for Windows.
 */


#include <Platform/PlatformDefines.hpp>

#ifdef NBL_PLATFORM_WINDOWS

#include <Core/Result.hpp>
#include <Core/Array.hpp>
#include <Core/String.hpp>
#include <Platform/Arena.hpp>
#include <Platform/Process.hpp>
#include <Utility/StringBuilder.hpp>
#include <Utility/Sort.hpp>

#include "WindowsUtil.hpp"

#include <Platform/Logging.hpp>


namespace {


nbl::Result _launchSubprocess(nbl::StringView commandLine, const nbl::ProcessCreateOptions& options, HANDLE* outHandle, HANDLE* outStdoutHandle, HANDLE* outJobHandle) {

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());

    HANDLE readPipe;
    HANDLE writePipe;
    if(options.outputMode == nbl::ProcessOutputMode::PIPE) {
        if(!CreatePipe(&readPipe, &writePipe, 0, 0)) {
            return nbl::Result::PROCESS_ERROR;
        }
    }
    else if(options.outputMode == nbl::ProcessOutputMode::REDIRECT) {
        if(options.outputRedirectFile.empty()) {
            return nbl::Result::INVALID_ARGUMENT;
        }
    }

    usize wsize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, commandLine.data(), commandLine.length(), nullptr, 0);
    if(wsize == 0) return nbl::Result::INVALID_ARGUMENT;

    wchar_t* wdata;
    NBL_CHECK(frame.allocate(&wdata, wsize + 1));

    if(!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, commandLine.data(), commandLine.length(), wdata, wsize)) {
        return nbl::Result::INVALID_ARGUMENT;
    }
    wdata[wsize] = 0;

    void* env = nullptr;
    if(options.environment.length() > 0) {

        nbl::Span<nbl::Span<wchar_t>> vars(nullptr, options.environment.length());
        nbl::Span<nbl::Span<wchar_t>> values(nullptr, vars.length());
        nbl::Span<usize> indices(nullptr, vars.length());

        frame.allocate(&vars);
        frame.allocate(&values);
        frame.allocate(&indices);

        for(usize i = 0; i < vars.length(); i++) {
            nbl::StringView var = options.environment[i].variable;
            nbl::StringView value = options.environment[i].value;

            indices[i] = i;

            usize sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, var.data(), var.length(), nullptr, 0);
            if(sz == 0) return nbl::Result::INVALID_ARGUMENT;

            vars[i] = nbl::Span<wchar_t>(nullptr, sz + 1);

            frame.allocate(&values[i]);
            if(!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, var.data(), var.length(), values[i].data(), sz)) {
                return nbl::Result::INVALID_ARGUMENT;
            }


            sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), value.length(), nullptr, 0);
            if(sz == 0) return nbl::Result::INVALID_ARGUMENT;

            values[i] = nbl::Span<wchar_t>(nullptr, sz + 1);

            frame.allocate(&values[i]);
            if(!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), value.length(), values[i].data(), sz)) {
                return nbl::Result::INVALID_ARGUMENT;
            }
        }

        auto cmp = [&](const usize& i1, const usize& i2) -> i32 {
            i32 out = CompareStringOrdinal(
                vars[i1].data(), static_cast<int>(vars[i1].length()),
                vars[i2].data(), static_cast<int>(vars[i2].length()),
                TRUE) == CSTR_LESS_THAN;
            return out;
            };
        nbl::FunctionRef<i32(const usize&, const usize&)> compareFunc = cmp;

        quicksort(indices, compareFunc);


        usize current = 0;

        LPWCH parentEnv = GetEnvironmentStringsW();
        if(parentEnv != nullptr) {
            env = frame.end();

            while(true) {

                if(wcslen(parentEnv) == 0) break;

                wchar_t* sep = nullptr;
                if(parentEnv[0] == L'=') {
                    sep = wcschr(parentEnv + 1, L'=');
                }
                else {
                    sep = wcschr(parentEnv, L'=');
                }

                usize len = 0;
                if(sep == nullptr) {
                    len = wcslen(parentEnv);
                }
                else {
                    len = sep - (parentEnv);
                }
                while(true) {
                    nbl::Span<wchar_t> currentVar = vars[indices[current]];
                    if(CompareStringOrdinal(parentEnv, len, currentVar.data(), currentVar.length(), TRUE) != CSTR_LESS_THAN) {
                        break;
                    }

                    nbl::Span<wchar_t> currentValue = values[indices[current]];

                    wchar_t* appended = nullptr;
                    frame.allocate(&appended, currentVar.length() + 1 + currentValue.length() + 1);
                    memcpy(appended, currentVar.data(), currentVar.length() * sizeof(wchar_t));
                    appended[currentVar.length() * sizeof(wchar_t)] = L'=';
                    memcpy(appended + (currentVar.length() + 1), currentValue.data(), currentValue.length() * sizeof(wchar_t));
                    appended[(currentVar.length() + 1 + currentValue.length()) * sizeof(wchar_t)] = L'\0';

                    current++;
                }

                wchar_t* end = wcschr(parentEnv, L'\0');
                usize diff = 0;
                if(end == nullptr) {
                    diff = wcslen(parentEnv);
                }
                else {
                    diff = end - parentEnv;
                }

                wchar_t* appended = nullptr;
                frame.allocate(&appended, diff + 1);

                memcpy(appended, parentEnv, diff);
                appended[diff] = L'\0';

                parentEnv = end + 1;
            }
            FreeEnvironmentStringsW(parentEnv);
        }
    }


    wchar_t* workingDir = nullptr;

    if(!options.workingDir.empty()) {
        usize sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, options.workingDir.data(), options.workingDir.length(), nullptr, 0);
        if(sz == 0) return nbl::Result::INVALID_ARGUMENT;

        frame.allocate(&workingDir, sz + 1);
        if(!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, options.workingDir.data(), options.workingDir.length(), workingDir, sz)) {
            return nbl::Result::INVALID_ARGUMENT;
        }
    }

    if(options.killOnClose) {
        *outJobHandle = CreateJobObjectW(NULL, NULL);
        if(*outJobHandle == INVALID_HANDLE_VALUE) {
            NBL_ERROR("Failed to create job object!");
            return nbl::Result::OS_ERROR;
        }

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = {};
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

        if(!SetInformationJobObject(*outJobHandle, JobObjectExtendedLimitInformation, &info, sizeof(info))) {
            CloseHandle(*outJobHandle);
            return nbl::Result::PROCESS_ERROR;
        }
    }

    SECURITY_ATTRIBUTES security = {
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle = true,
    };



    HANDLE nulHandle = CreateFileW(
        L"NUL",
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        &security,
        OPEN_EXISTING,
        0,
        nullptr
    );
    if(nulHandle == INVALID_HANDLE_VALUE) {
        return nbl::Result::IO_ERROR;
    }


    STARTUPINFOW startupInfo = {
        .cb = sizeof(STARTUPINFOW),
        .dwFlags = STARTF_USESTDHANDLES | 0,
        .hStdInput = nulHandle
    };


    HANDLE pipeReader = nullptr;
    HANDLE pipeWriter = nullptr;

    switch(options.outputMode) {
    case nbl::ProcessOutputMode::INHERIT: {
        startupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        startupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        break;
    }
    case nbl::ProcessOutputMode::PIPE: {
        if(!CreatePipe(&pipeReader, &pipeWriter, &security, 0)) {
            CloseHandle(nulHandle);
            return nbl::Result::IO_ERROR;
        }

        if(!SetHandleInformation(pipeReader, HANDLE_FLAG_INHERIT, 0)) {
            CloseHandle(nulHandle);
            CloseHandle(pipeReader);
            CloseHandle(pipeWriter);
            return nbl::Result::IO_ERROR;
        }


        startupInfo.hStdOutput = pipeWriter;
        startupInfo.hStdError = nulHandle;

        *outStdoutHandle = pipeReader;
        break;
    }
    case nbl::ProcessOutputMode::NULLIFY: {
        startupInfo.hStdOutput = nulHandle;
        startupInfo.hStdError = nulHandle;
        break;
    }
    case nbl::ProcessOutputMode::REDIRECT: {

        usize sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, options.workingDir.data(), options.workingDir.length(), nullptr, 0);
        if(sz == 0) return nbl::Result::INVALID_ARGUMENT;

        wchar_t* fileName;

        frame.allocate(&fileName, sz + 1);
        if(!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, options.workingDir.data(), options.workingDir.length(), workingDir, sz)) {
            return nbl::Result::INVALID_ARGUMENT;
        }

        pipeWriter = CreateFileW(
            fileName,
            GENERIC_WRITE,
            FILE_SHARE_WRITE,
            &security,
            CREATE_ALWAYS,
            0,
            nullptr
        );
        if(pipeWriter == INVALID_HANDLE_VALUE) {
            return nbl::Result::IO_ERROR;
        }

        startupInfo.hStdError = nulHandle;
        break;
    }
    }

    PROCESS_INFORMATION processInfo{};
    if(!CreateProcessW(
        nullptr,
        wdata,
        nullptr,
        nullptr,
        true,
        env == nullptr ? 0 : CREATE_UNICODE_ENVIRONMENT,
        env,
        workingDir,
        &startupInfo,
        &processInfo
    )) {

        CloseHandle(nulHandle);
        if(pipeWriter) CloseHandle(pipeWriter);

        return nbl::Result::PROCESS_ERROR;
    }

    CloseHandle(nulHandle);
    if(pipeWriter) CloseHandle(pipeWriter);

    CloseHandle(processInfo.hThread);

    if(options.killOnClose) {
        if(!AssignProcessToJobObject(*outJobHandle, processInfo.hProcess)) {
            CloseHandle(processInfo.hProcess);
            CloseHandle(*outJobHandle);
            return nbl::Result::PROCESS_ERROR;
        }
    }

    *outHandle = processInfo.hProcess;
    return nbl::Result::SUCCESS;
}


nbl::Result _escapeArgument(nbl::StringView arg, nbl::StringBuilder* builder) {

    bool needsEscape = false;
    for(usize i = 0; i < arg.length(); i++) {
        char c = arg[i];
        if(c == ' ' || c == '"') {
            needsEscape = true;
            break;
        }
    }

    if(needsEscape) {
        builder->append('"');
        for(char c : arg) {
            if(c == '"') {
                builder->append('\\').append('"');
            }
            else {
                builder->append(c);
            }
        }
        builder->append('"');
    }
    else {
        return builder->appendChecked(arg);
    }

}


}


nbl::StringView nbl::getHostName() {


    static usize _length = 0;
    static char _hostName[256]{};

    if(_length == 0) {

        DWORD size = 0;
        GetComputerNameExW(ComputerNamePhysicalDnsHostname, nullptr, &size);
        NBL_ASSERT(GetLastError() == ERROR_MORE_DATA);

        nbl::AllocationFrame frame(nbl::Arena::threadLocal());
        wchar_t* data = nullptr;
        NBL_ASSERT(frame.allocate(&data, size).success());
        NBL_ASSERT(GetComputerNameExW(ComputerNamePhysicalDnsHostname, data, &size));

        int utfSize = WideCharToMultiByte(CP_UTF8, 0, data, size, nullptr, 0, nullptr, nullptr);
        NBL_ASSERT(utfSize > 0);

        char* utfData = nullptr;
        NBL_ASSERT(frame.allocate(&utfData, utfSize).success());
        NBL_ASSERT(WideCharToMultiByte(CP_UTF8, 0, data, size, utfData, utfSize, nullptr, nullptr));

        _length = min(255, utfSize);
        memcpy(_hostName, utfData, _length);
        _hostName[_length] = '\0';
    }

    return nbl::StringView(_hostName, _length, true);
}

nbl::StringView nbl::getEnv(nbl::StringView name) {

    static union {
        char smallBuffer[256]{};
        char* dynamic;
    } _data;
    static usize _length = 0;

    if(_length > 255) {
        delete[] _data.dynamic;
    }

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());

    if(!name.nullTerminated()) {
        nbl::StringBuilder(&frame).append(name).buildNullTerminated(&name);
    }

    _length = GetEnvironmentVariable(name.data(), _data.smallBuffer, 256);
    if(_length == 0) {
        return {};
    }

    char* buf = _data.smallBuffer;

    if(_length > 255) {
        _data.dynamic = new char[_length + 1];
        buf = _data.dynamic;
    }

    GetEnvironmentVariable(name.data(), buf, _length + 1);

    return StringView(buf, _length, true);
}


void nbl::setEnv(StringView name, StringView value, bool overwrite) {


    nbl::AllocationFrame frame(nbl::Arena::threadLocal());
    if(!name.nullTerminated()) {
        StringBuilder(&frame).append(name).buildNullTerminated(&name);
    } 

    if(!overwrite && getEnv(name).length() > 0) return;

    if(!value.nullTerminated()) {
        StringBuilder(&frame).append(value).buildNullTerminated(&value);
    }

    NBL_ASSERT(SetEnvironmentVariable(name.data(), value.data()));
}


nbl::StringView nbl::getBinaryPath() {

    static char* _path{};
    static usize _length = 0;

    if(_length == 0) {

        nbl::AllocationFrame frame(nbl::Arena::threadLocal());
        wchar_t* path;
        usize allocationLength = MAX_PATH;
        frame.allocate(&path, MAX_PATH);
        
        while(true) {
            _length = GetModuleFileNameW(nullptr, path, allocationLength);
            NBL_ASSERT(_length > 0);
            if(_length < allocationLength) {
                break;
            }

            usize prevLength = allocationLength;
            allocationLength *= 2;
            frame.push(allocationLength - prevLength, nullptr);
        }

        int utfSize = WideCharToMultiByte(CP_UTF8, 0, path, _length, nullptr, 0, nullptr, nullptr);
        NBL_ASSERT(utfSize > 0);

        char* utfData;
        NBL_ASSERT(frame.allocate(&utfData, utfSize).success());
        NBL_ASSERT(WideCharToMultiByte(CP_UTF8, 0, path, _length, utfData, utfSize, nullptr, nullptr));

        _path = new char[utfSize + 1];
        memcpy(_path, utfData, utfSize);
        _path[utfSize] = '\0';

        _length = utfSize;
    }
    return StringView(_path, _length, true);
}


nbl::Result nbl::sleep(u32 millis) {
    Sleep(millis);
    return Result::SUCCESS;
}


nbl::ProcessHandle::~ProcessHandle() {
    if(_bKillOnClose) {
        kill();
    }
    if(_pHandle) {
        CloseHandle(_pHandle);
        _pHandle = nullptr;
    }
    if(captureOutput()) {
        HANDLE output = _pOutputHandle;
        CloseHandle(output);
        _pOutputHandle = nullptr;
    }
}


nbl::Result nbl::ProcessHandle::wait(i32* exitCode) const {

    DWORD rc = WaitForSingleObject(_pHandle, INFINITE);
    if(rc != WAIT_OBJECT_0) {
        return nbl::Result::PROCESS_ERROR;
    }

    if(exitCode) {
        DWORD ec = 0;
        GetExitCodeProcess(_pHandle, &ec);

        *exitCode = static_cast<i32>(ec);
    }
    return nbl::Result::SUCCESS;
}

bool nbl::ProcessHandle::isRunning() const {
    if(_pHandle == nullptr) return false;

    DWORD ec;
    if(GetExitCodeProcess(_pHandle, &ec)) {
        return ec == STILL_ACTIVE;
    }

    return false;
}

void nbl::ProcessHandle::terminate() {
    if(_pHandle == nullptr) return;
    TerminateProcess(_pHandle, 1);
    WaitForSingleObject(_pHandle, INFINITE);
    CloseHandle(_pHandle);
    _pHandle = nullptr;
}

void nbl::ProcessHandle::kill() {
    terminate();
}

i32 nbl::ProcessHandle::getPid() const {
    return static_cast<i32>(GetProcessId(_pHandle));
}

nbl::Reader nbl::ProcessHandle::stdoutReader() const {
    if(!captureOutput()) {
        return nbl::NullStream();
    }
    return nbl::Reader{
        .impl = _pOutputHandle,
        .pfnRead = nbl::win::readHandle
    };
}

nbl::Result nbl::ProcessHandle::run(StringView commandLine, ProcessHandle* outHandle, const ProcessCreateOptions& options) {
    nbl::AllocationFrame frame(nbl::Arena::threadLocal());
    nbl::StringView cmd{};
    nbl::StringBuilder(&frame).append("cmd.exe /d /s /c \"").append(commandLine).append('"').build(&cmd);

    return _launchSubprocess(cmd, options, &outHandle->_pHandle, &outHandle->_pOutputHandle, &outHandle->_pJobHandle);
}

nbl::Result nbl::ProcessHandle::spawn(StringView path, Span<StringView> args, ProcessHandle* outHandle, const ProcessCreateOptions& options) {

    nbl::AllocationFrame frame(nbl::Arena::threadLocal());
    nbl::StringView commandLine{};

    nbl::StringBuilder builder(&frame);
    _escapeArgument(path, &builder);

    for(StringView arg : args) {
        if(arg.empty()) {
            builder.append(" \"\"");
        }
        else {
            builder.append(' ');
            _escapeArgument(arg, &builder);
        }
    }

    NBL_CHECK(builder.build(&commandLine));

    return _launchSubprocess(commandLine, options, &outHandle->_pHandle, &outHandle->_pOutputHandle, &outHandle->_pJobHandle);
}

nbl::Result nbl::ProcessHandle::fromPid(i32 pid, nbl::ProcessHandle* out) {
    out->_pHandle = OpenProcess(READ_CONTROL, false, pid);
    return nbl::Result::SUCCESS;
}

#endif
