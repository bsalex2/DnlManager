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

#ifndef MAIN_H
#define MAIN_H

#include <QApplication>
#include <QNetworkProxy>

class MyApplication : public QApplication
{
public:

    MyApplication(int &argc, char **argv)
        : QApplication(argc, argv)
    {
        // Custom initialization
        qDebug() << "MyApplication started.";
        setApplicationName( "Download Manager" );
    }

    ~MyApplication()
    {
    }

    static MyApplication *instance() { return static_cast<MyApplication*>(QApplication::instance()); }

    void
    run( const QUrl &url, const QNetworkProxy &proxy );

private:

};


#endif // MAIN_H
