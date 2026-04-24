/* 
 * This file is part of Download Manager.
 *
 * Download Manager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Copyright (C) 2026 Aliaksandr L.
 *
 * Download Manager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Download Manager. If not, see <https://www.gnu.org/licenses/>.
 */

#include "DownloadDatabase.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#include <assert.h>

CDownloadDatabase::CDownloadDatabase(const QString& dbPath)
{
    bool bSuccess;
    m_db = QSqlDatabase::addDatabase("QSQLITE"); // Use SQLite driver
    m_db.setDatabaseName(dbPath);

    bSuccess = initializeDatabase();
    assert( bSuccess );
}

CDownloadDatabase::~CDownloadDatabase()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
}

bool CDownloadDatabase::isOk() const
{
    return m_db.isOpen();
}

bool CDownloadDatabase::initializeDatabase()
{
    if (!m_db.open()) {
        qWarning() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery query(m_db);
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS downloads (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            url TEXT,
            dir TEXT,
            filename TEXT
        )
    )";

    if (!query.exec(sql)) {
        qWarning() << "Failed to create table:" << query.lastError().text();
        return false;
    }
    return true;
}

uint64_t CDownloadDatabase::insert(const QString& url, const QString& dir, const QString& fileName)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO downloads (url, dir, filename ) VALUES (?, ?, ?)");
    query.addBindValue(url);
    query.addBindValue(dir);
    query.addBindValue(fileName);

    if (query.exec()) {
        return query.lastInsertId().toULongLong();
    } else {
        qWarning() << "Insert failed:" << query.lastError().text();
        return 0;
    }
}

void CDownloadDatabase::enumerate(PFN_DATABASE_ENUM pfnCallback)
{
    QSqlQuery query(m_db);
    if (!query.exec("SELECT id, url, dir, filename FROM downloads")) {
        qWarning() << "Query failed:" << query.lastError().text();
        return;
    }

    while (query.next()) {
        uint64_t id = query.value(0).toULongLong();
        QString url = query.value(1).toString();
        QString dir = query.value(2).toString();
        QString filename = query.value(3).toString();

        pfnCallback(id, url, dir, filename);
    }
}

void CDownloadDatabase::deleteRecord(uint64_t Id)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM downloads WHERE id = ?");
    query.addBindValue(static_cast<qint64>(Id));
    if (!query.exec()) {
        qWarning() << "Delete failed:" << query.lastError().text();
    }
}
