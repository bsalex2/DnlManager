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

class CDownloadDatabaseBase: public QObject
{
    Q_OBJECT
    
public:

    virtual ~CDownloadDatabaseBase() = default;

    virtual bool isOk() const = 0;
    
    virtual uint64_t insert(const QString &url, const QString& dir, const QString &fileName ) = 0;
    
    typedef std::function<void (uint64_t Id, const QString &url, const QString &dir, const QString &fileName)> PFN_DATABASE_ENUM;
    virtual void enumerate(PFN_DATABASE_ENUM pfnCallback) = 0;
    virtual void deleteRecord(uint64_t Id) = 0;
        
};

class CDownloadDatabase : public CDownloadDatabaseBase
{
    Q_OBJECT

public:

    CDownloadDatabase( const QString &dbPath );
    ~CDownloadDatabase();

    virtual bool isOk() const override;

    virtual uint64_t insert(const QString &url, const QString& dir, const QString &fileName ) override;
    
    typedef std::function<void (uint64_t Id, const QString &url, const QString &dir, const QString &fileName)> PFN_DATABASE_ENUM;
    virtual void enumerate(PFN_DATABASE_ENUM pfnCallback) override;

    virtual void deleteRecord(uint64_t Id) override;

private:

    QSqlDatabase m_db;

    bool initializeDatabase();
};

#endif // DOWNLOADDATABASE_H
