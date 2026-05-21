/* 
 * This file is part of Download Manager.
 *
 * Download Manager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#include "MainWindow.h"
#include <QIntValidator>
#include <QMenu>
#include <QMenuBar>
#include <QApplication>
#include <QMessageBox>
#include <QMimeData>
#include <QClipboard>
#include <QDir>
#include <QDesktopServices>
#include <algorithm>

#include "DlgDownloadAdd.h"
#include "DownloadsTableModel.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    m_DownloadsTable(nullptr),
    m_DownloadManagerPtr(  QSharedPointer<CDownloadManager>::create(
          QSharedPointer<CDownloadDatabase>::create( CDownloadManager::getDefaultDownloadDir() + QDir::separator() + "DnlManager.db" ) ) ),
    m_RunAction(nullptr),
    m_PauseAction(nullptr),
    m_DeleteAction(nullptr),
    m_OpenFileDirAction(nullptr)
{
    setWindowTitle("Download manager");
    setAcceptDrops(true);

    //use own dropFile signal to avoid freeze. We use Qt::QueuedConnection to achieve asynchronous behavior
    QObject::connect( this, &MainWindow::dropFile, this, &MainWindow::onDropFile, Qt::QueuedConnection );

    createMenuBar();
    createToolbar();
    createStatusBar();

    m_DownloadsTable =  new TableViewEx();


    m_DownloadsTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    DownloadsTableModel *model = new DownloadsTableModel( nullptr, m_DownloadManagerPtr );
    m_DownloadsTable->setModel(model);
    m_DownloadsTable->setItemDelegateForColumn( DownloadsTableModel::Column::Progress, new DownloadProgressPainter( m_DownloadsTable, m_DownloadManagerPtr ) );

    m_DownloadsTable->resizeColumnsToContents();

    m_DownloadsTable->horizontalHeader()->setSectionResizeMode( DownloadsTableModel::Column::Url, QHeaderView::Stretch);

    m_DownloadsTable->setColumnWidth( DownloadsTableModel::Column::Progress, 150 );
    m_DownloadsTable->setColumnWidth( DownloadsTableModel::Column::FileName, 100 );
    m_DownloadsTable->setColumnWidth( DownloadsTableModel::Column::Size, 90 );
    m_DownloadsTable->setColumnWidth( DownloadsTableModel::Column::Speed, 110 );

    m_DownloadsTable->resizeRowsToContents();

    m_DownloadsTable->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    m_DownloadsTable->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection); //QAbstractItemView::SelectionMode::ExtendedSelection QAbstractItemView::SelectionMode::SingleSelection

    m_DownloadsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect( m_DownloadsTable, &QTableView::customContextMenuRequested, this, &MainWindow::onContextMenu );
    QObject::connect( m_DownloadsTable, &TableViewEx::keyPressed, this, &MainWindow::onListKeyPressed );

    QObject::connect( m_DownloadsTable->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &MainWindow::onTableCurrentRowChanged );
    QObject::connect( m_DownloadsTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onTableSelectionChanged );

    setCentralWidget(m_DownloadsTable);

    QObject::connect( &m_TimerRefresh, &QTimer::timeout, this, &MainWindow::onTimerRefresh );
    m_TimerRefresh.start(REFRESH_TIMER_INTERVAL);

    updateToolbarButtonsState();

    resize(800,500);
}

void MainWindow::onExitMenu(bool checked)
{
    Q_UNUSED(checked);

    QApplication::quit();
}

void MainWindow::onDownloadNew()
{

    CDlgDownloadAdd dlg( this, false, "", m_DownloadManagerPtr->getDownloadFlags() );

    if ( dlg.exec() )
    {
        std::size_t Row = -1;
        m_DownloadManagerPtr->newDownloadJob( &Row, dlg.getUrl(), dlg.getDir() );

        QModelIndex modelIndex = m_DownloadsTable->model()->index(Row, 0);
        m_DownloadsTable->selectionModel()->select(modelIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        m_DownloadsTable->setCurrentIndex( modelIndex );
        m_DownloadsTable->scrollTo(modelIndex, QAbstractItemView::EnsureVisible);
    }
}

void MainWindow::onDownloadResume()
{
    CDlgDownloadAdd dlg( this, true, "", m_DownloadManagerPtr->getDownloadFlags() );

    if ( dlg.exec() )
    {
        m_DownloadManagerPtr->resumeDownloadJob( dlg.getUrl(), dlg.getFile() );
    }
}

void MainWindow::onIgnoreSSLErrors(bool checked)
{
    m_DownloadManagerPtr->setIgnoreSSLErrorsFlag( checked );
}

void MainWindow::onDbgTest()
{
    m_DownloadManagerPtr->newDownloadJob( nullptr, QUrl("https://www.google.com"), CDownloadManager::getDefaultDownloadDir() );
}

void MainWindow::onTimerRefresh()
{
     int firstRow = m_DownloadsTable->rowAt(0);

     if ( firstRow == -1 )
         return;

     int lastRow = m_DownloadsTable->rowAt(m_DownloadsTable->viewport()->height() - 1);

     if (lastRow == -1)
     {
         lastRow = firstRow + m_DownloadsTable->model()->rowCount() - 1;
     }

     auto currentIndex = m_DownloadsTable->currentIndex();

     auto fnRefreshRow = [this,currentIndex]( int row ) {
         auto model = m_DownloadsTable->model();
         model->dataChanged(model->index(row, 0), model->index(row, model->columnCount() - 1));

         if ( currentIndex.isValid() && currentIndex.row() == row )
             updateToolbarButtonsState();
     };

     for ( int iRow = firstRow; iRow <= lastRow; iRow++ )
     {
         auto currJob = m_DownloadManagerPtr->getJob( iRow );

         if ( !currJob )
             break;

         if ( currJob->isUpdated() )
         {
             currJob->clearUpdatedFlag();
             fnRefreshRow( iRow );
         }
     }
}

void MainWindow::onPause()
{
    QModelIndex currentIndex = m_DownloadsTable->currentIndex();

    if ( !currentIndex.isValid() )
        return;

    auto currentJob = m_DownloadManagerPtr->getJob( currentIndex.row() );

    if ( !currentJob )
        return;

    currentJob->pause();
}

void MainWindow::onRun()
{
    QModelIndex currentIndex = m_DownloadsTable->currentIndex();

    if ( !currentIndex.isValid() )
        return;

    auto currentJob = m_DownloadManagerPtr->getJob( currentIndex.row() );

    if ( !currentJob )
        return;

    currentJob->resume( m_DownloadManagerPtr.data() );
}

void MainWindow::onDelete()
{
    doDelete();
}

void MainWindow::onOpenDownloadDir()
{
    QModelIndex currentIndex = m_DownloadsTable->currentIndex();

    if ( !currentIndex.isValid() )
        return;

    auto Job = m_DownloadManagerPtr->getJob( currentIndex.row() );

    if ( !Job )
    {
        return;
    }

    QDesktopServices::openUrl( QUrl( "file:///" + Job->getDirectory() ) );
}

void MainWindow::onDropFile(const QString &filePath)
{
    CDlgDownloadAdd dlg( this, true, filePath, m_DownloadManagerPtr->getDownloadFlags() );

    if ( dlg.exec() )
    {
        m_DownloadManagerPtr->resumeDownloadJob( dlg.getUrl(), dlg.getFile() );
    }
}

void MainWindow::onTableCurrentRowChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED( previous );

    //updateToolbarButtonsState();

    auto currentJob = m_DownloadManagerPtr->getJob( current.row() );

    if ( !currentJob )
        return;

    if ( currentJob->getState() == CDownloadJob::DownloadState::Error )
    {
        statusBar()->showMessage( currentJob->getErrorString() );
        return;
    }

    statusBar()->showMessage("");

}

void MainWindow::onTableSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED( selected );
    Q_UNUSED( deselected );
    updateToolbarButtonsState();
}

void MainWindow::onContextMenu(const QPoint &pos)
{
    QPoint globalPos = m_DownloadsTable->viewport()->mapToGlobal(pos);

    auto fnCopyFilePath = [this](bool checked) {

        Q_UNUSED( checked );

        auto currentJob = getFocusedItemJob();

        if ( !currentJob )
            return;

        QApplication::clipboard()->setText( currentJob->getFilePath() );
    };

    auto fnCopyUrl = [this](bool checked) {

        Q_UNUSED( checked );

        auto currentJob = getFocusedItemJob();

        if ( !currentJob )
            return;

        QApplication::clipboard()->setText( currentJob->getUrl().toString() );
    };

    QMenu contextMenu;

    auto currentJob = getFocusedItemJob();

    if ( currentJob )
    {
        if ( currentJob->canRun() )
        {
            QAction *runAction = contextMenu.addAction("Run");
            connect( runAction, &QAction::triggered, this, &MainWindow::onRun );
        }

        if ( currentJob->canPause() )
        {
            QAction *pauseAction = contextMenu.addAction("Pause");
            connect( pauseAction, &QAction::triggered, this, &MainWindow::onPause );
        }

        if ( contextMenu.actions().count() > 0 )
            contextMenu.addSeparator();
    }

    QAction *copyFilePathAction = contextMenu.addAction("Copy File Path");
    connect( copyFilePathAction, &QAction::triggered, fnCopyFilePath );

    QAction *copyUrlAction = contextMenu.addAction("Copy URL");
    connect( copyUrlAction, &QAction::triggered, fnCopyUrl );

    contextMenu.exec(globalPos); //QAction *selectedAction
}

void MainWindow::onListKeyPressed(QKeyEvent *event)
{
    switch ( event->key() )
    {
        case Qt::Key_Delete:
        {
            doDelete();
            break;
        }

        case Qt::Key_A:
        {
            if ( event->modifiers() == Qt::KeyboardModifier::ControlModifier )
            {
                m_DownloadsTable->selectAll();
            }

            break;
        }
    }
}

void MainWindow::createMenuBar()
{
    auto menuBar = this->menuBar();

    QMenu *fileMenu = new QMenu("File");
    menuBar->addMenu( fileMenu );

    QAction *newDownloadAction = new QAction("New Download...");
    newDownloadAction->setShortcut( QKeySequence(Qt::CTRL | Qt::Key_N) ); //QKeySequence(Qt::CTRL | Qt::Key_Insert)
    fileMenu->addAction(newDownloadAction);
    connect( newDownloadAction, &QAction::triggered, this, &MainWindow::onDownloadNew );

    QAction *resumeDownloadAction = new QAction("Resume Download...");
    fileMenu->addAction(resumeDownloadAction);
    connect( resumeDownloadAction, &QAction::triggered, this, &MainWindow::onDownloadResume );

    QAction *ignoreSSLErrorsAction = new QAction("Ignore SSL errros");
    ignoreSSLErrorsAction->setCheckable(true);
    ignoreSSLErrorsAction->setChecked( m_DownloadManagerPtr && m_DownloadManagerPtr->getDownloadFlags().testFlag( DownloadFlag::IgnoreSSLErrors ) );
    fileMenu->addAction(ignoreSSLErrorsAction);
    connect( ignoreSSLErrorsAction, &QAction::triggered, this, &MainWindow::onIgnoreSSLErrors );


#ifdef QT_DEBUG

    QAction *dbgTestAction = new QAction("Test");
    dbgTestAction->setShortcut( QKeySequence(Qt::CTRL | Qt::Key_T) );

    fileMenu->addAction(dbgTestAction);
    connect( dbgTestAction, &QAction::triggered, this, &MainWindow::onDbgTest );

#endif

    fileMenu->addSeparator();

    QAction *exitAction = new QAction("Exit");
    fileMenu->addAction( exitAction );
    connect(exitAction, &QAction::triggered, this, &MainWindow::onExitMenu );

    //-- file menu end


    QMenu *helpMenu = new QMenu("Help");
    menuBar->addMenu( helpMenu );
    QAction *aboutAction = new QAction("About");
    helpMenu->addAction(aboutAction);

    connect( aboutAction, &QAction::triggered, [this]() {
            QMessageBox::about(this, "About",
                           "Version 1.0\n"
                           "© 2026 Aliaksandr L.");
            }
        );

}

void MainWindow::createToolbar()
{
    QToolBar *toolbar = addToolBar("Main Toolbar");

    m_RunAction = new QAction( QApplication::style()->standardIcon(QStyle::SP_MediaPlay), "Run", this);
    connect( m_RunAction, &QAction::triggered, this, &MainWindow::onRun );
    toolbar->addAction(m_RunAction);

    m_PauseAction = new QAction( QApplication::style()->standardIcon(QStyle::SP_MediaPause), "Pause", this );
    connect( m_PauseAction, &QAction::triggered, this, &MainWindow::onPause );
    toolbar->addAction(m_PauseAction);

    m_DeleteAction = new QAction( QApplication::style()->standardIcon(QStyle::SP_TrashIcon), "Delete", this );
    connect( m_DeleteAction, &QAction::triggered, this, &MainWindow::onDelete );
    toolbar->addAction(m_DeleteAction);

    toolbar->addSeparator();

    m_OpenFileDirAction = new QAction( QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon), "Open directory", this );
    connect( m_OpenFileDirAction, &QAction::triggered, this, &MainWindow::onOpenDownloadDir );
    toolbar->addAction(m_OpenFileDirAction);
}

void MainWindow::createStatusBar()
{
    auto *pStatusBar = statusBar();

    pStatusBar->showMessage( "" );
}

void MainWindow::updateToolbarButtonsState()
{
    if ( m_RunAction == nullptr || m_PauseAction == nullptr || m_DeleteAction == nullptr )
        return;

    if ( !m_DownloadsTable )
        return;

    auto currentIndex = m_DownloadsTable->currentIndex();

    if ( !currentIndex.isValid() )
    {
        m_RunAction->setEnabled(false);
        m_PauseAction->setEnabled(false);
        m_DeleteAction->setEnabled(false);
        m_OpenFileDirAction->setEnabled(false);
        return;
    }

    auto Job = m_DownloadManagerPtr->getJob( currentIndex.row() );

    if ( !Job )
        return;

    m_DeleteAction->setEnabled( Job->canDelete() );
    m_PauseAction->setEnabled( Job->canPause() );
    m_RunAction->setEnabled( Job->canRun() );
    m_OpenFileDirAction->setEnabled(true);
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasUrls())
    {
        QList<QUrl> urlList = mimeData->urls();

        if ( urlList.size() == 0 )
            return;

        QString filePath = urlList.at(0).toLocalFile();

        if ( filePath.isEmpty() )
            return;

        event->acceptProposedAction();

        //avoid freeze of drop file source app
        emit dropFile( filePath );
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    //TODO: warn an user for incomplete downloads
    event->accept();

    m_DownloadManagerPtr->abortAllJobsAndSaveDatabase();
}

CDownloadJobShared MainWindow::getFocusedItemJob()
{
    auto currentIndex = m_DownloadsTable->currentIndex();

    if ( !currentIndex.isValid() )
        return nullptr;

    return m_DownloadManagerPtr->getJob( currentIndex.row() );

}

void MainWindow::doDelete()
{    
    QModelIndexList selection = m_DownloadsTable->selectionModel()->selectedRows();

    if ( selection.count() == 0 )
        return;

    std::sort(selection.begin(), selection.end(),
        [](const QModelIndex &a, const QModelIndex &b) {
            return a.row() < b.row();
        }
        );

    bool deleteDataCheckBoxAvailable = true;

    for ( auto iterCurIndex = selection.rbegin(); iterCurIndex != selection.rend(); iterCurIndex++ )
    {
        CDownloadJobShared Job = m_DownloadManagerPtr->getJob( iterCurIndex->row() );

        if ( Job && Job->getState() == CDownloadJob::DownloadState::Completed )
            deleteDataCheckBoxAvailable = false;
    }

    QMessageBox msgBox;
    msgBox.setWindowTitle( QApplication::applicationName() );
    msgBox.setText( QString("Are you sure you want to delete the selected download(s)?") );
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    QCheckBox *deleteFileCheckBox = nullptr;

    if ( deleteDataCheckBoxAvailable )
    {
        deleteFileCheckBox = new QCheckBox("Delete the file");
        deleteFileCheckBox->setChecked(true);
        msgBox.setCheckBox(deleteFileCheckBox);
    }

    if ( msgBox.exec() != QMessageBox::Yes )
        return;

    bool deleteFile = false;

    if ( deleteFileCheckBox != nullptr )
        deleteFile = deleteFileCheckBox->isChecked();

    for ( auto iterCurIndex = selection.rbegin(); iterCurIndex != selection.rend(); iterCurIndex++ )
    {
        m_DownloadManagerPtr->deleteJob( iterCurIndex->row(), deleteFile );
    }

    updateToolbarButtonsState();
}



