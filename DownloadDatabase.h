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


#ifndef DOWNLOADDATABASE_H
#define DOWNLOADDATABASE_H

#include <QObject>
#include <QString>
#include <QSqlQuery>

#include <stdint.h>
#include <vector>
#include <functional>


class CDownloadDatabase : public QObject
{
    Q_OBJECT

public:
    explicit CDownloadDatabase(const QString &dbPath = "downloads.db");
    ~CDownloadDatabase();

    bool isOk() const;

    uint64_t insert(const QString &url, const QString& dir, const QString &fileName );

    typedef std::function<void (uint64_t Id, const QString &url, const QString &dir, const QString &fileName)> PFN_DATABASE_ENUM;

    void enumerate(PFN_DATABASE_ENUM pfnCallback);

    void deleteRecord(uint64_t Id);

private:
    QSqlDatabase m_db;

    bool initializeDatabase();
};

#endif // DOWNLOADDATABASE_H
