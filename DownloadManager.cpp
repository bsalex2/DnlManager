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

#include "DownloadManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QElapsedTimer>
#include <assert.h>
#include <type_traits>
#include <chrono>

CDownloadJob::CDownloadJob( CDownloadManager *pMgr, ID_TYPE Id, const QUrl &url, const QString &dir, const QString &filename, CDownloadJob::DownloadState State, uint64_t DbRecordId ) :
    m_DbRecordId( DbRecordId ),
    m_pMgr(pMgr)
{    
    m_Timer.start();
    m_DownloadStartTime = getTickInMiliSec();
    m_DownloadEndTime = 0;

    m_FirstRead = true;

    m_ContentLength = 0;
    m_SessionDataReceived = 0;
    m_TotalDataReceived = 0;
    m_Reply = nullptr;
    m_File = nullptr;

    m_Id = Id;
    m_Url = url;
    m_Directory = dir;
    m_FileName = filename;

    m_Updated = true;        
    m_State = State;

}

void CDownloadJob::downloadInternal(QNetworkAccessManager *pManager)
{
    m_FirstRead = true;
    m_DownloadStartTime = getTickInMiliSec();
    m_ErrorString.clear();

    updateDownloadState( DownloadState::RequestSending );

    QNetworkRequest request( m_Url );

    if ( m_ResumeOffset != 0 )
    {
        request.setRawHeader( "Range", QString("bytes=%1-").arg(m_ResumeOffset).toUtf8() );
    }

    m_Reply = pManager->get(request);

    if (  m_pMgr->getDownloadFlags().testFlag( DownloadFlag::IgnoreSSLErrors )  )
        m_Reply->ignoreSslErrors();

    m_Reply->setReadBufferSize( 50*1024 );

    connect( m_Reply, &QNetworkReply::finished, this, &CDownloadJob::onFinished );
    connect( m_Reply, &QNetworkReply::readyRead, this, &CDownloadJob::onReadyRead );
    connect( m_Reply, &QNetworkReply::metaDataChanged, this, &CDownloadJob::onMetaDataChanged );
}

void CDownloadJob::download(QNetworkAccessManager *pManager)
{
    assert( m_State == DownloadState::Wait );

    if ( m_State != DownloadState::Wait )
        return;

    m_ResumeOffset = 0;
    downloadInternal( pManager );
}

void CDownloadJob::setTotalDataReceived(uint64_t totalDataSize)
{
    assert( m_TotalDataReceived == 0 );
    m_TotalDataReceived = totalDataSize;
}

CDownloadJob::~CDownloadJob()
{
    if ( m_Reply )
    {
        delete m_Reply;
        m_Reply = nullptr;
    }

    if ( m_File )
    {
        delete m_File;
        m_File = nullptr;
    }
}

CDownloadJob::DownloadState CDownloadJob::getState()
{
    return m_State;
}

CDownloadJob::ID_TYPE CDownloadJob::getId()
{
    return m_Id;
}

bool CDownloadJob::isContentLengthAvailable()
{
    return m_ContentLength != 0;
}

uint64_t CDownloadJob::getContentLength()
{
    return m_ContentLength;
}

uint64_t CDownloadJob::getDataReceived()
{
    return m_TotalDataReceived;
}

uint32_t CDownloadJob::getProgress()
{
    if ( m_State == DownloadState::Completed )
        return 100;

    if ( !isContentLengthAvailable() )
        return 0;

    double fPercent = 100*(double( m_TotalDataReceived ) / double( m_ContentLength ));

    return (uint32_t)fPercent;
}

QUrl CDownloadJob::getUrl()
{
    return m_Url;
}

QString
CDownloadJob::getDirectory()
{
    return m_Directory;
}

QString CDownloadJob::getFileName()
{
    return m_FileName;
}

bool CDownloadJob::isUpdated()
{
    return m_Updated;
}

void CDownloadJob::clearUpdatedFlag()
{
    m_Updated = false;
}

QString CDownloadJob::getErrorString()
{
    return m_ErrorString;
}

