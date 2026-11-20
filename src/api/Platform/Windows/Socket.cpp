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
 * \file Platform/Windows/Socket.cpp
 * \author jwall64
 * \brief Implementation of Platform/Socket.hpp for Windows.
 */


#include <Platform/PlatformDefines.hpp>

#if defined(NBL_PLATFORM_WINDOWS)

#include <Platform/Socket.hpp>

#include "WindowsUtil.hpp"


namespace {

SOCKET _getSocket(void* handle) {
    return static_cast<SOCKET>(reinterpret_cast<intptr_t>(handle));
}

nbl::Result _writeSocket(void* impl, const void* data, usize length, usize* bytesWritten) {
    int rc = send(_getSocket(impl), static_cast<const char*>(data), length, 0);
    if(rc == SOCKET_ERROR) return nbl::Result::IO_ERROR;
    if(bytesWritten) *bytesWritten = rc;
    return nbl::Result::SUCCESS;
}
nbl::Result _readSocket(void* impl, void* data, usize length, usize* bytesRead) {
    int rc = recv(_getSocket(impl), static_cast<char*>(data), length, 0);
    if(rc == SOCKET_ERROR) return nbl::Result::IO_ERROR;
    if(bytesRead) *bytesRead = rc;
    return nbl::Result::SUCCESS;

}

}

nbl::Writer nbl::Socket::writer() {
    return {
        .impl = _mHandle,
        .pfnWrite = _writeSocket
    };
}

nbl::Reader nbl::Socket::reader() {
    return {
        .impl = _mHandle,
        .pfnRead = _readSocket
    };
}

void nbl::Socket::close() {
    if(_mHandle) {
        closesocket(_getSocket(_mHandle));
        _mHandle = nullptr;
    }
}

nbl::Result nbl::Socket::connectIPv4(u32 address, u16 port, SocketType type, Socket* outSocket) {

    int sockType;
    if(type == SocketType::TCP) {
        sockType = SOCK_STREAM;
    } else if(type == SocketType::UDP) {
        sockType = SOCK_DGRAM;
    } else {
        return nbl::Result::INVALID_ARGUMENT;
    }

    SOCKET sock = socket(AF_INET, sockType, 0);

    if(sock < 0) {
        return nbl::Result::IO_ERROR;
    }

    sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = {
            .S_un = {
                .S_addr = address
            }
        }
    };

    if(connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr_in)) < 0) {
        closesocket(sock);
        return nbl::Result::IO_ERROR;
    }

    outSocket->_mHandle = reinterpret_cast<void*>(static_cast<intptr_t>(sock));
    return nbl::Result::SUCCESS;
}


nbl::Result nbl::ServerSocket::accept(Socket* client) {

    SOCKET serverSock = static_cast<SOCKET>(reinterpret_cast<intptr_t>(_mHandle));
    SOCKET clientSock;
    if((clientSock = ::accept(serverSock, nullptr, nullptr)) < 0) {
        return nbl::Result::IO_ERROR;
    }

    client->close();
    client->_mHandle = reinterpret_cast<void*>(static_cast<intptr_t>(clientSock));
    return nbl::Result::SUCCESS;
}

bool nbl::ServerSocket::isClosed() {
    return _mHandle == nullptr;
}

void nbl::ServerSocket::close() {
    if(_mHandle) {
        SOCKET serverSock = static_cast<int>(reinterpret_cast<intptr_t>(_mHandle));
        _mHandle = nullptr;
        closesocket(serverSock);
    }
}

nbl::Result nbl::ServerSocket::listen(u16 port, SocketType type, ServerSocket *outSocket) {

    int sockType;
    if(type == SocketType::TCP) {
        sockType = SOCK_STREAM;
    } else if(type == SocketType::UDP) {
        sockType = SOCK_DGRAM;
    } else {
        return nbl::Result::INVALID_ARGUMENT;
    }

    SOCKET sock = socket(AF_INET, sockType, 0);

    if(sock < 0) {
        return nbl::Result::IO_ERROR;
    }

    sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = {
            .S_un = {
                .S_addr = INADDR_ANY
            }
        }
    };

    if(bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        closesocket(sock);
        return nbl::Result::IO_ERROR;
    }

    if(::listen(sock, 10) < 0) {
        closesocket(sock);
        return nbl::Result::IO_ERROR;
    }

    outSocket->_mHandle = reinterpret_cast<void*>(static_cast<intptr_t>(sock));
    return nbl::Result::SUCCESS;


}

#endif
