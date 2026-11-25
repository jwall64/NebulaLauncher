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
 * \file Platform/Process.hpp
 * \author jwall64
 * \brief Interfaces for managing subprocesses and threads
 */


#pragma once

#include <Core/Types.hpp>
#include <Core/String.hpp>
#include <Core/Result.hpp>
#include <Core/Span.hpp>


#include <Platform/PlatformDefines.hpp>


namespace nbl {

StringView getHostName();

/**
 * \brief Retrieves the value of an environment variable by name.
 * \param name The name of the environment variable to retrieve.
 * \return A StringView containing the value of the environment variable, or an empty StringView if the variable is not set.
 */
StringView getEnv(StringView name);

/**
 * \brief Sets the value of an environment variable.
 * \param name The name of the environment variable to set.
 * \param value The value to should to
 * \param overwrite Whether the value should be set if it already has a value
 */
void setEnv(StringView name, StringView value, bool overwrite=true);


/**
 * \brief Retrieves the path to the binary executable being run.
 * \return A StringView containing the path of the binary executable being run.
 */
StringView getBinaryPath();


Result sleep(u32 millis);


/**
 * \brief A class representing a thread of execution in the program. This class provides methods for creating, joining, and detaching threads, as well as checking if a thread is currently running.
 */ 
class Thread {
public:
   
    ~Thread();

    /**
     * \brief A function pointer type representing the entry point of a thread. The function should take a single void* argument for user data and return an i32 value as the thread's exit code.
     */
    typedef i32 (*EntryPointFn)(void* userData);

    /**
     * \brief Determines if the thread is currently running.
     * \return true if the thread is running, false otherwise.
     */
    bool isRunning() const;

    /**
     * \brief Waits for the thread to finish execution and retrieves its exit code.
     * \param outReturnValue Pointer to a variable that will receive the thread's exit code. Can be nullptr if the exit code is not needed.
     * \return A Result indicating success or failure of the join operation.
     */
    Result join(i32* outReturnValue);

    /**Proce
     * \brief Detaches the thread, allowing it to run independently without the need for joining. After detaching, the thread will continue to run in the background and its resources will be automatically cleaned up when it finishes execution.
     * \return A Result indicating success or failure of the detach operation.
     */
    Result detach();

    /**
     * \brief Creates a new thread of execution with the specified entry point function and user data. The thread will start running immediately after creation.
     * \param function The entry point function that the thread will execute.
     * \param userData A pointer to user-defined data that will be passed to the entry point function when the thread starts executing. This can be used to provide context or parameters to the thread's execution.
     * \param outThread Pointer to a Thread variable that will receive the handle to the newly created thread. This can be used to manage the thread's lifecycle, such as joining or detaching it later.
     * \return A Result indicating success or failure of the thread creation operation.
     */
    static Result create(EntryPointFn function, void* userData, Thread* outThread);

private:

    void* _pHandle;
};


enum class ProcessOutputMode {
    INHERIT,
    PIPE,
    NULLIFY,
    REDIRECT
};

struct EnvVar {
    StringView variable;
    StringView value;
};

struct ProcessCreateOptions {
    ProcessOutputMode outputMode;
    StringView outputRedirectFile;
    StringView workingDir;
    bool killOnClose;
    Span<EnvVar> environment;
};

/**
 * \brief A class representing a handle to a subprocess. This class provides methods for waiting for the subprocess to finish execution and checking if it is still running, as well as a static method for spawning new subprocesses.
 */
class ProcessHandle {
public:

    ProcessHandle() = default;
    ProcessHandle(const ProcessHandle& other) = delete;
    ProcessHandle(ProcessHandle&& other) noexcept :
        _pHandle(other._pHandle),
        _pOutputHandle(other._pOutputHandle)
    { 
        other._pHandle = nullptr;
        other._pOutputHandle = nullptr;
    }

    ~ProcessHandle();

    ProcessHandle& operator=(const ProcessHandle& other) = delete;
    ProcessHandle& operator=(ProcessHandle&& other) noexcept {

        if(&other == this) return *this;

        _pHandle = other._pHandle;
        _pOutputHandle = other._pOutputHandle;

        other._pHandle = nullptr;
        other._pOutputHandle = nullptr;

        return *this;
    }

    /**
     * \brief Waits for the subprocess to finish execution and retrieves its exit code.
     * \param exitCode Pointer to a variable that will receive the subprocess's exit code. Can be nullptr if the exit code is not needed.
     * \return A Result indicating success or failure of the wait operation.
     */
    Result wait(i32* exitCode) const;

    /**
     * \brief Checks if the subprocess is still running.
     * \return true if the subprocess is running, false otherwise.
     */
    bool isRunning() const;

    void terminate();
    void kill();

    /**
     * \brief Gets the process id (PID) for the subprocess
     * \return The subprocess's PID
     */
    i32 getPid() const;

    /**
     * \brief Determines if the subprocess was created with the CAPTURE_OUTPUT flag, which allows capturing its standard output. If this method returns true, you can use the stdoutReader() method to read the subprocess's output.
     * \return true if the subprocess was created with CAPTURE_OUTPUT, false otherwise.
     */
    bool captureOutput() const {
        return _pOutputHandle != nullptr;
    }

    /**
     * \brief If the subprocess was created with the CAPTURE_OUTPUT flag, this method returns a Reader object that can be used to read the standard output of the subprocess. If the subprocess was not created with CAPTURE_OUTPUT, this method will return a null reader.
     * \return A Reader object for reading the subprocess's standard output.
     */
    Reader stdoutReader() const;

    /**
     * \brief Spawns a new subprocess with the specified command line. The subprocess will start running immediately after creation.
     * \param commandLine The full command line to execute, including the executable path and any arguments. The command line should be formatted according to the conventions of the underlying operating system (e.g., using spaces to separate arguments and quoting paths with spaces).
     * \param outHandle Pointer to a ProcessHandle variable that will receive the handle to the newly spawned subprocess.
     * \return A Result indicating success or failure of the subprocess creation operation.
     */
    static Result run(StringView commandLine, ProcessHandle* outHandle, const ProcessCreateOptions& options = {});


    /**
     * \brief Spawns a new subprocess with the specified executable path and arguments. The subprocess will start running immediately after creation.
     * \param path The file system path to the executable that the subprocess will run.
     * \param args A span of StringViews representing the command-line arguments to be passed to the subprocess.
     * \param outHandle Pointer to a ProcessHandle variable that will receive the handle to the newly spawned subprocess. This can be used to manage the subprocess's lifecycle, such as waiting for it to finish or checking if it is still running.
     * \return A Result indicating success or failure of the subprocess creation operation.
     */
    static Result spawn(StringView path, Span<StringView> args, ProcessHandle* outHandle, const ProcessCreateOptions& options = {});


    static Result fromPid(i32 pid, ProcessHandle* outHandle);

private:

    void* _pHandle;
    void* _pOutputHandle;
    bool _bKillOnClose;
#if defined(NBL_PLATFORM_WINDOWS)
    void* _pJobHandle;
#endif
};

}