uint64_t
CDownloadJob::getSpeedInBytesPerSec()
{
    if ( m_SessionDataReceived == 0 )
        return 0;

    uint64_t elapsed_in_seconds = 0;

    if ( m_DownloadEndTime == 0 )
        elapsed_in_seconds = (getTickInMiliSec() - m_DownloadStartTime) / 1000;
    else
        elapsed_in_seconds = (m_DownloadEndTime - m_DownloadStartTime) / 1000;

    if ( elapsed_in_seconds == 0 )
        return 0;

    uint64_t BytesPerSec = m_SessionDataReceived / elapsed_in_seconds;

    return BytesPerSec;
}

QString CDownloadJob::getSpeedString( uint64_t SpeedInBytesPerSec )
{
    if ( SpeedInBytesPerSec == 0 )
        return "";

    double fSpeed = SpeedInBytesPerSec;
    int unitIndex = 0;

    QStringList units{ "B/s", "KB/s", "MB/s",  "GB/s" };

    while ( fSpeed >= 1024 && unitIndex < units.size() - 1) {
        fSpeed /= 1024;
        unitIndex++;
    }

    return QString::number(fSpeed, 'f', 2) + " " + units.at(unitIndex);
}

uint64_t CDownloadJob::getDbRecordId()
{
    return m_DbRecordId;
}

void CDownloadJob::pause()
{
    bool bValidState = ( m_State == DownloadState::RequestSending || m_State == DownloadState::ReplyReceiving );
    assert( bValidState );

    if ( !bValidState )
    {
        return;
    }

    if ( m_Reply == nullptr )
        return;

    updateState( DownloadState::Paused );
    m_Reply->abort();
}

void CDownloadJob::resume( QNetworkAccessManager *pManager )
{
    bool bSuccess;

    if ( m_State != DownloadState::Paused && m_State != DownloadState::Error && m_State != DownloadState::Wait )
        return;

    if ( m_FileName.isEmpty() )
    {
        m_SessionDataReceived = 0;
        m_TotalDataReceived = 0;
        m_ResumeOffset = 0;        
        downloadInternal( pManager );
        return;
    }

    if ( m_File )
    {
        m_File->close();
        delete m_File;
        m_File = nullptr;
    }

    QString fullPath = getFilePath();

    if ( fullPath.isEmpty() )
    {
        setErrorState( "Path was not set" );
        return;
    }

    m_File = new QFile( fullPath );

    //QIODevice::WriteOnly caused the file to rewrite
    if ( !m_File->open( QIODevice::ReadWrite ) )
    {
        setErrorState( "Cannot open the file. " + m_File->errorString() );
        delete m_File;
        m_File = nullptr;
        return;
    }

    m_SessionDataReceived = 0;
    m_ResumeOffset = m_File->size();
    m_TotalDataReceived = m_ResumeOffset;
    m_ContentLength = 0;

    if ( m_ResumeOffset > 0 )
    {
        bSuccess = m_File->seek( m_ResumeOffset );
        assert(bSuccess);
    }

    assert( m_Reply == nullptr );

    downloadInternal( pManager );
}


void CDownloadJob::onFinished()
{
    //finished( reply->readAll(), reply->rawHeaderPairs() );

    m_DownloadEndTime = getTickInMiliSec();

    if ( m_File != nullptr )
    {
        m_File->close();
        delete m_File;
        m_File = nullptr;
    }

    if ( m_Reply == nullptr )
    {
        assert(false);
        return;
    }

    if ( m_State != DownloadState::Paused )
    {
        if ( m_Reply->error() == QNetworkReply::NoError )
        {
            updateState( DownloadState::Completed );
        }
        else
        {
            if ( m_ErrorString.isEmpty() )
                updateErrorString( m_Reply->errorString() );

            updateState( DownloadState::Error );
        }
    }

    m_Reply->deleteLater();
    m_Reply = nullptr;
}

