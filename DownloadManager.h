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


#ifndef _DOWNLOADMANAGER_H_
#define _DOWNLOADMANAGER_H_

#include <vector>

#include <QObject>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QSharedPointer>
#include <chrono>
#include <QElapsedTimer>
#include <QFlags>


#include "DownloadDatabase.h"

enum DownloadFlag {
    DefaultFlag = 0,
    IgnoreSSLErrors = 1
};

Q_DECLARE_FLAGS( DownloadFlags, DownloadFlag )

class CDownloadJob;
class CDownloadManager;

class CDownloadJob : public QObject
{
    Q_OBJECT

public:

    enum class DownloadState
    {
        Wait,
        RequestSending,
        ReplyReceiving,
        Completed,
        Paused,
        Error
    };

private:

    CDownloadJob() = delete;
    CDownloadJob( const CDownloadJob & ) = delete;

    void
    downloadInternal( CDownloadManager *pManager );

protected:

    typedef uint32_t ID_TYPE;

    friend class CDownloadManager;

    CDownloadJob( ID_TYPE Id, const QUrl &url, const QString &dir, const QString &filename, CDownloadJob::DownloadState State, uint64_t DbRecordId = 0 );

    void
    download( CDownloadManager *pManager );

    void
    setTotalDataReceived( uint64_t totalDataSize );

public:


    ~CDownloadJob();

    CDownloadJob::DownloadState
    getState();

    ID_TYPE
    getId();

    bool
    isContentLengthAvailable();

    uint64_t
    getContentLength();

    uint64_t
    getDataReceived();

    uint32_t
    getProgress();

    QUrl
    getUrl();

    QString
    getFilePath();

    QString
    getDirectory();

    QString
    getFileName();

    bool
    isUpdated();

    void
    clearUpdatedFlag();

    QString
    getErrorString();

    uint64_t
    getSpeedInBytesPerSec();

    uint64_t
    getDbRecordId();

    void
    pause();

    void
    resume( CDownloadManager *pManager );

    bool
    canDelete();

    bool
    canPause();

    bool
    canRun();

private Q_SLOTS:

    void onFinished();
    void onReadyRead();
    void onMetaDataChanged();


private:

    void updateDownloadState( CDownloadJob::DownloadState NewState );
    void updateFileName( const QString &NewFileName );
    void updateContentLength( uint64_t ContentLength );
    void updateDataReceived( uint64_t DataReceived );
    void updateErrorString( const QString &ErrorString );
    void updateState( CDownloadJob::DownloadState NewState );
    void setErrorState( const QString &ErrorString );

    QString generateFileName( const QString &dirPath, const QString &fileName );
    uint64_t getTickInMiliSec();

    void DebugDumpReplyHeaders(const char *Msg);

private:

    uint64_t m_ContentLength;
    uint64_t m_SessionDataReceived;
    uint64_t m_TotalDataReceived;
    DownloadState m_State;
    QNetworkReply *m_Reply;
    QFile *m_File;
    QString m_Directory;
    QString m_FileName;
    QUrl m_Url;
    ID_TYPE m_Id;
    bool m_Updated;
    QString m_ErrorString;
    uint64_t m_DownloadStartTime;
    uint64_t m_DownloadEndTime;
    QElapsedTimer m_Timer;
    uint64_t m_ResumeOffset;
    bool m_FirstRead;
    uint64_t m_DbRecordId;
};

using CDownloadJobShared = QSharedPointer<CDownloadJob>;

class CDownloadManager : public QObject
{
    Q_OBJECT

public:

    CDownloadManager( QSharedPointer<CDownloadDatabaseBase> Database );
    ~CDownloadManager();

    //on quit
    void
    abortAllJobsAndSaveDatabase();

    CDownloadJobShared
    newDownloadJob( const QUrl &url, const QString &directory );

    CDownloadJobShared
    resumeDownloadJob( const QUrl &url, const QString &FilePath );

    bool
    canDeleteJob( uint64_t jobNum );

    void
    deleteJob( uint64_t jobNum, bool bDeleteFile );

    std::size_t
    count();

    CDownloadJobShared
    getJob( std::size_t JobNum );

    CDownloadJobShared
    getJobById( CDownloadJob::ID_TYPE Id, std::size_t *pIndex = nullptr );

    static
    QString
    getDefaultDownloadDir();

    QNetworkAccessManager * getNetworkAccessManager() { return &m_NetworkAccessManager; }

    DownloadFlags
    getDownloadFlags(){ return m_DownloadFlags; }

    void
    setIgnoreSSLErrorsFlag(bool bOn );

Q_SIGNALS:

    void jobListUpdated();

private:

    bool getNextId( CDownloadJob::ID_TYPE *pId );

    std::vector<CDownloadJobShared> m_Jobs;
    QNetworkAccessManager m_NetworkAccessManager;
    uint32_t m_NextId;
    DownloadFlags m_DownloadFlags;
    QSharedPointer<CDownloadDatabaseBase> m_DownloadDatabase;

private:

    void loadDatabase();
    void saveDatabase();

};

using CDownloadManagerShared = QSharedPointer<CDownloadManager>;

#endif //_DOWNLOADMANAGER_H_

