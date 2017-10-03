#ifndef AZFRAMEWORK_SQLITECONNECTION_H
#define AZFRAMEWORK_SQLITECONNECTION_H

/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/parallel/mutex.h>

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;

namespace AZ
{
    struct Uuid;
}

namespace AzToolsFramework
{
    namespace SQLite
    {
        class StatementPrototype;

        class Statement;

        /** AzToolsFramework::SQLite::Connection represents a barebones, single-threaded connection to a SQLite database
        */
        class Connection
        {
        public:
            AZ_CLASS_ALLOCATOR(Connection, AZ::SystemAllocator, 0)
            Connection(void);
            ~Connection(void);

            //! Open a database connection given a filename
            bool Open(const AZStd::string& filename, bool readOnly);
            void Close();
            bool IsOpen() const;

            // ----- Transaction support -----
            void BeginTransaction();
            void CommitTransaction();
            void RollbackTransaction();
            // -------------------------------

            //! SQLite-specific, compacts the database and cleans up any temporary space allocated.
            void Vacuum();

            //! Registers a prepared statement with the database
            void AddStatement(const AZStd::string& shortName, const AZStd::string& sqlStatement);
            void AddStatement(const char* shortName, const char* sqlStatement);

            //! Looks up a prepared statement and returns a Statement handle to it, which can then be used
            //! To bind paramaters and execute the statement.
            Statement* GetStatement(const AZStd::string& stmtName);

            //! Unregisters and finalizes the statement, freeing its memory
            void RemoveStatement(const char* name);

            //! Removes all statements and frees their memory.
            void FinalizeAll();

            //! Returns the ID of the last row affected.
            AZ::s64 GetLastRowID();

            //! Returns the number of rows affected by the most recent statment.
            int GetNumAffectedRows();

            //! If a Statement takes no parameters, you can execute it one-off without binding any parameters:
            bool ExecuteOneOffStatement(const char* name);

            //! Returns true if the given table name exists in the database.
            bool DoesTableExist(const char* name);

        private:
            sqlite3* m_db;
            typedef AZStd::unordered_map< AZStd::string, StatementPrototype* > StatementContainer;
            StatementContainer m_statementPrototypes;
        };


        /** A statement is a live, working-right now statement (or a cached one) which you are currently executing.
         *  All binding is by reference, so you must not destroy the bound objects before executing the statement.
        */
        class Statement
        {
        public:
            AZ_CLASS_ALLOCATOR(Statement, AZ::SystemAllocator, 0);
            Statement(StatementPrototype* parent);
            ~Statement();

            enum SqlStatus
            {
                SqlError = 0,
                SqlOK,
                SqlDone
            };


            SqlStatus Step();
            void Finalize();
            bool Prepared() const;
            bool PrepareFirstTime(sqlite3* db);
            bool Reset();

            // only valid during step()
            int FindColumn(const char* name);

            AZStd::string GetColumnText(int col);
            int GetColumnInt(int col);
            double GetColumnDouble(int col);
            const void* GetColumnBlob(int col);
            int GetColumnBlobBytes(int col);
            AZ::s64 GetColumnInt64(int col);

            AZ::Uuid GetColumnUuid(int col);

            bool BindValueUuid(int col, const AZ::Uuid& data);
            bool BindValueBlob(int col, void* data, int dataSize);
            bool BindValueDouble(int col, double data);
            bool BindValueInt(int col, int data);
            bool BindValueText(int idx, const char* data);
            bool BindValueInt64(int idx, AZ::s64 data);

            //! returns zero if it does not find the named index.
            int GetNamedParamIdx(const char* name);

            // internal use only
            const StatementPrototype* GetParentPrototype() const;

        private:
            sqlite3_stmt* m_statement;
            AZStd::unordered_map<AZStd::string, int> m_cachedColumnNames;
            StatementPrototype* m_parentPrototype;

            // no copy, no default construct allowed, no assignment, etc
            Statement(const Statement& other) = delete;
            Statement(const Statement&& other) = delete;
            Statement() = delete;
        };

        // a utiltity class which auto-finalizes a statement in a scope.
        // use Get() to retrieve the statement.  it will be null if it couldn't find it.
        // The auto finalizer owns the statement
        class StatementAutoFinalizer
        {
        public:
            StatementAutoFinalizer(Connection& connect, const char* statementName);
            Statement* Get() const;
            ~StatementAutoFinalizer();

            // no default construction allowed neither is copy:
            StatementAutoFinalizer(const StatementAutoFinalizer& other) = delete;
            StatementAutoFinalizer(StatementAutoFinalizer&& other) = delete;
            StatementAutoFinalizer& operator=(const StatementAutoFinalizer& other) = delete;

        private:
            Statement* m_statement = nullptr;
        };

        //! A utility class to limit a transaction by scope
        //! unless you tell it to commit the transaction it will revert automaticaly
        //! if scope is lost for any reason
        class ScopedTransaction
        {
        public:
            ScopedTransaction(Connection* connect);
            ~ScopedTransaction();
            void Commit();

            // no default construction allowed neither is copy:
            ScopedTransaction(const ScopedTransaction& other) = delete;
            ScopedTransaction(ScopedTransaction&& other) = delete;
            ScopedTransaction& operator=(const ScopedTransaction& other) = delete;
        private:
            Connection* m_connection = nullptr;
        };
    } // namespace SQLite
} // namespace AZFramework

#endif //AZFRAMEWORK_SQLITECONNECTION_H