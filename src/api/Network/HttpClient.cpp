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
 * \file Network/HttpClient.cpp
 * \author jwall64
 * \brief Implementation of Network/HttpClient.hpp
 */

#define NOGDI

#include <Network/HttpClient.hpp>
#include <Utility/StringBuilder.hpp>
#include <Core/Result.hpp>
#include <Core/String.hpp>
#include <Platform/File.hpp>
#include <Platform/Logging.hpp>

#include <curl/multi.h>
#include <curl/curl.h>

namespace {

static bool _sInitialized = false;
constexpr const char* _USER_AGENT = "Nebula/0.1.0 curl/8.20.0";

usize _writeCallback(char* ptr, usize size, usize nmemb, void* userData) {

    nbl::Writer writer = *static_cast<nbl::Writer*>(userData);
    if(writer.writeAll(ptr, size * nmemb).error()) {
        return 0;
    }
    return size * nmemb;
}

}

nbl::HttpClient::~HttpClient() {

    if(_pHandle) {
        _mRequestMutex.lock();
        curl_multi_wakeup(_pHandle);

        CURLM* handle = static_cast<CURLM*>(_pHandle);

        for(_ActiveRequest& req : _mRequests) {

            CURL* easy = static_cast<CURL*>(req.handle);

            req.finished(nbl::Result::INTERRUPTED, 0);

            curl_multi_remove_handle(handle, easy);
            curl_easy_cleanup(easy);

            _mRequests.pop(&req);
        }

        _mRequestMutex.unlock();
        _mReaderThread.join(nullptr);

        curl_multi_cleanup(handle);
        _pHandle = nullptr;
    }
}


nbl::Result nbl::HttpClient::downloadFile(StringView url, StringView file, HttpRequestOptions options) {

    if(_mMaxOpenFiles == 0) return nbl::Result::INVALID_STATE;

    nbl::log(nbl::LogLevel::DEBUG, "GET ", url, " > ", file);
    auto frameLock = _mRequestMutex.frameLock();

    _ActiveRequest* req = nullptr;
    NBL_CHECK(_mRequests.push(&req));
    

    FileOpenFlags flags = FileOpenFlag::WRITE | FileOpenFlag::CREATE;
    if(_mFlags & HttpClientFlag::CREATE_FILE_PARENTS) flags = flags | FileOpenFlag::CREATE_PARENTS;


    nbl::FileChannel channel{};
    nbl::Result res = nbl::Result::TOO_MANY_OPEN_FILES;
    if(_mOpenFiles == _mMaxOpenFiles || (res = nbl::FileChannel::open(file, &channel, flags)) == Result::TOO_MANY_OPEN_FILES) {
        if(!(_mFlags & HttpClientFlag::REFERENCE_FILE_PATHS)) {
            if(_mPaths.begin() == nullptr) {
                NBL_CHECK(nbl::Arena::create(_mRequests.capacity() * 4096U, &_mPaths));
            }
            NBL_CHECK(StringBuilder(&_mPaths).append(file).buildNullTerminated(&file));
        }
        
        _mOpenFiles++;
        req->createParents = _mFlags & HttpClientFlag::CREATE_FILE_PARENTS;
        req->file = file;
        req->markExecutable = options.markExecutable;

    } else if(res.error()) {
        return res;
    } else {
        if(options.markExecutable) {
            nbl::setExecutable(file);
        }
        req->file = std::move(channel);
        req->writer = req->file.left()->writer();
    }

    req->download = true;

    return _setupConnection(url, req, {}, options);
}

nbl::Result nbl::HttpClient::makeRequest(StringView url, Writer writer, HttpRequestOptions options) {

    nbl::log(nbl::LogLevel::DEBUG, "GET ", url);
    auto frameLock = _mRequestMutex.frameLock();

    _ActiveRequest* req = nullptr;
    NBL_CHECK(_mRequests.push(&req));

    req->writer = writer;

    return _setupConnection(url, req, {}, options);
}

