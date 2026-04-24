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

#include "Main.h"

#include <QApplication>
#include <QNetworkProxy>
#include <QSharedMemory>
#include <QMessageBox>

#include "MainWindow.h"

void MyApplication::run(const QUrl &url, const QNetworkProxy &proxy)
{
    Q_UNUSED(url);
    Q_UNUSED(proxy);

}


int main(int argc, char *argv[])
{
    MyApplication app(argc, argv);

    QSharedMemory sharedMemory("DnlManager");

    if ( !sharedMemory.create(1) )
    {
        QMessageBox::warning( nullptr,  app.applicationName(), "Another instance of this application is already running." );
        return 1;
    }

    MainWindow mainWidget;

    mainWidget.show();

    return app.exec();
}

