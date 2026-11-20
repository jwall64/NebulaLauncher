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
 * \file Codec/Qoi.cpp
 * \author jwall64
 * \brief Implementation of Codec/Qoi.hpp
 */

#include <Codec/Qoi.hpp>

#include <Core/Result.hpp>
#include <Platform/Allocator.hpp>
#include <Platform/PlatformDefines.hpp>

#ifdef NBL_PLATFORM_WINDOWS
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include <utility>
#include <cstring>

namespace {

struct _QoiHeader {
    alignas(4) char magic[4];
    u32 widthBe;
    u32 heightBe;
    u8 channels;
    u8 colorSpace;
};

constexpr const char* _MAGIC = "qoif";
constexpr u8 _END[8] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01
};

#define QOI_OP_RGB   0b11111110
#define QOI_OP_RGBA  0b11111111
#define QOI_OP_INDEX 0b00000000
#define QOI_OP_DIFF  0b01000000
#define QOI_OP_LUMA  0b10000000
#define QOI_OP_RUN   0b11000000

usize _index(nbl::RGBA color) {
    return (color.r * 3 + color.g * 5 + color.b * 7 + color.a * 11) % 64;
}

}

nbl::Result nbl::QoiImageMeta::readMeta(nbl::Reader reader, QoiImageMeta* out) {
    u8 buffer[14]{};
    NBL_CHECK(reader.readAll(buffer, 14));
    return decodeMeta(ConstSpan<u8>(buffer, 14), out);
}

