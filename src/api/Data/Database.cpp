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
 * \file Database.cpp
 * \author jwall64
 * \brief Implementation of Data/Database.hpp using SQLite.
 */

#include <Core/Result.hpp>
#include <Platform/Arena.hpp>
#include <Codec/Json.hpp>
#include <Platform/Logging.hpp>
#include <Utility/StringBuilder.hpp>
#include <Data/Database.hpp>

#include <climits>
#include <sqlite3.h>


namespace {

int _databaseCallback(void* userData, int argc, char** argv, char** colNames) {

    i64 lastInsertRowId = sqlite3_last_insert_rowid(reinterpret_cast<sqlite3*>(userData));

    nbl::DatabaseCallback* callback = static_cast<nbl::DatabaseCallback*>(userData);
    nbl::Result res = (*callback)(argc, const_cast<const char**>(argv), const_cast<const char**>(colNames), lastInsertRowId);
    if(!res.success()) {
        return -1;
    }
    return 0;
}

void _strDistance(sqlite3_context* ctx, int argc, sqlite3_value** argv) {

    nbl::StringView str1 = {};
    nbl::StringView str2 = {};

    if(argc > 0) {
        const unsigned char* data1 = sqlite3_value_text(argv[0]);
        if(data1) {
            str1 = reinterpret_cast<const char*>(data1);
        }
    }
    if(argc > 1) {
        const unsigned char* data2 = sqlite3_value_text(argv[1]);
        if(data2) {
            str2 = reinterpret_cast<const char*>(data2);
        }
    }
    sqlite3_result_int64(ctx, str1.distance(str2));
}

}



nbl::PreparedStatement::~PreparedStatement() {
    if(_pHandle) {
        sqlite3_finalize(reinterpret_cast<sqlite3_stmt*>(_pHandle));
        _pHandle = nullptr;
    }
}

