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

#ifndef _DLGDOWNLOADADD_H_
#define _DLGDOWNLOADADD_H_

#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QStatusBar>
#include <QCheckBox>
#include <QUrl>

#include "DownloadManager.h"
#include "HttpRequestInfo.h"

class CDlgDownloadAdd : public QDialog
{
    Q_OBJECT
    
public:

    CDlgDownloadAdd( QWidget *parent, bool bResume, const QString &ExistingFileName, DownloadFlags DownloadFlags );
    ~CDlgDownloadAdd();

    QString getDir();
    QString getFile();
    QUrl getUrl() { return m_Url; }
    DownloadFlags getDownloadFlags() { return m_DownloadFlags; }

private Q_SLOTS:

    void onDownload(bool checked);
    void onBrowseOutputDirectory();
    void onBrowseResumeFile();
    void onUrlInfo();

private:

    QUrl m_Url;
    QString m_Dir;
    QString m_File;
    QLineEdit *m_UrlEdit = nullptr;
    QLineEdit *m_DirOrFileEdit = nullptr;
    QLabel *m_InfoLabel = nullptr;    
    CHttpRequestInfo *m_pResumeInfo = nullptr;
    QCheckBox *m_IgnoreSSLErrorsCheckBox = nullptr;
    QPushButton *m_UrlInfoButton = nullptr;
    bool m_bResume;
    DownloadFlags m_DownloadFlags;
    //QStatusBar *m_statusBar = nullptr;
    
};


#endif //_DLGDOWNLOADADD_H_