nbl::Result nbl::HttpClient::makePostRequest(StringView url, StringView body, Writer output, HttpRequestOptions options) {


    nbl::log(nbl::LogLevel::DEBUG, "POST ", url, ":\n", body);
    auto frameLock = _mRequestMutex.frameLock();

    _ActiveRequest* req = nullptr;
    NBL_CHECK(_mRequests.push(&req));

    req->writer = output;

    return _setupConnection(url, req, body, options);
}


nbl::Result nbl::HttpClient::awaitRequest(StringView url, Writer writer, HttpRequestOptions options) {

    nbl::Latch latch{};
    NBL_CHECK(nbl::Latch::create(1, &latch));

    struct _RequestData {
        HttpRequestOptions* ogOptions;
        Latch* latch;
        Result outResult;
    } _requestData = { &options, &latch, Result::SUCCESS };

    HttpRequestOptions opts = options;
    opts.doneCallback = [](void* userData, Result res, i32 code) {
            _RequestData* rd = static_cast<_RequestData*>(userData);
            if(rd->ogOptions->doneCallback) {
                rd->ogOptions->doneCallback(rd->ogOptions->userData, res, code);
            }
            rd->outResult = res;
            rd->latch->decrement();
        };
    opts.userData = &_requestData;

    NBL_CHECK(makeRequest(url, writer, opts));

    latch.wait();
    return _requestData.outResult;
}


