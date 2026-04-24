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
 
#include "DlgDownloadAdd.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QApplication>
#include <QStyle>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QClipboard>
#include <QDir>
#include <QMessageBox>
#include "DownloadManager.h"

CDlgDownloadAdd::CDlgDownloadAdd(QWidget *parent,bool bResume, const QString &ExistingFileName,DownloadFlags DownloadFlags) :
    QDialog(parent),
    m_bResume(bResume),
    m_DownloadFlags(DownloadFlags)
{
    setWindowTitle( bResume ? "Resume Download" : "New Download" );
    //remove icon from dialog caption bar
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    QVBoxLayout *mainLayout = new QVBoxLayout();

    QGridLayout *outputFileLayout = new QGridLayout();

    QLabel *urlLabel = new QLabel("URL:", this);
    urlLabel->setAlignment( Qt::AlignRight );
    outputFileLayout->addWidget( urlLabel, 0, 0 );
    outputFileLayout->addWidget( m_UrlEdit = new QLineEdit(this), 0, 1 );
    QObject::connect( m_UrlEdit, &QLineEdit::textChanged, [this](){
        if (m_InfoLabel)
            m_InfoLabel->clear();
        } );
    m_UrlEdit->setMinimumWidth( 300 );

    QUrl clipUrl( QApplication::clipboard()->text(), QUrl::StrictMode );

    if ( clipUrl.isValid() && !clipUrl.scheme().isEmpty() )
    {
        qDebug() << "url scheme: " << clipUrl.scheme();
        m_UrlEdit->setText( clipUrl.toString() );
    }

    m_UrlInfoButton = new QPushButton("", this);
    QIcon infoIcon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation);
    m_UrlInfoButton->setIcon( infoIcon );
    m_UrlInfoButton->setToolTip( "Info" );
    m_UrlInfoButton->setFixedWidth( m_UrlInfoButton->sizeHint().width() + 10 );
    outputFileLayout->addWidget( m_UrlInfoButton, 0, 2 );
    connect( m_UrlInfoButton, &QPushButton::clicked, this, &CDlgDownloadAdd::onUrlInfo );

    QLabel *directoryOrFileLabel = new QLabel( bResume ? "File:" : "Directory:", this);

    directoryOrFileLabel->setAlignment( Qt::AlignRight );
    outputFileLayout->addWidget( directoryOrFileLabel, 1, 0  );
    outputFileLayout->addWidget( m_DirOrFileEdit = new QLineEdit(this), 1, 1 );
    m_DirOrFileEdit->setText( bResume ? ExistingFileName : CDownloadManager::getDefaultDownloadDir()  );

    QPushButton *browseButton = new QPushButton("Browse...", this);
    browseButton->setFixedWidth( browseButton->sizeHint().width() + 6 );
    outputFileLayout->addWidget( browseButton, 1, 2 );
    connect( browseButton,  &QPushButton::clicked, this, bResume ? &CDlgDownloadAdd::onBrowseResumeFile : &CDlgDownloadAdd::onBrowseOutputDirectory );

    mainLayout->addLayout( outputFileLayout );

    m_InfoLabel = new QLabel("", this);
    auto font = m_InfoLabel->font();
    font.setBold(true);
    m_InfoLabel->setFont( font );
    mainLayout->addWidget(m_InfoLabel);


    QPushButton *downloadButton = new QPushButton("Download", this);
    connect(downloadButton, &QPushButton::clicked, this, &CDlgDownloadAdd::onDownload );
    downloadButton->setDefault(true);

    QPushButton *closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject );

    QHBoxLayout *buttonsLayout = new QHBoxLayout();

    buttonsLayout->addStretch();
    buttonsLayout->addWidget( downloadButton );
    buttonsLayout->addWidget( closeButton );

    mainLayout->addSpacing( 5 );
    mainLayout->addLayout( buttonsLayout );

    setLayout(mainLayout);

    setFixedHeight( mainLayout->sizeHint().height() );

}

CDlgDownloadAdd::~CDlgDownloadAdd()
{
    if (m_pResumeInfo)
    {
        delete m_pResumeInfo;
        m_pResumeInfo = nullptr;
    }
}