void CDownloadJob::onReadyRead()
{
    bool bOk = false;

    updateState( DownloadState::ReplyReceiving );

    if ( m_Reply->error() != QNetworkReply::NoError )
    {
        qDebug() << "onReadyRead Error: " << m_Reply->errorString();
    }

    if ( m_FirstRead )
    {
        m_FirstRead = false;

        uint32_t uHttpStatus = m_Reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toUInt();

        qDebug() << "HttpStatus = " << uHttpStatus;

        if ( uHttpStatus >= 400 )
        {
            QString errStr = QString("Http Error. Status: ") + QString::number( uHttpStatus );

            switch ( uHttpStatus )
            {
                case 403:
                {
                    errStr += " (Forbidden)";
                    break;
                }
                case 404:
                {
                    errStr += " (Not Found)";
                    break;
                }
            }

            setErrorState( errStr );
            m_Reply->abort();
            return;
        }

        if ( m_ResumeOffset != 0 )
        {
            DebugDumpReplyHeaders( "Resume headers" );

            //"content-range"  =  "bytes 1511437-3291432/3291433"

            QString contentRangestr = QString::fromLatin1( m_Reply->rawHeader("Content-Range") );
            int lastPos = contentRangestr.lastIndexOf('/');

            if ( lastPos != -1 )
            {
                bOk = false;
                uint64_t totalSize = contentRangestr.mid(lastPos + 1).toULongLong( &bOk );

                if ( bOk )
                    m_ContentLength = totalSize;

                qDebug() << "contentRange = " << m_ContentLength;
            }

            //partial content
            if ( uHttpStatus != 206 )
            {
                if ( 416 == uHttpStatus )
                    setErrorState( "Cannot resume the download: Range Not Satisfiable" );
                else
                    setErrorState( QString("The server does not support resuming. HttpStatus=") + QString::number( uHttpStatus ) );

                m_Reply->abort();
                return;
            }
        }
    }

    QByteArray readAllData = m_Reply->readAll();

    if ( m_File == nullptr )
    {
        QString fullPath = getFilePath();

        if ( fullPath.isEmpty() )
        {
            setErrorState( "Filename was not set" );
            m_Reply->abort();
            return;
        }

        m_File = new QFile( fullPath );

        if ( !m_File->open( QIODevice::WriteOnly ) )
        {
            setErrorState( "Cannot create the file. " + m_File->errorString() );
            delete m_File;
            m_File = nullptr;
            m_Reply->abort();
            return;
        }
    }

    qint64 bytesWritten = m_File->write( readAllData );

    if ( bytesWritten != -1 )
    {
        m_TotalDataReceived += bytesWritten;
        updateDataReceived( m_SessionDataReceived + bytesWritten );
    }

    if ( bytesWritten == -1 || bytesWritten != readAllData.size() )
    {
        setErrorState( "Cannot write to the file." + m_File->errorString() );
        m_File->close();
        delete m_File;
        m_File = nullptr;
        m_Reply->abort();
        return;
    }
}

void CDownloadJob::onMetaDataChanged()
{
    bool bOk = false;

    if ( m_ResumeOffset == 0 )
    {
        //for resuming ContentLength has size of requested data
        QVariant varContentLength =  m_Reply->header(QNetworkRequest::ContentLengthHeader);
        uint64_t ContentLength = 0;

        if ( !varContentLength.isNull() )
            ContentLength = varContentLength.toULongLong( &bOk );

        if ( bOk && ContentLength > 0 )
            updateContentLength( ContentLength );
    }

    if ( m_FileName.isEmpty() )
    {

        QString FileName = m_Reply->url().fileName();

        if ( FileName.isEmpty() )
            FileName = "index.html";

        FileName = generateFileName( m_Directory, FileName );

        if ( FileName.isEmpty() )
        {
            m_Reply->abort();
        }

        updateFileName( FileName );
    }

}

void CDownloadJob::updateDownloadState(DownloadState NewState)
{
    bool bUpdated = m_State != NewState;
    m_State = NewState;

    if ( bUpdated )
        m_Updated = true;
}

void CDownloadJob::updateFileName(const QString &NewFileName)
{
    bool bUpdated = m_FileName != NewFileName;
    m_FileName = NewFileName;

    if (bUpdated)
        m_Updated = true;
}

void CDownloadJob::updateContentLength(uint64_t ContentLength)
{
    bool bUpdated = m_ContentLength != ContentLength;
    m_ContentLength = ContentLength;

    if (bUpdated)
        m_Updated = true;
}

void CDownloadJob::updateDataReceived(uint64_t DataReceived)
{
    bool bUpdated = m_SessionDataReceived != DataReceived;
    m_SessionDataReceived = DataReceived;

    if (bUpdated)
        m_Updated = true;
}

void CDownloadJob::updateErrorString(const QString &ErrorString)
{
    bool bUpdated = m_ErrorString != ErrorString;
    m_ErrorString = ErrorString;

    if (bUpdated)
        m_Updated = true;
}