nbl::Result nbl::QoiImageMeta::readImageData(Reader reader, QoiImageMeta meta, LinearAllocator* storage, u8 channels) {

    if(channels == 0) {
        channels = meta.channels;
    }
    else if(channels < 3 || channels > 4) {
        return nbl::Result::INVALID_ARGUMENT;
    }

    void* begin = storage->end();

    Span<u8> outputBuffer(nullptr, meta.width * meta.height * channels);
    NBL_CHECK(storage->allocate(&outputBuffer));

    u8* qoiBegin = static_cast<u8*>(storage->end());
    Result res;
    if((res = storage->readAll(reader)).error()) {
        storage->popToPtr(begin);
        return res;
    }

    ConstSpan<u8> inputBuffer(qoiBegin, static_cast<u8*>(storage->end()) - qoiBegin);
    if((res = decodeImageData(inputBuffer, meta, outputBuffer, channels)).error()) {
        storage->popToPtr(begin);
        return res;
    }

    storage->popToPtr(qoiBegin);
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::QoiImageMeta::decodeMeta(ConstSpan<u8> buffer, QoiImageMeta* out) {

    if(buffer.length() < 14) return Result::INVALID_ARGUMENT;

    const _QoiHeader* header = reinterpret_cast<const _QoiHeader*>(buffer.data());
    if(::memcmp(header->magic, _MAGIC, 4) != 0) return nbl::Result::PARSE_ERROR;

    if(header->channels != 3 && header->channels != 4) {
        return nbl::Result::PARSE_ERROR;
    }
    if(header->colorSpace > 1) {
        return nbl::Result::PARSE_ERROR;
    }

    out->width = ::ntohl(header->widthBe);
    out->height = ::ntohl(header->heightBe);
    out->channels = header->channels;
    out->colorSpace = header->colorSpace == 1 ? ColorSpace::LINEAR : ColorSpace::SRGB;

    return nbl::Result::SUCCESS;
}

nbl::Result nbl::QoiImageMeta::decodeImageData(ConstSpan<u8> buffer, QoiImageMeta meta, Span<u8> outputBuffer, u8 channels) {

    if(channels == 0) {
        channels = meta.channels;
    }
    else if(channels < 3 || channels > 4) {
        return nbl::Result::INVALID_ARGUMENT;
    }
    if(outputBuffer.length() < meta.width * meta.height * channels) {
        return nbl::Result::INVALID_ARGUMENT;
    }
    if(buffer.length() < 8) {
        return nbl::Result::INVALID_ARGUMENT;
    }

    RGBA prev = { .r = 0, .g = 0, .b = 0, .a = 255 };
    RGBA seen[64] = {};

    usize outputIndex = 0;
    auto _pushPix = [&]() {
        memcpy(outputBuffer.data() + outputIndex, &prev, channels);
        outputIndex += channels;
    };

    usize inputIndex = 0;
    usize pixel = 0;
    const usize numPixels = meta.width * meta.height;

    while(pixel < numPixels) {

        u8 b1 = buffer[inputIndex++];

        if(b1 == QOI_OP_RGB) {
            prev.r = buffer[inputIndex++];
            prev.g = buffer[inputIndex++];
            prev.b = buffer[inputIndex++];
            _pushPix();
            seen[_index(prev)] = prev;
            pixel++;
        } else if(b1 == QOI_OP_RGBA) {
            prev.r = buffer[inputIndex++];
            prev.g = buffer[inputIndex++];
            prev.b = buffer[inputIndex++];
            prev.a = buffer[inputIndex++];
            _pushPix();
            seen[_index(prev)] = prev;
            pixel++;
        } else {
            u8 tag = b1 & 0b11000000;
            switch(tag) {
                case QOI_OP_INDEX:
                    prev = seen[b1 & 0b00111111];
                    _pushPix();
                    pixel++;
                    break;
                case QOI_OP_DIFF:
                    prev.r = prev.r + ((b1 & 0b00110000) >> 4) - 2;
                    prev.g = prev.g + ((b1 & 0b00001100) >> 2) - 2;
                    prev.b = prev.b +  (b1 & 0b00000011) - 2;
                    _pushPix();
                    seen[_index(prev)] = prev;
                    pixel++;
                    break;
                case QOI_OP_LUMA: {
                    u8 b2 = buffer[inputIndex++];
                    
                    i8 dg =  (b1 & 0b00111111) - 32;
                    i8 dr = ((b2 & 0b11110000) >> 4) - 8;
                    i8 db =  (b2 & 0b00001111) - 8;

                    prev.r += dr + dg;
                    prev.g += dg;
                    prev.b += db + dg;
                    
                    _pushPix();
                    seen[_index(prev)] = prev;
                    pixel++;
                    break;
                }
                case QOI_OP_RUN: {
                    u8 run = (b1 & 0b00111111) + 1;
                    for(u8 i = 0 ; i < run ; i++) {
                        _pushPix();
                    }
                    pixel += run;
                    break;
                }
                default:
                    std::unreachable();
            }
        }
    }

    return nbl::Result::SUCCESS;
}



nbl::Result nbl::QoiImageMeta::writeMeta(Writer writer, QoiImageMeta meta) {

    _QoiHeader header {
        .widthBe = ::htonl(meta.width),
        .heightBe = ::htonl(meta.height),
        .channels = meta.channels,
        .colorSpace = static_cast<u8>(meta.colorSpace == ColorSpace::LINEAR ? 1 : 0)
    };
    memcpy(header.magic, _MAGIC, 4);
    return writer.writeAll(&header, 14);
}

nbl::Result nbl::QoiImageMeta::writeImageData(Writer writer, QoiImageMeta meta, ConstSpan<u8> imageData) {

    usize bufferLength = meta.width * meta.height * meta.channels;
    if(imageData.length() < bufferLength) return nbl::Result::INVALID_ARGUMENT;

    const u8* pixels = imageData.data();
    RGBA prev = { .r = 0, .g = 0, .b = 0, .a = 255 };
    RGBA seen[64]{};

    usize run = 0;
    usize byteIndex = 0;

    for( ; byteIndex < bufferLength ; byteIndex += meta.channels) {

        RGBA current = {
            .r = pixels[byteIndex],
            .g = pixels[byteIndex + 1],
            .b = pixels[byteIndex + 2],
            .a = 255
        };
        if(meta.channels == 4) {
            current.a = pixels[byteIndex + 3];
        }

        if(current == prev) {
            run++;
            if(run == 62) {
                NBL_CHECK(writer.writeChar(QOI_OP_RUN | (run - 1)));
                run = 0;
            }
            continue;
        }

        if(run > 0) {
            NBL_CHECK(writer.writeChar(QOI_OP_RUN | (run - 1)));
            run = 0;
        }

        usize indexPos = _index(current);
        if(seen[indexPos] == current) {
            NBL_CHECK(writer.writeChar(QOI_OP_INDEX | indexPos));
            prev = current;
            continue;
        }

        seen[indexPos] = current;
        if(current.a != prev.a) {
            NBL_CHECK(writer.writeChar(QOI_OP_RGBA));
            NBL_CHECK(writer.writeAll(&current, 4));
            prev = current;
            continue;
        } 

        i8 dr = current.r - prev.r;
        i8 dg = current.g - prev.g;
        i8 db = current.b - prev.b;

        if(dr > -3 && dr < 2 
        && dg > -3 && dg < 2
        && db > -3 && db < 2) {
            NBL_CHECK(writer.writeChar(QOI_OP_DIFF | (dr + 2) << 4 | (dg + 2) << 2 | (db + 2)));
            prev = current;
            continue;
        }

        i8 dgr = dr - dg;
        i8 dgb = db - dg;

        if(dgr > -9 && dgr < 8
        && dg > -33 && dg < 32
        && dgb > -9 && dgb < 8) {
            u8 bytes[2] = {
                static_cast<u8>(QOI_OP_LUMA | (dg + 32)),
                static_cast<u8>((dgr + 8) << 4 | (dgb + 8))
            };
            NBL_CHECK(writer.writeAll(bytes, 2));
            prev = current;
            continue;
        }

        NBL_CHECK(writer.writeChar(QOI_OP_RGB));
        NBL_CHECK(writer.writeAll(&current, 3));
        prev = current;
    }

    if(run > 0) {
        NBL_CHECK(writer.writeChar(QOI_OP_RUN | (run - 1)));
    }

    NBL_CHECK(writer.writeAll(_END, 8));
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::QoiImageMeta::encodeMeta(Span<u8> buffer, QoiImageMeta meta) {

    if(buffer.length() < 14) return nbl::Result::INVALID_ARGUMENT;

    _QoiHeader* header = reinterpret_cast<_QoiHeader*>(buffer.data());
    *header = {
        .widthBe = ::htonl(meta.width),
        .heightBe = ::htonl(meta.height),
        .channels = meta.channels,
        .colorSpace = static_cast<u8>(meta.colorSpace == ColorSpace::LINEAR ? 1 : 0)
    };
    memcpy(header->magic, _MAGIC, 4);
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::QoiImageMeta::encodeImageData(nbl::LinearAllocator* alloc, QoiImageMeta meta, ConstSpan<u8> imageData) {

    usize bufferLength = meta.width * meta.height * meta.channels;
    if(imageData.length() < bufferLength) return nbl::Result::INVALID_ARGUMENT;

    const u8* pixels = imageData.data();
    RGBA prev = { .r = 0, .g = 0, .b = 0, .a = 255 };
    RGBA seen[64]{};

    usize run = 0;
    usize byteIndex = 0;

    u8* batch = nullptr;
    NBL_CHECK(alloc->push(4096, reinterpret_cast<void**>(&batch)));
    usize batchIndex = 0;

    for( ; byteIndex < bufferLength ; byteIndex += meta.channels) {
        RGBA current = {
            .r = pixels[byteIndex],
            .g = pixels[byteIndex + 1],
            .b = pixels[byteIndex + 2],
            .a = 255
        };
        if(meta.channels == 4) {
            current.a = pixels[byteIndex + 3];
        }

        if(4096 - batchIndex < 14) {
            batch += batchIndex;
            NBL_CHECK(alloc->push(batchIndex, nullptr))
            batchIndex = 0;
        }

        if(current == prev) {
            run++;
            if(run == 62) {
                batch[batchIndex++] = QOI_OP_RUN | (run - 1);
                run = 0;
            }
            continue;
        }

        if(run > 0) {
            batch[batchIndex++] = QOI_OP_RUN | (run - 1);
            run = 0;
        }

        usize indexPos = _index(current);
        if(seen[indexPos] == current) {
            batch[batchIndex++] = QOI_OP_INDEX | indexPos;
            prev = current;
            continue;
        }

        seen[indexPos] = current;
        if(current.a != prev.a) {
            batch[batchIndex++] = QOI_OP_RGBA;
            batch[batchIndex++] = current.r;
            batch[batchIndex++] = current.g;
            batch[batchIndex++] = current.b;
            batch[batchIndex++] = current.a;
            prev = current;
            continue;
        } 

        i8 dr = current.r - prev.r;
        i8 dg = current.g - prev.g;
        i8 db = current.b - prev.b;

        if(dr > -3 && dr < 2 
        && dg > -3 && dg < 2
        && db > -3 && db < 2) {
            batch[batchIndex++] = QOI_OP_DIFF | (dr + 2) << 4 | (dg + 2) << 2 | (db + 2);
            prev = current;
            continue;
        }

        i8 dgr = dr - dg;
        i8 dgb = db - dg;

        if(dgr > -9 && dgr < 8
        && dg > -33 && dg < 32
        && dgb > -9 && dgb < 8) {
            batch[batchIndex++] = static_cast<u8>(QOI_OP_LUMA | (dg + 32));
            batch[batchIndex++] = static_cast<u8>((dgr + 8) << 4 | (dgb + 8));
            prev = current;
            continue;
        }

        batch[batchIndex++] = QOI_OP_RGB;
        batch[batchIndex++] = current.r;
        batch[batchIndex++] = current.g;
        batch[batchIndex++] = current.b;
        prev = current;
    }

    if(run > 0) {
        batch[batchIndex++] = QOI_OP_RUN | (run - 1);
    }

    memcpy(batch + batchIndex, _END, 8);
    batchIndex += 8;

    return alloc->pop(4096 - batchIndex);
}