QString CDlgDownloadAdd::getDir()
{
    assert( !m_bResume );
    return m_Dir;
}

QString CDlgDownloadAdd::getFile()
{
    assert( m_bResume );
    return m_File;
}

void CDlgDownloadAdd::onDownload(bool checked)
{
    Q_UNUSED(checked);

    m_InfoLabel->setText("");

    QUrl url ( m_UrlEdit->text().trimmed(), QUrl::StrictMode );

    if ( !url.isValid() || url.scheme().isEmpty() )
    {
        m_UrlEdit->selectAll();
        m_UrlEdit->setFocus();
        return;
    }

    QString scheme = url.scheme();

    if ( scheme != "https" && scheme != "http" )
    {
        m_UrlEdit->selectAll();
        m_InfoLabel->setText( "Error: The protocol is not supported." );
        return;
    }

    QString filePathValue = m_DirOrFileEdit->text().trimmed();

    if ( filePathValue.isEmpty() )
    {
        m_DirOrFileEdit->setFocus();
        return;
    }

    QFileInfo fileInfo(filePathValue);

    if ( m_bResume )
    {
        if ( !fileInfo.exists() || fileInfo.isDir() )
        {
            QMessageBox::critical( this, "Error", "The file does not exist.");
            m_DirOrFileEdit->setFocus();
            return;
        }

        if ( !fileInfo.isWritable() )
        {
            QMessageBox::critical( this, "Error", "The file is not writable.");
            m_DirOrFileEdit->setFocus();
            return;
        }

        m_File = fileInfo.absoluteFilePath();

    }
    else
    {
        if ( !fileInfo.exists() || !fileInfo.isDir() )
        {
            QMessageBox::critical( this, "Error", "The directory does not exist.");
            m_DirOrFileEdit->setFocus();
            return;
        }

        if ( !fileInfo.isWritable() )
        {
            QMessageBox::critical( this, "Error", "The directory is not writable.");
            m_DirOrFileEdit->setFocus();
            return;
        }

        m_Dir = fileInfo.absoluteFilePath();
    }

    m_Url = url;
    accept();
}

void CDlgDownloadAdd::onBrowseOutputDirectory()
{
    QString filename = QFileDialog::getExistingDirectory( this, "Select Output Directory", CDownloadManager::getDefaultDownloadDir() );

    if (!filename.isEmpty()) {
        m_DirOrFileEdit->setText(filename);
    }
}

void CDlgDownloadAdd::onBrowseResumeFile()
{
    QString filename = QFileDialog::getOpenFileName( this, "Select file", CDownloadManager::getDefaultDownloadDir() );

    if (!filename.isEmpty()) {
        m_DirOrFileEdit->setText(filename);
    }
}

void CDlgDownloadAdd::onUrlInfo()
{
    QUrl url ( m_UrlEdit->text(), QUrl::StrictMode );

    if ( !url.isValid() )
    {
        m_UrlEdit->selectAll();
        return;
    }

    if ( m_pResumeInfo )
    {
        delete m_pResumeInfo;
        m_pResumeInfo = nullptr;
    }

    m_pResumeInfo = new CHttpRequestInfo( url, m_DownloadFlags );
    m_UrlInfoButton->setEnabled(false);
    QObject::connect( m_pResumeInfo, &CHttpRequestInfo::DataReady, [this]() {

        m_UrlInfoButton->setEnabled(true);

        if (m_pResumeInfo==nullptr)
            return;

        if ( m_pResumeInfo->isError() )
        {
            m_InfoLabel->setText( "Error occured. " + m_pResumeInfo->getErrorString() );
            return;
        }

        QString LabelText = m_pResumeInfo->isResumeAvailable() ? "Resume is available " : "Resume is not available ";

        uint64_t ContentLength = m_pResumeInfo->getContentLength();

        if ( ContentLength != 0 )
        {
            LabelText += " Size: " + QLocale().formattedDataSize( ContentLength, 2, QLocale::DataSizeTraditionalFormat ) + " ";
        }

        m_InfoLabel->setText( LabelText );
    });

}
