// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
#pragma once

#include <string>

#include <sql.h>
#include <sqlext.h>

#include <gmock/gmock.h>

namespace databricks {
namespace test {

/**
 * @brief Mock interface for ODBC operations
 *
 * This interface allows us to mock ODBC calls for testing without
 * requiring actual database connections.
 */
class MockODBCInterface {
public:
    virtual ~MockODBCInterface() = default;

    MOCK_METHOD(SQLRETURN, SQLAllocHandle, (SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE* OutputHandlePtr));

    MOCK_METHOD(SQLRETURN, SQLFreeHandle, (SQLSMALLINT HandleType, SQLHANDLE Handle));

    MOCK_METHOD(SQLRETURN, SQLSetEnvAttr,
                (SQLHENV EnvironmentHandle, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength));

    MOCK_METHOD(SQLRETURN, SQLSetConnectAttr,
                (SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength));

    MOCK_METHOD(SQLRETURN, SQLDriverConnect,
                (SQLHDBC hdbc, SQLHWND hwnd, SQLCHAR* szConnStrIn, SQLSMALLINT cchConnStrIn, SQLCHAR* szConnStrOut,
                 SQLSMALLINT cchConnStrOutMax, SQLSMALLINT* pcchConnStrOut, SQLUSMALLINT fDriverCompletion));

    MOCK_METHOD(SQLRETURN, SQLDisconnect, (SQLHDBC ConnectionHandle));

    MOCK_METHOD(SQLRETURN, SQLExecDirect, (SQLHSTMT StatementHandle, SQLCHAR* StatementText, SQLINTEGER TextLength));

    MOCK_METHOD(SQLRETURN, SQLNumResultCols, (SQLHSTMT StatementHandle, SQLSMALLINT* ColumnCountPtr));

    MOCK_METHOD(SQLRETURN, SQLFetch, (SQLHSTMT StatementHandle));

    MOCK_METHOD(SQLRETURN, SQLGetData,
                (SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType, SQLPOINTER TargetValuePtr,
                 SQLLEN BufferLength, SQLLEN* StrLen_or_IndPtr));

    MOCK_METHOD(SQLRETURN, SQLGetDiagRec,
                (SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber, SQLCHAR* SQLState,
                 SQLINTEGER* NativeErrorPtr, SQLCHAR* MessageText, SQLSMALLINT BufferLength,
                 SQLSMALLINT* TextLengthPtr));
};

/**
 * @brief Mock for testing configuration file operations
 */
class MockFileSystem {
public:
    virtual ~MockFileSystem() = default;

    MOCK_METHOD(bool, file_exists, (const std::string& path));
    MOCK_METHOD(std::string, read_file, (const std::string& path));
    MOCK_METHOD(const char*, getenv, (const char* name));
};

} // namespace test
} // namespace databricks
