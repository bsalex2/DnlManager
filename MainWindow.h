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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QTableView>
#include <QAbstractListModel>
#include <QHeaderView>
#include <QProgressBar>
#include <QPainter>
#include <QKeyEvent>
#include <QMainWindow>
#include <QToolBar>
#include <QTimer>
#include <QFileInfo>
#include <QMimeData>

#include "DownloadManager.h"
#include "TableViewEx.h"
#include "DownloadProgressPainter.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget *parent = nullptr);

private Q_SLOTS:

    void onExit(bool checked);
    void onDownloadNew();
    void onDownloadResume();
    void onIgnoreSSLErrors(bool checked);
    void onDbgTest();
    void onTimerRefresh();
    void onPause();
    void onRun();
    void onDelete();
    void onDropFile( const QString &filePath );
    void onCurrentRowChanged(const QModelIndex &current, const QModelIndex &previous);
    void onContextMenu(const QPoint &pos);
    void onListKeyPressed(QKeyEvent *event);

protected:

    void createMenuBar();
    void createToolbar();
    void createStatusBar();
    void updateToolbarButtonsState();

    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    CDownloadJobShared getFocusedItemJob();
    void doDelete();

Q_SIGNALS:

    void dropFile( const QString &FilePath );

private:

    enum { REFRESH_TIMER_INTERVAL = 1000 };

private:

    TableViewEx  *m_DownloadsTable;
    CDownloadManagerShared m_DownloadManagerPtr;
    QTimer m_TimerRefresh;
    QAction *m_RunAction;
    QAction *m_PauseAction;
    QAction *m_DeleteAction;
};

#endif // MAINWINDOW_H
