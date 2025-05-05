QT += core gui
QT += sql
QT += concurrent
QT += quick qml
QT += quickcontrols2

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    datafetcher.cpp \
    main.cpp

HEADERS += \
    datafetcher.h

# Removed:
# FORMS += mainwindow.ui
# SOURCES += mainwindow.cpp
# HEADERS += mainwindow.h

RESOURCES += \
    resource.qrc

DISTFILES += \
    qml/main.qml

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