void CDownloadJob::updateState(DownloadState NewState)
{
    bool bUpdated = m_State != NewState;

    if ( bUpdated && m_State == DownloadState::Error )
    {
        assert( DownloadState::RequestSending == NewState );

        if ( DownloadState::RequestSending != NewState )
            return;
    }

    m_State = NewState;

    if (bUpdated)
        m_Updated = true;
}

void CDownloadJob::setErrorState( const QString &ErrorString )
{
    updateErrorString( ErrorString );
    updateState( DownloadState::Error );
}

QString CDownloadJob::generateFileName(const QString &dirPath, const QString &fileName)
{
    QDir dir(dirPath);
    QString genFilename = fileName;

    genFilename.remove( ':' );
    genFilename.remove( '\\' );
    genFilename.remove( '/' );

    if ( genFilename.isEmpty() )
        genFilename = "index.html";

    QString filePath = dir.filePath( fileName );

    if ( !QFileInfo::exists( filePath ) )
    {
        return fileName;
    }

    QFileInfo fileInfo( filePath );

    QString baseName = fileInfo.baseName();
    QString ext = fileInfo.suffix();

    for ( uint32_t attempCount = 1; attempCount < 100; attempCount++ )
    {
        QString testFileName = baseName + QString::number(attempCount);

        if ( !ext.isEmpty() )
            testFileName += "." + ext;

        QString testFilePath = dir.filePath( testFileName );

        if ( !QFileInfo::exists( testFilePath ) )
        {
            return testFileName;
        }
    }

    setErrorState( "Cannot generate a filename" );
    qDebug() << "Cannot generate a filename from " << filePath;
    return "";

}

QString CDownloadJob::getFilePath()
{
    if ( m_Directory.isEmpty() || m_FileName.isEmpty() )
    {
        return "";
    }

    QDir dir( m_Directory );

    return  QDir::toNativeSeparators( dir.filePath( m_FileName ) );

}

uint64_t CDownloadJob::getTickInMiliSec()
{
    //return std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count();

    //auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    //std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now();
    return m_Timer.elapsed(); //
}

void CDownloadJob::DebugDumpReplyHeaders(const char *Msg)
{
    qDebug() << Msg;

    auto headers = m_Reply->rawHeaderPairs();

    for ( const auto &item : headers )
    {
        QString name = item.first;
        qDebug() << name << " = " << QString( item.second );
    }

}


CDownloadManager::CDownloadManager() :
    m_DownloadDatabase( getDefaultDownloadDir() + QDir::separator() + "downloads.db" )
{
    m_NextId = 1;
    m_DownloadFlags = DownloadFlag::DefaultFlag;

    loadDatabase();
}

CDownloadManager::~CDownloadManager()
{
    saveDatabase();
}

CDownloadJobShared CDownloadManager::newDownloadJob(const QUrl &url, const QString &directory )
{
    CDownloadJob::ID_TYPE Id = 0;

    if ( !getNextId( &Id ) )
        return nullptr;

    CDownloadJob *pJob = new CDownloadJob( this, Id, url, directory, "", CDownloadJob::DownloadState::Wait );

    if ( pJob == nullptr )
        return nullptr;

    pJob->download( &m_NetworkAccessManager );

    CDownloadJobShared ResultJob( pJob );

    m_Jobs.push_back( ResultJob );

    emit jobListUpdated();
    return ResultJob;
}

CDownloadJobShared CDownloadManager::resumeDownloadJob(const QUrl &url, const QString &FilePath )
{
    CDownloadJob::ID_TYPE Id = 0;

    if ( !getNextId( &Id ) )
        return nullptr;

    QFileInfo fileInfo( FilePath );

    CDownloadJob *pJob = new CDownloadJob( this, Id, url, fileInfo.absoluteFilePath(), fileInfo.fileName(), CDownloadJob::DownloadState::Paused );

    if ( pJob == nullptr )
        return nullptr;

    pJob->resume( &m_NetworkAccessManager );

    CDownloadJobShared ResultJob( pJob );

    m_Jobs.push_back( ResultJob );

    emit jobListUpdated();
    return ResultJob;
}

bool CDownloadManager::canDeleteJob(const CDownloadJobShared &Job)
{
    CDownloadJob::DownloadState State = Job->getState();

    return State != CDownloadJob::DownloadState::RequestSending &&
           State != CDownloadJob::DownloadState::ReplyReceiving;
}

