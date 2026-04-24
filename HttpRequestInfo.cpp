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

#include "HttpRequestInfo.h"

CHttpRequestInfo::CHttpRequestInfo( const QUrl &url, DownloadFlags Flags ) :
    m_Url(url),
    m_Reply(nullptr),
    m_ContentLength(0)
{
    QNetworkRequest request( m_Url );

    m_Reply = m_NetworkManager.head( request );

    if ( Flags.testFlag( DownloadFlag::IgnoreSSLErrors ) )
        m_Reply->ignoreSslErrors();

    connect( m_Reply, &QNetworkReply::finished, this, &CHttpRequestInfo::onFinished );
    connect( m_Reply, &QNetworkReply::metaDataChanged, this, &CHttpRequestInfo::onMetaDataChanged );

}

CHttpRequestInfo::~CHttpRequestInfo()
{
    if ( m_Reply )
    {
        delete m_Reply;
        m_Reply = nullptr;
    }
}

bool CHttpRequestInfo::isError()
{
    return !m_ErrorString.isEmpty();
}

QString CHttpRequestInfo::getErrorString()
{
    return m_ErrorString;
}

void CHttpRequestInfo::onFinished()
{
#ifdef QT_DEBUG

    QVariant HttpStatus = m_Reply->attribute( QNetworkRequest::HttpStatusCodeAttribute );

    if ( !HttpStatus.isNull() )
    {
        qDebug() << __FUNCTION__ << " Status = " << HttpStatus.toUInt();
    }

#endif

    onReplyReceived( (m_Reply->error() != QNetworkReply::NoError), m_Reply );

    m_Reply->deleteLater();
    m_Reply = nullptr;

    emit DataReady();
}

void CHttpRequestInfo::onMetaDataChanged()
{
#ifdef QT_DEBUG
    QString acceptRangesValue = m_Reply->rawHeader( "Accept-Ranges" );
    qDebug() << "Accept-Ranges = " << acceptRangesValue;

    if ( acceptRangesValue == "bytes" )
    {
        auto headers = m_Reply->rawHeaderPairs();

        for ( const auto &item : headers )
        {
            QString name = item.first;
            qDebug() << name << " = " << QString( item.second );
        }
    }
#endif
}

void CHttpRequestInfo::onReplyReceived(bool bError, QNetworkReply *Reply)
{
    m_ResumeAvail = false;

    if ( !bError )
    {
        QVariant lastModified = Reply->header(QNetworkRequest::LastModifiedHeader);
        if (lastModified.isValid()) {
            QDateTime dt = lastModified.toDateTime();
            qDebug() << "Last Modified:" << dt.toString();
        }

        bool bOk = false;
        QVariant varContentLength =  Reply->header(QNetworkRequest::ContentLengthHeader);
        m_ContentLength = 0;

        if ( !varContentLength.isNull() )
            m_ContentLength = varContentLength.toULongLong( &bOk );

        QByteArray acceptRangesValue = Reply->rawHeader( "Accept-Ranges" );

        if ( acceptRangesValue.compare( "bytes", Qt::CaseSensitivity::CaseInsensitive ) == 0 )
            m_ResumeAvail = true;
    }
    else
    {
        m_ErrorString = Reply->errorString();

        if ( m_ErrorString.isEmpty() )
            m_ErrorString = "Unknown network error";
    }


}