nbl::Result nbl::PreparedStatement::reset() {
    int rc = sqlite3_reset(reinterpret_cast<sqlite3_stmt*>(_pHandle));
    if(rc != SQLITE_OK) {
        return nbl::Result::IO_ERROR;
    }
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::PreparedStatement::bindInt(i32 index, i64 value) {
    int rc = sqlite3_bind_int64(reinterpret_cast<sqlite3_stmt*>(_pHandle), index, value);
    if(rc != SQLITE_OK) {
        nbl::log(nbl::LogLevel::ERROR, "Failed to bind int parameter at index ", index, ": ", sqlite3_errmsg(reinterpret_cast<sqlite3*>(_pHandle)));
        return nbl::Result::IO_ERROR;
    }
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::PreparedStatement::bindUint(i32 index, u64 value) {
    int rc = sqlite3_bind_int64(reinterpret_cast<sqlite3_stmt*>(_pHandle), index, value);
    if(rc != SQLITE_OK) {
        nbl::log(nbl::LogLevel::ERROR, "Failed to bind uint parameter at index ", index, ": ", sqlite3_errmsg(reinterpret_cast<sqlite3*>(_pHandle)));
        return nbl::Result::IO_ERROR;
    }
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::PreparedStatement::bindFloat(i32 index, f64 value) {
    int rc = sqlite3_bind_double(reinterpret_cast<sqlite3_stmt*>(_pHandle), index, value);
    if(rc != SQLITE_OK) {
        nbl::log(nbl::LogLevel::DEBUG, "Failed to bind float parameter at index ", index, ": ", sqlite3_errmsg(reinterpret_cast<sqlite3*>(_pHandle)));
        return nbl::Result::IO_ERROR;
    }
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::PreparedStatement::bindString(i32 index, nbl::StringView value, bool transient) {

    NBL_ASSERT(value.length() <= static_cast<usize>(INT_MAX));

    int rc = sqlite3_bind_text(
        reinterpret_cast<sqlite3_stmt*>(_pHandle), 
        index, 
        value.data(), 
        static_cast<int>(value.length()), 
        transient ? SQLITE_TRANSIENT : SQLITE_STATIC);

    if(rc != SQLITE_OK) {
        nbl::log(nbl::LogLevel::DEBUG, "Failed to bind string parameter at index ", index, ": ", sqlite3_errmsg(reinterpret_cast<sqlite3*>(_pHandle)));
        return nbl::Result::IO_ERROR;
    }
    return nbl::Result::SUCCESS;
}
nbl::Result nbl::PreparedStatement::bindBlob(i32 index, ConstSpan<u8> value, bool transient) {

    NBL_ASSERT(value.length() <= static_cast<usize>(INT_MAX));

    int rc = sqlite3_bind_blob64(
        reinterpret_cast<sqlite3_stmt*>(_pHandle), 
        index, 
        value.data(), 
        value.length(),
        transient ? SQLITE_TRANSIENT : SQLITE_STATIC);

    if(rc != SQLITE_OK) {
        nbl::log(nbl::LogLevel::DEBUG, "Failed to bind string parameter at index ", index, ": ", sqlite3_errmsg(reinterpret_cast<sqlite3*>(_pHandle)));
        return nbl::Result::IO_ERROR;
    }
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::PreparedStatement::bindNull(i32 index) {
    int rc = sqlite3_bind_null(reinterpret_cast<sqlite3_stmt*>(_pHandle), index);
    if(rc != SQLITE_OK) {
        nbl::log(nbl::LogLevel::DEBUG, "Failed to bind null parameter at index ", index, ": ", sqlite3_errmsg(reinterpret_cast<sqlite3*>(_pHandle)));
        return nbl::Result::IO_ERROR;
    }
    return nbl::Result::SUCCESS;
}


nbl::DatabaseConnection::~DatabaseConnection() {
    if(_pHandle) {
        sqlite3_close_v2(reinterpret_cast<sqlite3*>(_pHandle));
        _pHandle = nullptr;
    }
}


nbl::Result nbl::DatabaseConnection::prepare(nbl::StringView query, PreparedStatement* outStatement) {
    
    if(outStatement->_pHandle) {
        sqlite3_finalize(reinterpret_cast<sqlite3_stmt*>(outStatement->_pHandle));
    }
    NBL_ASSERT(query.length() <= static_cast<usize>(INT_MAX));
    outStatement->_pConnectionHandle = _pHandle;

    int rc = sqlite3_prepare_v2(
        reinterpret_cast<sqlite3*>(_pHandle), 
        query.data(), 
        static_cast<int>(query.length()), 
        reinterpret_cast<sqlite3_stmt**>(&outStatement->_pHandle), 
        nullptr);

    if(rc != SQLITE_OK) {
        nbl::log(nbl::LogLevel::ERROR, "Failed to prepare statement: ", sqlite3_errmsg(reinterpret_cast<sqlite3*>(_pHandle)));
        return nbl::Result::IO_ERROR;
    }

    return nbl::Result::SUCCESS;
}



nbl::Result nbl::DatabaseConnection::execute(nbl::StringView query) {

    AllocationFrame frame(&_mArena);
    if(!query.nullTerminated()) {
        NBL_CHECK(StringBuilder::nullTerminate(query, &query, &frame));
    }
    int rc = sqlite3_exec(
        reinterpret_cast<sqlite3*>(_pHandle), 
        query.data(),
        nullptr,
        nullptr,
        nullptr);

    if(rc != SQLITE_OK) {
        nbl::log(nbl::LogLevel::ERROR, "Database execution error: ", sqlite3_errmsg(reinterpret_cast<sqlite3*>(_pHandle)));
        return nbl::Result::IO_ERROR;
    }
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::DatabaseConnection::execute(nbl::StringView query, DatabaseCallback callback) {

    AllocationFrame frame(&_mArena);
    if(!query.nullTerminated()) {
        NBL_CHECK(StringBuilder::nullTerminate(query, &query, &frame));
    }

    int rc = sqlite3_exec(
        reinterpret_cast<sqlite3*>(_pHandle), 
        query.data(), 
        _databaseCallback, 
        &callback, 
        nullptr);

    if(rc != SQLITE_OK) {
        nbl::log(nbl::LogLevel::ERROR, "Database execution error: ", sqlite3_errmsg(reinterpret_cast<sqlite3*>(_pHandle)));
        return nbl::Result::IO_ERROR;
    }
    return nbl::Result::SUCCESS;
}


nbl::Result nbl::DatabaseConnection::execute(PreparedStatement* query, i64* lastInsertRowId) {

    if(!query->isValid(this)) {
        nbl::log(nbl::LogLevel::ERROR, "Invalid prepared statement for this connection");
        return nbl::Result::INVALID_STATE;
    }

    sqlite3_stmt* stmt = reinterpret_cast<sqlite3_stmt*>(query->_pHandle);
    
#ifndef NDEBUG

    char* stmtStr = sqlite3_expanded_sql(stmt);
    StringView stmtView = stmtStr;

    nbl::log(nbl::LogLevel::DEBUG, "Running SQL: ", stmtView);

    sqlite3_free(stmtStr);

#endif

    int rc;
    while((rc = sqlite3_step(stmt)) == SQLITE_ROW) { }
    if(rc != SQLITE_DONE) {
        nbl::log(nbl::LogLevel::ERROR, "Database execution error: (", rc, ") ", sqlite3_errmsg(reinterpret_cast<sqlite3*>(_pHandle)));
        NBL_CHECK(query->reset());
        return nbl::Result::IO_ERROR;
    }

    if(lastInsertRowId) {
        *lastInsertRowId = sqlite3_last_insert_rowid(reinterpret_cast<sqlite3*>(_pHandle));
    }

    NBL_CHECK(query->reset());
    return nbl::Result::SUCCESS;
}


nbl::Result nbl::DatabaseConnection::execute(PreparedStatement* query, TypedDatabaseCallback callback, i64* lastInsertRowId) {

    if(!query->isValid(this)) {
        nbl::log(nbl::LogLevel::ERROR, "Invalid prepared statement for this connection");
        return nbl::Result::INVALID_STATE;
    }

    sqlite3_stmt* stmt = reinterpret_cast<sqlite3_stmt*>(query->_pHandle);
    
#ifndef NDEBUG

    char* stmtStr = sqlite3_expanded_sql(stmt);
    StringView stmtView = stmtStr;

    nbl::log(nbl::LogLevel::DEBUG, "Running SQL: ", stmtView);

    sqlite3_free(stmtStr);

#endif

    int rc;
    while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {

        const char** columnNames = nullptr;
        DataValue* columnValues = nullptr;

        nbl::AllocationFrame frame(&_mArena);
        int columns = sqlite3_column_count(stmt);

        if(columns > 0) {

            NBL_CHECK(frame.allocate<const char*>(&columnNames, columns));
            NBL_CHECK(frame.allocate<DataValue>(&columnValues, columns));

            for(int i = 0; i < columns; i++) {
                columnNames[i] = sqlite3_column_name(stmt, i);

                int type = sqlite3_column_type(stmt, i);
                switch(type) {
                    case SQLITE_INTEGER:
                        columnValues[i].type = DataType::INTEGER;
                        columnValues[i].intValue = sqlite3_column_int64(stmt, i);
                        break;
                    case SQLITE_FLOAT:
                        columnValues[i].type = DataType::FLOAT;
                        columnValues[i].floatValue = sqlite3_column_double(stmt, i);
                        break;
                    case SQLITE_TEXT:
                        columnValues[i].type = DataType::TEXT;
                        columnValues[i].textValue = StringView(
                            reinterpret_cast<const char*>(sqlite3_column_text(stmt, i)),
                            static_cast<usize>(sqlite3_column_bytes(stmt, i))
                        );
                        break;
                    case SQLITE_BLOB:
                        columnValues[i].type = DataType::BLOB;
                        columnValues[i].blobValue = ConstSpan<u8>(
                            reinterpret_cast<const u8*>(sqlite3_column_blob(stmt, i)),
                            static_cast<usize>(sqlite3_column_bytes(stmt, i))
                        );
                        break;

                    case SQLITE_NULL:
                        columnValues[i].type = DataType::NULL_TYPE;
                        columnValues[i].textValue = StringView();
                        break;
                    default:
                        nbl::log(nbl::LogLevel::ERROR, "Unknown column data type: ", type);
                        NBL_CHECK(query->reset());
                        return nbl::Result::IO_ERROR;
                }
            }
        }
        
        nbl::Result res = callback(
                columns,
                columnValues, 
                columnNames, 
                sqlite3_last_insert_rowid(reinterpret_cast<sqlite3*>(_pHandle))
            );
        if(res.error()) {
            NBL_CHECK(query->reset());
            return res;
        }
    }
    if(rc != SQLITE_DONE) {
        nbl::log(nbl::LogLevel::ERROR, "Database execution error: (", rc, ") ", sqlite3_errmsg(reinterpret_cast<sqlite3*>(_pHandle)));
        NBL_CHECK(query->reset());
        return nbl::Result::IO_ERROR;
    }

    if(lastInsertRowId) {
        *lastInsertRowId = sqlite3_last_insert_rowid(reinterpret_cast<sqlite3*>(_pHandle));
    }

    NBL_CHECK(query->reset());
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::DatabaseConnection::beginTransaction() {

    if(_mTransactionDepth > 0) {

        const u32 savepoint = _mTransactionDepth + 1;

        AllocationFrame frame(Arena::threadLocal());
        StringView sql{};
        NBL_CHECK(StringBuilder(&frame).append("SAVEPOINT sp").append(savepoint).append(";").buildNullTerminated(&sql));
        NBL_CHECK(execute(sql));

        _mTransactionDepth = savepoint;
        return nbl::Result::SUCCESS;
    }

    static nbl::PreparedStatement stmt;
    if(!stmt.isValid(this)) {
        NBL_CHECK(prepare("BEGIN TRANSACTION;", &stmt));
    }
    NBL_CHECK(execute(&stmt));
    _mTransactionDepth = 1;
    return nbl::Result::SUCCESS;
}
nbl::Result nbl::DatabaseConnection::commitTransaction() {

    if(_mTransactionDepth == 0) return nbl::Result::INVALID_STATE;
    if(--_mTransactionDepth > 0) return nbl::Result::SUCCESS;

    static nbl::PreparedStatement stmt;
    if(!stmt.isValid(this)) {
        NBL_CHECK(prepare("COMMIT;", &stmt));
    }

    Result res = execute(&stmt);
    if(res.error()) {
        _mTransactionDepth = 1;
        return res;
    }
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::DatabaseConnection::rollbackTransaction() {

    if(_mTransactionDepth == 0) return nbl::Result::INVALID_STATE;

    const u32 previousDepth = _mTransactionDepth;
    --_mTransactionDepth;

    if(_mTransactionDepth > 0) {
        AllocationFrame frame(Arena::threadLocal());
        StringView sql{};
        NBL_CHECK(StringBuilder(&frame).append("ROLLBACK TO SAVEPOINT sp").append(_mTransactionDepth + 1).append(";").buildNullTerminated(&sql));

        Result res = execute(sql);
        if(res.error()) {
            _mTransactionDepth = previousDepth;
            return res;
        }
        return nbl::Result::SUCCESS;
    }

    static nbl::PreparedStatement stmt;
    if(!stmt.isValid(this)) {
        NBL_CHECK(prepare("ROLLBACK;", &stmt));
    }

    Result res = execute(&stmt);
    if(res.error()) {
        _mTransactionDepth = previousDepth;
        return res;
    }
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::DatabaseConnection::doTransaction(nbl::Span<PreparedStatement*> statements, i64* lastInsertRowId) {

    NBL_CHECK(beginTransaction());
    for(PreparedStatement* stmt : statements) {
        NBL_CHECK(execute(stmt, lastInsertRowId));
    }
    return commitTransaction();
}

nbl::Result nbl::DatabaseConnection::open(nbl::StringView path, DatabaseConnection* outConnection) {

    nbl::Arena::create(1024 * 1024, &outConnection->_mArena);

    nbl::AllocationFrame frame(&outConnection->_mArena);
    nbl::StringView nullTerminatedPath;
    NBL_CHECK(nbl::StringBuilder::nullTerminate(path, &nullTerminatedPath, &frame));

    int rc = sqlite3_open_v2(
        nullTerminatedPath.data(), 
        reinterpret_cast<sqlite3**>(&outConnection->_pHandle), 
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 
        nullptr);

    if(rc != SQLITE_OK) {
        nbl::log(nbl::LogLevel::ERROR, "Failed to open database: ", sqlite3_errmsg(reinterpret_cast<sqlite3*>(outConnection->_pHandle)));
        return nbl::Result::IO_ERROR;
    }

    sqlite3_create_function(
            static_cast<sqlite3*>(outConnection->_pHandle),
            "nbl_str_distance",
            2,
            SQLITE_UTF8,
            nullptr,
            _strDistance,
            nullptr,
            nullptr);

    return nbl::Result::SUCCESS;
}

nbl::Result nbl::DatabaseConnection::openInMemory(DatabaseConnection* outConnection) {

    nbl::Arena::create(1024 * 1024, &outConnection->_mArena);

    int rc = sqlite3_open_v2(
        nullptr,
        reinterpret_cast<sqlite3**>(&outConnection->_pHandle), 
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_MEMORY,
        nullptr);

    if(rc != SQLITE_OK) {
        nbl::log(nbl::LogLevel::ERROR, "Failed to open database: ", sqlite3_errmsg(reinterpret_cast<sqlite3*>(outConnection->_pHandle)));
        return nbl::Result::IO_ERROR;
    }

    sqlite3_create_function(
            static_cast<sqlite3*>(outConnection->_pHandle),
            "nbl_str_distance",
            2,
            SQLITE_UTF8,
            nullptr,
            _strDistance,
            nullptr,
            nullptr);

    return nbl::Result::SUCCESS;
}

