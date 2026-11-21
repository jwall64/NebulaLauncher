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
 * \file Core/Stream.hpp
 * \author jwall64
 * \brief Contains Reader and Writer interfaces for dealing with byte streams.
 */

#pragma once

#include <Core/Types.hpp>
#include <Core/Result.hpp>
#include <Core/Span.hpp>

namespace nbl {

struct StringView;

/**
 * \brief A simple stream writer interface that can write data to an underlying stream.
 */
struct Writer {

    /**
     * \brief Writes data to the stream.
     * \param data Pointer to the data to be written.
     * \param length The number of bytes to write.
     * \param bytesWritten Pointer to a variable that will receive the number of bytes actually written. Can be nullptr.
     * \return A Result indicating success or failure of the write operation.
     */
    Result write(const void* data, usize length, usize* bytesWritten) {
        return pfnWrite(impl, data, length, bytesWritten);
    }

    /**
     * \brief Writes all data to the stream, ensuring that the entire buffer is written before returning.
     * \param data Pointer to the data to be written.
     * \param length The number of bytes to write.
     * \return A Result indicating success or failure of the write operation.
     */
    Result writeAll(const void* data, usize length) {
        usize totalBytesWritten = 0;

        while (totalBytesWritten < length) {
            usize bytesWritten;
            Result result = write(static_cast<const char*>(data) + totalBytesWritten, length - totalBytesWritten, &bytesWritten);
            if (result.error()) {
                return result;
            }
            totalBytesWritten += bytesWritten;
        }
        return Result::SUCCESS;
    }

    /**
     * \brief Writes a string to the stream.
     * \param str The string to be written.
     * \return A Result indicating success or failure of the write operation.
     */
    Result writeString(StringView str);

    /**
     * \brief Writes a single character to the stream.
     * \param c The character to be written.
     * \return A Result indicating success or failure of the write operation.
     */
    Result writeChar(char c) {
        return writeAll(&c, 1);
    }

    /**
     * \brief An opaque handle to the underlying stream implementation.
     */
    void* impl;

    /**
     * \brief Function pointer to the implementation of the write operation for the underlying stream.
     */
    Result (*pfnWrite)(void* impl, const void* data, usize length, usize* bytesWritten);
};

/**
 * \brief A simple stream reader interface that can read data from an underlying stream.
 */
struct Reader {

    /**
     * \brief Reads data from the stream.
     * \param data Pointer to a buffer that will receive the read data.
     * \param length The maximum number of bytes to read.
     * \param bytesRead Pointer to a variable that will receive the number of bytes actually read. Can be nullptr.
     * \return A Result indicating success or failure of the read operation.
     */
    Result read(void* data, usize length, usize* bytesRead) {
        return pfnRead(impl, data, length, bytesRead);
    }

    /**
     * \brief Reads data from the stream, ensuring that the entire buffer is filled before returning.
     * \param data Pointer to a buffer that will receive the read data.
     * \param length The number of bytes to read.
     * \return A Result indicating success or failure of the read operation.
     */
    Result readAll(void* data, usize length) {
        usize totalBytesRead = 0;
        while (totalBytesRead < length) {
            usize bytesRead;
            Result result = read(static_cast<char*>(data) + totalBytesRead, length - totalBytesRead, &bytesRead);
            if (result.error()) {
                return result;
            } else if(bytesRead == 0 && result == nbl::Result::END_OF_STREAM) {
                return nbl::Result::UNEXPECTED_END_OF_STREAM;
            }
            totalBytesRead += bytesRead;
        }
        return Result::SUCCESS;
    }

    /**
     * \brief An opaque handle to the underlying stream implementation.
     */
    void* impl;

