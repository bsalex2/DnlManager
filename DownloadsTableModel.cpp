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

#include "DownloadsTableModel.h"

QVariant DownloadsTableModel::data(const QModelIndex &index, int role) const {
    if ( !index.isValid() )
        return QVariant();

    int row = index.row();
    int col = index.column();

    if (role == Qt::TextAlignmentRole)
    {
        switch (col)
        {
        case Column::Progress:
        case Column::Url:
        {
            break;
        }
        default:
        {
            return  QVariant( Qt::AlignRight | Qt::AlignVCenter );
            break;
        }
        }
    }
    else if ( role == Qt::DisplayRole )
    {
        CDownloadJobShared currentJob;

        if ( m_DownloadManagerPtr )
            currentJob = m_DownloadManagerPtr->getJob( row );

        switch (col) {
        case Column::RowNumber:
            return QString::number( row + 1 );
        case Column::FileName:
            return currentJob->getFileName();
        case Column::Size:
            return currentJob->isContentLengthAvailable() ? QLocale().formattedDataSize( currentJob->getContentLength(), 2, QLocale::DataSizeTraditionalFormat ) : "?";
        case Column::Completed:
            return QLocale().formattedDataSize( currentJob->getDataReceived(), 2, QLocale::DataSizeTraditionalFormat );

        case Column::Progress: // Progress
            return currentJob->getProgress();

        case Column::Speed: // Speed
            return getSpeedString( currentJob->getSpeedInBytesPerSec() );

        case Column::Url:
            return currentJob->getUrl() ;  //"https://download.test/file1.7z";

        default:
            return QVariant();
        }
    }

    return QVariant();
}

QVariant DownloadsTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case Column::RowNumber: return "#";
        case Column::FileName: return "Name";
        case Column::Size: return "Size";
        case Column::Completed: return "Completed";
        case Column::Progress: return "Progress";
        case Column::Speed: return "Speed";
        case Column::Url: return "Url";
        default: return QVariant();
        }
    }
    return QVariant();
}

QString DownloadsTableModel::getSpeedString(uint64_t SpeedInBytesPerSec) const
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
