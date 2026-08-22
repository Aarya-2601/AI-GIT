#include "metadata_db.hpp"

#include <sqlite3.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace Storage
{

MetadataDB::MetadataDB(const std::filesystem::path& path)
    : dbPath(path)
{
}

void MetadataDB::initialize()
{
    sqlite3* db = nullptr;

    int result = sqlite3_open(
        dbPath.string().c_str(),
        &db
    );

    if (result != SQLITE_OK)
    {
        std::string error =
            db ? sqlite3_errmsg(db)
               : "Unknown SQLite error";

        if (db)
        {
            sqlite3_close(db);
        }

        throw std::runtime_error(
            "Could not open SQLite database: " + error
        );
    }

    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS objects (
            object_id TEXT PRIMARY KEY,
            size INTEGER NOT NULL,
            type TEXT NOT NULL,
            created_at TEXT NOT NULL
        );
    )";

    char* errorMessage = nullptr;

    result = sqlite3_exec(
        db,
        sql,
        nullptr,
        nullptr,
        &errorMessage
    );

    if (result != SQLITE_OK)
    {
        std::string error =
            errorMessage
                ? errorMessage
                : "Unknown SQLite error";

        sqlite3_free(errorMessage);
        sqlite3_close(db);

        throw std::runtime_error(
            "Could not create objects table: " + error
        );
    }

    sqlite3_close(db);
}

void MetadataDB::addObject(
    const std::string& objectId,
    long long size,
    const std::string& type
)
{
    sqlite3* db = nullptr;

    int result = sqlite3_open(
        dbPath.string().c_str(),
        &db
    );

    if (result != SQLITE_OK)
    {
        std::string error =
            db ? sqlite3_errmsg(db)
               : "Unknown SQLite error";

        if (db)
        {
            sqlite3_close(db);
        }

        throw std::runtime_error(
            "Could not open SQLite database: " + error
        );
    }

    const char* sql = R"(
        INSERT OR IGNORE INTO objects
        (object_id, size, type, created_at)
        VALUES (?, ?, ?, datetime('now'));
    )";

    sqlite3_stmt* statement = nullptr;

    result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK)
    {
        std::string error = sqlite3_errmsg(db);

        sqlite3_close(db);

        throw std::runtime_error(
            "Could not prepare SQLite statement: " + error
        );
    }

    sqlite3_bind_text(
        statement,
        1,
        objectId.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int64(
        statement,
        2,
        size
    );

    sqlite3_bind_text(
        statement,
        3,
        type.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    result = sqlite3_step(statement);

    if (result != SQLITE_DONE)
    {
        std::string error = sqlite3_errmsg(db);

        sqlite3_finalize(statement);
        sqlite3_close(db);

        throw std::runtime_error(
            "Could not insert object metadata: " + error
        );
    }

    sqlite3_finalize(statement);
    sqlite3_close(db);
}

bool MetadataDB::objectExists(
    const std::string& objectId
) const
{
    sqlite3* db = nullptr;

    int result = sqlite3_open(
        dbPath.string().c_str(),
        &db
    );

    if (result != SQLITE_OK)
    {
        std::string error =
            db ? sqlite3_errmsg(db)
               : "Unknown SQLite error";

        if (db)
        {
            sqlite3_close(db);
        }

        throw std::runtime_error(
            "Could not open SQLite database: " + error
        );
    }

    const char* sql = R"(
        SELECT 1
        FROM objects
        WHERE object_id = ?
        LIMIT 1;
    )";

    sqlite3_stmt* statement = nullptr;

    result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK)
    {
        std::string error = sqlite3_errmsg(db);

        sqlite3_close(db);

        throw std::runtime_error(
            "Could not prepare SQLite query: " + error
        );
    }

    sqlite3_bind_text(
        statement,
        1,
        objectId.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    result = sqlite3_step(statement);

    bool exists = (result == SQLITE_ROW);

    sqlite3_finalize(statement);
    sqlite3_close(db);

    return exists;
}

ObjectMetadata MetadataDB::getObject(
    const std::string& objectId
) const
{
    sqlite3* db = nullptr;

    int result = sqlite3_open(
        dbPath.string().c_str(),
        &db
    );

    if (result != SQLITE_OK)
    {
        std::string error =
            db ? sqlite3_errmsg(db)
               : "Unknown SQLite error";

        if (db)
        {
            sqlite3_close(db);
        }

        throw std::runtime_error(
            "Could not open SQLite database: " + error
        );
    }

    const char* sql = R"(
        SELECT object_id, size, type, created_at
        FROM objects
        WHERE object_id = ?
        LIMIT 1;
    )";

    sqlite3_stmt* statement = nullptr;

    result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK)
    {
        std::string error = sqlite3_errmsg(db);

        sqlite3_close(db);

        throw std::runtime_error(
            "Could not prepare SQLite query: " + error
        );
    }

    sqlite3_bind_text(
        statement,
        1,
        objectId.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    result = sqlite3_step(statement);

    if (result != SQLITE_ROW)
    {
        sqlite3_finalize(statement);
        sqlite3_close(db);

        throw std::runtime_error(
            "Object metadata not found: " + objectId
        );
    }

    ObjectMetadata metadata;

    metadata.objectId =
        reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 0)
        );

    metadata.size =
        sqlite3_column_int64(statement, 1);

    metadata.type =
        reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 2)
        );

    metadata.createdAt =
        reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 3)
        );

    sqlite3_finalize(statement);
    sqlite3_close(db);

    return metadata;
}

std::vector<std::string> MetadataDB::getAllObjectIds() const
{
    sqlite3* db = nullptr;

    int result = sqlite3_open(
        dbPath.string().c_str(),
        &db
    );

    if (result != SQLITE_OK)
    {
        std::string error =
            db ? sqlite3_errmsg(db)
               : "Unknown SQLite error";

        if (db)
        {
            sqlite3_close(db);
        }

        throw std::runtime_error(
            "Could not open SQLite database: " + error
        );
    }

    const char* sql = "SELECT object_id FROM objects;";
    sqlite3_stmt* statement = nullptr;

    result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK)
    {
        std::string error = sqlite3_errmsg(db);
        sqlite3_close(db);

        throw std::runtime_error(
            "Could not prepare SQLite query: " + error
        );
    }

    std::vector<std::string> objectIds;

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        const char* text =
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 0)
            );

        if (text)
        {
            objectIds.emplace_back(text);
        }
    }

    sqlite3_finalize(statement);
    sqlite3_close(db);

    return objectIds;
}

}