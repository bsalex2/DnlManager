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

#ifndef HTTPREQUESTINFO_H
#define HTTPREQUESTINFO_H

#include <QObject>
#include <QNetworkAccessManager>

#include "DownloadManager.h"

class CHttpRequestInfo: public QObject
{
    Q_OBJECT

public:

    CHttpRequestInfo() = delete;
    CHttpRequestInfo( const CHttpRequestInfo & ) = delete;

    CHttpRequestInfo( const QUrl &url, DownloadFlags Flags );
    ~CHttpRequestInfo();

    bool
    isResumeAvailable() { return m_ResumeAvail; }

    uint64_t
    getContentLength() { return m_ContentLength; }

    bool
    isError();

    QString
    getErrorString();

Q_SIGNALS:

    void DataReady();

protected Q_SLOTS:

    void onFinished();
    void onMetaDataChanged();

protected:

    virtual void onReplyReceived( bool bError, QNetworkReply *Reply );

private:

    QNetworkAccessManager m_NetworkManager;
    QUrl m_Url;
    QNetworkReply *m_Reply;
    bool m_ResumeAvail = false;
    QString m_ErrorString;
    uint64_t m_ContentLength;
};


#endif // HTTPREQUESTINFO_H