    /**
     * \brief Function pointer to the implementation of the read operation for the underlying stream.
     */
    Result (*pfnRead)(void* impl, void* data, usize length, usize* bytesRead);
};



/**
 * \brief Writes an integer value to a writer in Base-10 ASCII.
 * \param value The integer value to write.
 * \param writer The writer to write the value to.
 * \return A Result indicating success or failure of the write operation.
 */
Result writeInt(i64 value, Writer writer);

/**
 * \brief Writes an unsigned integer value to a writer in Base-10 ASCII.
 * \param value The unsigned integer value to write.
 * \param writer The writer to write the value to.
 * \return A Result indicating success or failure of the write operation.
 */
Result writeUint(u64 value, Writer writer);

/**
 * \brief Writes a floating-point value to a writer in Base-10 ASCII format with a specified precision.
 * \param value The floating-point value to write.
 * \param writer The writer to write the value to.
 * \param precision The number of decimal places to include in the output. Default is 15.
 * \return A Result indicating success or failure of the write operation.
 */
Result writeFloat(f64 value, Writer writer, u8 precision = 15);

/**
 * \brief Writes an unsigned integer value to a writer in Base-16 ASCII.
 * \param value The unsigned integer value to write.
 * \param writer The writer to write the value to.
 * \param minDigits The minimum number of digits to write. The output will be left-padded with zeros if necessary.
 * \param lower Whether to use lowercase hex digits for place values greater than 9
 * \return A Result indicating success or failure of the write operation.
 */
Result writeHex(u64 value, Writer writer, u8 minDigits = 0, bool lower = false);


/**
* \brief Converts a value of any type to a string representation and writes it to a writer. This is a template function that should be specialized for different types.
* \tparam T The type of the value to convert to a string.
* \param value The value to convert to a string.
* \param writer The writer to write the string representation of the value to.
* \return A Result indicating success or failure of the write operation. By default, this function returns an error indicating that the conversion is unimplemented for the given type, and should be specialized for specific types to provide actual implementations.
*/
template<typename T>
Result write(T value, Writer writer) {
    return nbl::Result::UNIMPLEMENTED;
}

template<typename T>
inline Result write(T* ptr, Writer writer) {
    NBL_CHECK(writer.writeAll("0x", 2));
    return writeHex(reinterpret_cast<u64>(ptr), writer, sizeof(T*) * 2);
}

inline Result write(bool value, Writer writer) { return write(value ? "true" : "false", writer); }
inline Result write(char value, Writer writer) { return writer.writeAll(&value, 1); }

inline Result write(i64 value, Writer writer) { return writeInt(value, writer); }
inline Result write(u64 value, Writer writer) { return writeUint(value, writer); }
inline Result write(f64 value, Writer writer) { return writeFloat(value, writer); }

inline Result write(i32 value, Writer writer) { return writeInt(static_cast<i64>(value), writer); }
inline Result write(u32 value, Writer writer) { return writeUint(static_cast<u64>(value), writer); }
inline Result write(f32 value, Writer writer) { return writeFloat(static_cast<f64>(value), writer); }

inline Result write(i16 value, Writer writer) { return writeInt(static_cast<i64>(value), writer); }
inline Result write(u16 value, Writer writer) { return writeUint(static_cast<u64>(value), writer); }

inline Result write(i8 value, Writer writer) { return writeInt(static_cast<i64>(value), writer); }
inline Result write(u8 value, Writer writer) { return writeUint(static_cast<u64>(value), writer); }

inline Result write(nbl::Result value, Writer writer) {
    return write(static_cast<i32>(value), writer);
}

/**
 * \brief A struct that holds a floating-point value and a precision for formatting the value as a string. This can be used to specify the precision when converting a floating-point value to a string representation.
 */
struct FloatLimiter {
    
    /** 
     * \brief Constructor that initializes the FloatLimiter with a floating-point value and a precision.
     * \param value The floating-point value to be formatted as a string.
     * \param precision The number of decimal places to include in the string representation of the value.
     */
    FloatLimiter(f64 value, u8 precision) : 
        value(value),
        precision(precision) 
    { }
    
