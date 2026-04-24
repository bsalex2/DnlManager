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

#ifndef DOWNLOADPROGRESSPAINTER_H
#define DOWNLOADPROGRESSPAINTER_H

#include <QStyledItemDelegate>

#include "DownloadManager.h"

class DownloadProgressPainter : public QStyledItemDelegate
{
public:

    DownloadProgressPainter( QObject *parent, CDownloadManagerShared DownloadManagerPtr ) : QStyledItemDelegate(parent), m_DownloadManagerPtr(DownloadManagerPtr) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        //QVariant value = index.data(Qt::DisplayRole);
        CDownloadJobShared currentJob = m_DownloadManagerPtr->getJob( index.row() );
        uint32_t progress = currentJob->getProgress();

        QColor rectColor = Qt::darkGray;
        bool bDrawStateName = true;
        QString stateText;

        auto State = currentJob->getState();

        if ( State == CDownloadJob::DownloadState::Completed )
        {
            rectColor = Qt::blue;
            stateText = "Completed";
        }
        else if ( State == CDownloadJob::DownloadState::Paused )
        {
            rectColor = Qt::darkGray;
            stateText = "Paused";

        }
        else if ( State == CDownloadJob::DownloadState::Error )
        {
            rectColor = Qt::darkGray;
            stateText = "Error: " + currentJob->getErrorString();
        }
        else
        {
            bDrawStateName = false;
            QStyledItemDelegate::paint(painter, option, index);

            QStyleOptionProgressBar progressBarOption;
            progressBarOption.rect = option.rect;
            progressBarOption.rect.adjust( 2, 2, -2, -2 );
            progressBarOption.minimum = 0;
            progressBarOption.maximum = 100;
            progressBarOption.progress = progress;
            progressBarOption.text = QString("%1%").arg(progress);
            progressBarOption.textVisible = true;
            progressBarOption.textAlignment = Qt::AlignVCenter | Qt::AlignHCenter;

            QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);
        }

        if ( bDrawStateName )
        {
            QRect rect = option.rect;
            rect.adjust( 2, 2, -2, -2 );
            painter->fillRect(rect, rectColor);
            painter->setPen(Qt::white);
            painter->drawText(rect, Qt::AlignCenter, stateText);
        }

    }

private:

    CDownloadManagerShared m_DownloadManagerPtr;
};


#endif // DOWNLOADPROGRESSPAINTER_H
