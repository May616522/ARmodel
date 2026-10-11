QT += core gui printsupport
QT += charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += cxx20
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    carfunction.cpp \
    main.cpp \
    mainwindow.cpp \
    qcustomplot.cpp

HEADERS += \
    carfunction.h \
    mainwindow.h \
    qcustomplot.h

FORMS += \
    mainwindow.ui


INCLUDEPATH += D:\vs_cpp\eigen-3.3.8\eigen-3.3.8
INCLUDEPATH += D:\vc\boost_1_90_0

include($$PWD/QXlsx/QXlsx.pri)
INCLUDEPATH += $$PWD/QXlsx

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
