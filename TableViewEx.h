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

#ifndef TABLEVIEWEX_H
#define TABLEVIEWEX_H

#include <QTableView>
#include <QKeyEvent>

class TableViewEx : public QTableView
{
    Q_OBJECT

public:

    explicit TableViewEx(QWidget *parent = nullptr) : QTableView(parent) {}

protected:

    void keyPressEvent(QKeyEvent *event) override;

Q_SIGNALS:

    void keyPressed( QKeyEvent *event );

};

#endif // TABLEVIEWEX_H
