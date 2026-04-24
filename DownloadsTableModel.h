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

#ifndef DOWNLOADSTABLEMODEL_H
#define DOWNLOADSTABLEMODEL_H

#include <QAbstractTableModel>
#include <QFileInfo>
#include <QMimeData>

#include "DownloadManager.h"

class DownloadsTableModel : public QAbstractTableModel {
    Q_OBJECT
public:

    explicit DownloadsTableModel(QObject *parent, CDownloadManagerShared DownloadManagerPtr )
        : QAbstractTableModel(parent),
        m_DownloadManagerPtr(DownloadManagerPtr)
    {
        connect( m_DownloadManagerPtr.data(), &CDownloadManager::jobListUpdated, this, &DownloadsTableModel::onJobListUpdated );
    }

    enum Column {
        RowNumber,
        FileName,
        Size,
        Completed,
        Progress,
        Speed,
        Url,

        ColumnCount
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        Q_UNUSED(parent);
        return m_DownloadManagerPtr->count();
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override {
        Q_UNUSED(parent);
        return Column::ColumnCount;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void
    fullRefresh()
    {
        beginResetModel();
        endResetModel();
    }

private Q_SLOTS:

    void onJobListUpdated()
    {
        fullRefresh();
    }

private:

    CDownloadManagerShared m_DownloadManagerPtr;
};



#endif // DOWNLOADSTABLEMODEL_H
