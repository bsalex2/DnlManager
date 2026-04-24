TEMPLATE = app

QT += core network gui widgets sql

OUT_BASE = output_$${QMAKE_HOST.os}

OBJECTS_DIR = $${OUT_BASE}/obj
MOC_DIR     = $${OUT_BASE}/moc
RCC_DIR     = $${OUT_BASE}/rcc
UI_DIR      = $${OUT_BASE}/ui
DESTDIR     = $${OUT_BASE}/

QMAKE_CXXFLAGS += -std=c++23

SOURCES += \
           DlgDownloadAdd.cpp \
           DownloadDatabase.cpp \
           DownloadsTableModel.cpp \
           HttpRequestInfo.cpp \
           Main.cpp \
           MainWindow.cpp \
           DownloadManager.cpp \
           TableViewEx.cpp

HEADERS += \
    DownloadDatabase.h \
    DownloadProgressPainter.h \
    DownloadsTableModel.h \
    HttpRequestInfo.h \
    Main.h \
    MainWindow.h \
    TableViewEx.h \
    DlgDownloadAdd.h \
    DownloadManager.h

