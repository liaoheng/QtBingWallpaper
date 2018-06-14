TARGET = bingwallpaperservice
TEMPLATE = app

CONFIG   += console qt
QT = core network

SOURCES = bingwallpaperservice.cpp \
    json.cpp

include(../qtservice/src/qtservice.pri)

HEADERS += \
    json.h