nbl::Result nbl::HttpClient::_setupConnection(StringView url, _ActiveRequest* req, Optional<StringView> postData, const HttpRequestOptions& options) {
    
    CURLM* multiHandle = static_cast<CURLM*>(_pHandle);
    CURL* easyHandle = curl_easy_init();
    if(!easyHandle) {
        return nbl::Result::IO_ERROR;
    }

    AllocationFrame frame(Arena::threadLocal());

    if(!url.nullTerminated()) {
        NBL_CHECK(StringBuilder::nullTerminate(url, &url, &frame));
    }

    curl_easy_setopt(easyHandle, CURLOPT_WRITEFUNCTION, _writeCallback);
    curl_easy_setopt(easyHandle, CURLOPT_USERAGENT, _USER_AGENT);
    curl_easy_setopt(easyHandle, CURLOPT_WRITEDATA, &req->writer);
    curl_easy_setopt(easyHandle, CURLOPT_PRIVATE, req);
    curl_easy_setopt(easyHandle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easyHandle, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(easyHandle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
    curl_easy_setopt(easyHandle, CURLOPT_UPKEEP_INTERVAL_MS, 5000L);
    curl_easy_setopt(easyHandle, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(easyHandle, CURLOPT_ACCEPT_ENCODING, "");

    if(options.ifModifiedSince > 0) {
        curl_easy_setopt(easyHandle, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
        curl_easy_setopt(easyHandle, CURLOPT_TIMEVALUE, options.ifModifiedSince);
    }

    if(postData.hasValue()) {

        StringView postStr = *postData;
        if(!postStr.nullTerminated()) {
            NBL_CHECK(nbl::StringBuilder::nullTerminate(postStr, &postStr, &frame));
        }
        curl_easy_setopt(easyHandle, CURLOPT_POSTFIELDS, postStr.data());
    }

    if(options.headers.length() > 0) {
        curl_slist* list = nullptr;
    
        for(const HttpHeader& header : options.headers) { 
            if(!header.validateName())
                return nbl::Result::INVALID_ARGUMENT;

            StringView headerStr;
            NBL_CHECK(nbl::StringBuilder(&frame)
                .append(header.name)
                .append(": ")
                .append(header.value)
                .buildNullTerminated(&headerStr));

            list = curl_slist_append(list, headerStr.data());
        }

        curl_easy_setopt(easyHandle, CURLOPT_HTTPHEADER, list);
        req->headers = list;
    }
    curl_easy_setopt(easyHandle, CURLOPT_URL, url.data());

    req->doneCallback = options.doneCallback;
    req->userData = options.userData;
    req->handle = easyHandle;
    req->queued = true;
    req->retryCount = 0;
    _mQueuedRequests++;

    _mAllDone.increment();

    NBL_CHECK(_startThread());

    return nbl::Result::SUCCESS;
}

void nbl::HttpClient::awaitAll(u32* numFailures) {
    _mAllDone.wait();
    if(_mPaths.begin() != nullptr) {
        _mPaths.popTo(0);
    }
    if(numFailures) *numFailures = _mFailures;
    _mFailures = 0;
}



nbl::Result nbl::HttpClient::_startThread() {

    if(_mReaderThread.isRunning()) {
        CURLM* multiHandle = static_cast<CURLM*>(_pHandle);
        curl_multi_wakeup(multiHandle);
        return nbl::Result::SUCCESS;
    }

    _mReaderThread.detach();

    return nbl::Thread::create([](void* userData) {

        HttpClient* client = static_cast<HttpClient*>(userData);
        CURLM* handle = static_cast<CURLM*>(client->_pHandle);

        int runningRequests = 0;
        do {

            if(curl_multi_perform(handle, &runningRequests) != CURLM_OK) {
                return 1;
            }

            if(curl_multi_poll(handle, nullptr, 0, 0, nullptr) != CURLM_OK) {
                return 2;
            }

            client->_mRequestMutex.lock();
            if(!client->_bQueuePaused) {
                for(_ActiveRequest& req : client->_mRequests) {
                    if(req.queued) {
                        if(req.download && !req.fileOpened()) {

                            FileOpenFlags flags = FileOpenFlag::WRITE | FileOpenFlag::CREATE;
                            if(req.createParents) flags |= FileOpenFlag::CREATE_PARENTS;

                            FileChannel channel{};
                            StringView path = *req.file.right();
                            nbl::Result res = nbl::FileChannel::open(path, &channel, flags);
                            if(runningRequests > 0 && res == Result::TOO_MANY_OPEN_FILES) {
                                client->_bQueuePaused = true;
                                break;
                            }
                            if(res.error()) {
                                req.finished(nbl::Result::IO_ERROR, 0);
                                client->_mFailures++;
                            } else {
                                if(req.markExecutable) {
                                    nbl::setExecutable(path);
                                }
                                req.file = std::move(channel);
                                req.writer = req.file.left()->writer();
                                curl_multi_add_handle(handle, static_cast<CURL*>(req.handle));
                                runningRequests++;
                                client->_mQueuedRequests--;
                                req.queued = false;
                            }
                        } else {
                            curl_multi_add_handle(handle, static_cast<CURL*>(req.handle));
                            runningRequests++;
                            client->_mQueuedRequests--;
                            req.queued = false;
                        }
                    }
                }
            }

            client->_mRequestMutex.unlock();

            CURLMsg* msg;
            int msgsLeft;
            while((msg = curl_multi_info_read(handle, &msgsLeft)) != nullptr) {
                if(msg->msg == CURLMSG_DONE) {

                    CURL* easy = msg->easy_handle;
                    _ActiveRequest* req = nullptr;
                    curl_easy_getinfo(easy, CURLINFO_PRIVATE, &req);

                    if(msg->data.result == CURLE_OPERATION_TIMEDOUT) { // Timeout, OK to retry
                        if(req->retryCount++ < client->_mMaxRetries) {
                            curl_multi_remove_handle(handle, easy);
                            curl_multi_add_handle(handle, easy);
                            continue;
                        }
                        req->finished(nbl::Result::TIMEOUT, 0);
                        client->_mFailures++;

                    } else if(msg->data.result == CURLE_OK) {

                        long responseCode;
                        curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &responseCode);

                        if(responseCode >= 500 && responseCode < 600) {
                            if(req->retryCount++ < client->_mMaxRetries) { // Server error, OK to retry
                                curl_multi_remove_handle(handle, easy);
                                curl_multi_add_handle(handle, easy);
                                continue;
                            }
                            req->finished(nbl::Result::IO_ERROR, responseCode);
                            client->_mFailures++;
                        } else if((responseCode >= 200 && responseCode < 300) || responseCode == 304) {
                            req->finished(nbl::Result::SUCCESS, responseCode);
                        } else {
                            nbl::log(nbl::LogLevel::DEBUG, "Response Code: ", responseCode);
                            req->finished(nbl::Result::IO_ERROR, responseCode);
                            client->_mFailures++;
                        }

                    } else {
                        nbl::log(nbl::LogLevel::ERROR, "HTTP connection failed! ", curl_easy_strerror(msg->data.result));
                        req->finished(nbl::Result::IO_ERROR, 0);
                        client->_mFailures++;
                    }

                    if(req->headers) {
                        curl_slist_free_all(static_cast<curl_slist*>(req->headers));
                    }
                    if(req->download) {
                        client->_mOpenFiles--;
                    }

                    curl_multi_remove_handle(handle, easy);
                    curl_easy_cleanup(easy);
                    client->_mRequests.pop(req);
                    client->_bQueuePaused = false;

                    client->_mAllDone.decrement();
                }
            }

        } while(runningRequests > 0 || client->_mQueuedRequests > 0);

        return 0;
    }, this, &_mReaderThread);

}

nbl::Result nbl::HttpClient::create(u32 maxRequests, u32 maxConnections, u32 maxRetries, u32 maxOpenFiles, HttpClient* out, HttpClientFlags flags) {

    NBL_CHECK(nbl::Pool<_ActiveRequest>::create(maxRequests, &out->_mRequests));
    NBL_CHECK(nbl::Latch::create(0, &out->_mAllDone));

    if(!_sInitialized) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        _sInitialized = true;
    }

    CURLM* multiHandle = curl_multi_init();
    if(multiHandle == nullptr) return nbl::Result::IO_ERROR;

    curl_multi_setopt(multiHandle, CURLMOPT_MAX_TOTAL_CONNECTIONS, maxConnections);
    curl_multi_setopt(multiHandle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
    curl_multi_setopt(multiHandle, CURLMOPT_MAX_CONCURRENT_STREAMS, nbl::max(100U, maxRequests / maxConnections));

    NBL_CHECK(nbl::Mutex::create(&out->_mRequestMutex));

    out->_mMaxRetries = maxRetries;
    out->_pHandle = multiHandle;
    out->_mFlags = flags;
    out->_mMaxOpenFiles = maxOpenFiles;
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::write(const UrlEncode& encode, Writer writer) {
    for(char c : encode.string) {
        if(
        (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') ||
        (c == '-' || c == '_' || c == '.' || c == '~')) {
            NBL_CHECK(writer.writeChar(c));
        } else {
            NBL_CHECK(writer.writeChar('%'));
            NBL_CHECK(writeHex(static_cast<u8>(c), writer));
        }
    }
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::write(const UrlDecode& decode, Writer writer) {
    for(usize i = 0 ; i < decode.string.length() ; i++) {

        char c = decode.string[i];
        if(c == '%') {
            if(i + 2 >= decode.string.length()) {
                NBL_DEBUG("Found '%' too close to the end of the string!");
                return nbl::Result::FORMAT_ERROR;
            }
            char hex[2] = {
                decode.string[i + 1],
                decode.string[i + 2]
            };

            u8 chara;
            for(usize i = 0 ; i < 2 ; i++) {
                char hexChar = hex[i];
                if(!(hexChar >= '0' && hexChar <= '9')
                && !(hexChar >= 'a' && hexChar <= 'f')
                && !(hexChar >= 'A' && hexChar <= 'F')) {
                    NBL_DEBUG("Found invalid hex char! ", hexChar);
                    return nbl::Result::FORMAT_ERROR;
                }

                u8 hexValue = (hexChar >= '0' && hexChar <= '9') 
                    ? hexChar - '0'
                    : hexChar >= 'a' && hexChar <= 'f' 
                        ? hexChar - 'a' + 10
                        : hexChar - 'A' + 10;

                if(i == 0) {
                    chara |= (hexValue << 4);
                } else {
                    chara |= (hexValue & 0xF);
                }
            }

            NBL_CHECK(writer.writeChar(chara));
            i += 2;
            
        } else {
            NBL_CHECK(writer.writeChar(c));
        }
    }
    return nbl::Result::SUCCESS;
}