bool CDownloadManager::canDeleteJob(uint64_t jobNum)
{
    if ( jobNum >= m_Jobs.size() )
    {
        assert(false);
        return false;
    }

    auto currJob = m_Jobs.at( jobNum );

    return canDeleteJob( currJob );
}

void CDownloadManager::deleteJob(uint64_t jobNum, bool bDeleteFile)
{
    if ( jobNum >= m_Jobs.size() )
    {
        assert(false);
        return;
    }

    if ( !canDeleteJob(jobNum) )
    {
        return;
    }

    auto Job = m_Jobs.at( jobNum );

    m_Jobs.erase( m_Jobs.begin() + jobNum );

    if ( Job->getDbRecordId() != 0 )
    {
        m_DownloadDatabase.deleteRecord(Job->getDbRecordId());
    }

    QString jobFilePath = Job->getFilePath();

    if ( bDeleteFile && !jobFilePath.isEmpty() && QFile::exists( jobFilePath ) )
    {
        if ( !QFile::remove(jobFilePath) )
        {
            qDebug() << "Failed to delete the file:\n" << jobFilePath;
        }
    }

    emit jobListUpdated();
}

std::size_t CDownloadManager::count()
{
    return m_Jobs.size();
}

CDownloadJobShared CDownloadManager::getJob(std::size_t JobNum)
{
    if ( JobNum >= m_Jobs.size() )
    {
        assert(false);
        return nullptr;
    }

    return m_Jobs.at(JobNum);
}

CDownloadJobShared CDownloadManager::getJobById(CDownloadJob::ID_TYPE Id,std::size_t *pIndex)
{
    auto iter = std::lower_bound(
        m_Jobs.begin(),
        m_Jobs.end(),
        Id,
        [](const CDownloadJobShared &currJob, CDownloadJob::ID_TYPE currId) {
            return currJob->getId() < currId;
        }
        );

    if ( iter == m_Jobs.end() || (*iter)->getId() != Id )
        return nullptr;

    if ( pIndex != nullptr )
        *pIndex = iter - m_Jobs.begin();

    return *iter;
}

QString CDownloadManager::getDefaultDownloadDir()
{
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);

    if ( !QDir(dirPath).exists() )
        dirPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

    return QDir::toNativeSeparators( dirPath );
}

void CDownloadManager::setIgnoreSSLErrorsFlag(bool bOn)
{
    m_DownloadFlags.setFlag( DownloadFlag::IgnoreSSLErrors, bOn );
}

void CDownloadManager::saveDatabase()
{
    uint64_t DbId;

    if ( !m_DownloadDatabase.isOk() )
        return;

    for ( auto iter = m_Jobs.begin(); iter != m_Jobs.end(); iter++ )
    {
        auto job = (*iter);

        DbId = job->getDbRecordId();

        if ( job->getState() == CDownloadJob::DownloadState::Completed )
        {
            if ( DbId != 0 )
            {
                m_DownloadDatabase.deleteRecord( DbId );
            }
        }
        else
        {
            if ( DbId == 0 )
            {
                DbId = m_DownloadDatabase.insert(
                    job->getUrl().toString(),
                    job->getDirectory(),
                    job->getFileName()
                    );

                assert(DbId != 0);
            }
        }

    }
}

bool CDownloadManager::getNextId(CDownloadJob::ID_TYPE *pId)
{
    if ( m_NextId == std::numeric_limits<CDownloadJob::ID_TYPE>::max() )
        return false;

    *pId = m_NextId++;
    return true;
}

void CDownloadManager::loadDatabase()
{
    if ( !m_DownloadDatabase.isOk() )
        return;

    m_DownloadDatabase.enumerate(
        [this](uint64_t DbRecordId, const QString &url, const QString &dir, const QString &fileName) {

            CDownloadJob::ID_TYPE ItemId = 0;
            getNextId( &ItemId );
            CDownloadJob *pJob = new CDownloadJob( this, ItemId, url, dir, fileName, CDownloadJob::DownloadState::Paused, DbRecordId );

            QFileInfo fileInfo( pJob->getFilePath() );
            pJob->setTotalDataReceived( fileInfo.size() );

            CDownloadJobShared JobShared(pJob);

            m_Jobs.push_back( JobShared );
        }
        );
}

