QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    uploadtask.cpp \
    downloadtask.cpp \
    taskmanager.cpp \
    chunkuploadtask.cpp

HEADERS += \
    mainwindow.h \
    uploadtask.h \
    downloadtask.h \
    taskmanager.h \
    chunkuploadtask.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    FileTransferClient.pro.user