    f64 value;
    u8 precision;
};

// Specialization of the write function for the FloatLimiter struct, which writes the floating-point value to the writer with the specified precision.
inline Result write(FloatLimiter limiter, Writer writer) {
    return writeFloat(limiter.value, writer, limiter.precision);
}


struct HexEncoder {
    ConstSpan<u8> data;
    bool upper;
};
// Specialization of the write function for the HexEncoder struct, which writes the buffer to the stream in hexadecimal format.
Result write(HexEncoder encoder, Writer writer);


/**
 * \brief A simple stream implementation for reading and writing from a known buffer.
 */
class BufferStream {
public:

    BufferStream(void* buffer, usize length) :
        _mBuffer{static_cast<u8*>(buffer), length},
        _mReaderIndex(0),
        _mWriterIndex(0)
    { }
 
    template<typename T>
    BufferStream(Span<T> data) :
        _mBuffer(data. template reinterpret<u8>()),
        _mReaderIndex(0),
        _mWriterIndex(0)
    { }

    operator Writer() {
        return Writer{ this, _write };
    }

    operator Reader() {
        return Reader{ this, _read };
    }

    usize writerIndex() const { return _mWriterIndex; }
    usize readerIndex() const { return _mReaderIndex; }

    void resetWriterIndex() { _mWriterIndex = 0; }
    void resetReaderIndex() { _mReaderIndex = 0; }

private:
    nbl::Span<u8> _mBuffer;
    usize _mReaderIndex;
    usize _mWriterIndex;
    
    static Result _write(void* impl, const void* data, usize length, usize* bytesWritten);
    
    static Result _read(void* impl, void* data, usize length, usize* bytesRead);

};

/**
 * \brief A dummy stream that disards bytes written to it and is always at the end of file when reading
 */
class NullStream {
public:

    operator Writer() {
        return Writer{ nullptr, _write };
    }

    operator Reader() {
        return Reader{ nullptr, _read };
    }

private:

    static Result _write(void* impl, const void* data, usize length, usize* bytesWritten) {
        if(bytesWritten) {
            *bytesWritten = length;
        }
        return Result::SUCCESS;
    }

    static Result _read(void* impl, void* data, usize length, usize* bytesRead) {
        if(bytesRead) {
            *bytesRead = 0;
        }
        return nbl::Result::END_OF_STREAM;
    }
};


/**
 * \brief A writer which first writes to a buffer in memory, then flushes data to the underlying writer in batches
 */
class BufferedWriter {
public:

    BufferedWriter(Writer sink, void* buffer, usize bufLen) :
        _mSink(sink),
        _mPos(0),
        _mBufLen(bufLen),
        _pBuffer(static_cast<u8*>(buffer))
    { }

    ~BufferedWriter() {
        flush();
    }

    operator Writer() { 
        return Writer{this, _write};
    }

    Result flush();

    Result write(const void* data, usize length, usize* bytesWritten);

    Result writeAll(const void* data, usize length) {
        return operator Writer().writeAll(data, length);
    }

private:
    static Result _write(void* impl, const void* data, usize length, usize* bytesWritten) {
        BufferedWriter* writer = static_cast<BufferedWriter*>(impl);
        return writer->write(data, length, bytesWritten);
    }

    Writer _mSink;
    usize _mPos;
    usize _mBufLen;
    u8* _pBuffer;
};

/**
 * \brief A writer which batch reads from some underlying reader into to a buffer in memory 
 */
class BufferedReader {
public:

    BufferedReader(Reader source, void* buffer, usize bufLen) :
        _mSource(source),
        _mPos(0),
        _mAvailable(0),
        _mBufLen(bufLen),
        _pBuffer(static_cast<u8*>(buffer))
    { }

    operator Reader() { 
        return Reader{ this, _read };
    }

    Result read(void* data, usize length, usize* bytesRead);

    inline Result readAll(void* data, usize length) {
        return operator Reader().readAll(data, length);
    }

private:
    static Result _read(void* impl, void* data, usize length, usize* bytesRead) {
        BufferedReader* reader = static_cast<BufferedReader*>(impl);
        return reader->read(data, length, bytesRead);
    }

    Reader _mSource;
    usize _mPos;
    usize _mAvailable;
    usize _mBufLen;
    u8* _pBuffer;
};

}
