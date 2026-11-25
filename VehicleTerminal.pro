TEMPLATE = app
TARGET = VehicleTerminal
QT += core gui widgets network concurrent multimedia multimediawidgets dbus
CONFIG += c++11 warn_on
INCLUDEPATH += $$PWD
DEFINES += QT_DEPRECATED_WARNINGS
win32-g++: QMAKE_CXXFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8
unix: LIBS += -lpthread -lm

SOURCES += \
main.cpp \
    mainwindow.cpp \
    settingwindow.cpp \
    onevideo.cpp \
    dht11.cpp \
    speechrecognition.cpp \
    Core/appconfig.cpp \
    Core/commandrunner.cpp \
    Core/downloadservice.cpp \
    Core/gnssservice.cpp \
    Core/medialibrary.cpp \
    Core/speechservice.cpp \
    Core/systemstatus.cpp \
    Core/timeservice.cpp \
    Core/weatherservice.cpp \
    Core/wifiservice.cpp \
    Ui/uikit.cpp \
    Map/baidumap.cpp \
    Monitor/ap3216.cpp \
    Monitor/capture_thread.cpp \
    Monitor/monitor.cpp \
    Music/musicplayer.cpp \
    Music/searchmusic.cpp \
    Weather/weather.cpp \
    Map/gps.c

HEADERS +=  \
    Core/appconfig.h \
    Core/commandrunner.h \
    Core/downloadservice.h \
    Core/gnssservice.h \
    Core/medialibrary.h \
    Core/network.h \
    Core/speechservice.h \
    Core/systemstatus.h \
    Core/timeservice.h \
    Core/weatherservice.h \
    Core/wifiservice.h \
    dht11.h \
    mainwindow.h \
    Map/baidumap.h \
    Map/gps.h \
    Monitor/ap3216.h \
    Monitor/capture_thread.h \
    Monitor/monitor.h \
    Music/musicplayer.h \
    Music/searchmusic.h \
    onevideo.h \
    settingwindow.h \
    speechrecognition.h \
    Ui/uikit.h \
    Weather/weather.h

RESOURCES += img.qrc

unix:!android {
    target.path = /opt/vehicle-terminal
    INSTALLS += target
}
