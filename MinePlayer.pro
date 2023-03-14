QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimedia

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    audiodecoder.cpp \
    audioplayback.cpp \
    basetime.cpp \
    main.cpp \
    mainwindow.cpp \
    playengine.cpp \
    reader.cpp \
    renderview.cpp \
    utils.cpp \
    videodecoder.cpp \
    videorender.cpp

HEADERS += \
    audiodecoder.h \
    audioplayback.h \
    basetime.h \
    mainwindow.h \
    playengine.h \
    reader.h \
    renderview.h \
    utils.h \
    videodecoder.h \
    videorender.h

FORMS += \
    mainwindow.ui \
    renderview.ui

INCLUDEPATH += \
    "$${PWD}/dep/ffmpeg/include"

FFMPEG_LIB_PATH = "$${PWD}/dep/ffmpeg/lib"

LIBS += \
    $${FFMPEG_LIB_PATH}/avcodec.lib \
    $${FFMPEG_LIB_PATH}/avdevice.lib \
    $${FFMPEG_LIB_PATH}/avfilter.lib \
    $${FFMPEG_LIB_PATH}/avformat.lib \
    $${FFMPEG_LIB_PATH}/avutil.lib \
    $${FFMPEG_LIB_PATH}/swresample.lib \
    $${FFMPEG_LIB_PATH}/swscale.lib

#PRE_TARGETDEPS += \
#    $${FFMPEG_LIB_PATH}/avcodec.lib \
#    $${FFMPEG_LIB_PATH}/avdevice.lib \
#    $${FFMPEG_LIB_PATH}/avfilter.lib \
#    $${FFMPEG_LIB_PATH}/avformat.lib \
#    $${FFMPEG_LIB_PATH}/avutil.lib \
#    $${FFMPEG_LIB_PATH}/swresample.lib \
#    $${FFMPEG_LIB_PATH}/swscale.lib

#LIBS += \
#    -L$${FFMPEG_LIB_PATH} -lavcodec -lavdevice -lavfilter -lavcodec -lavcodec -lavcodec -lavcodec
#    -l$${FFMPEG_LIB_PATH}/avcodec \
#    -l$${FFMPEG_LIB_PATH}/avdevice \
#    -l$${FFMPEG_LIB_PATH}/avfilter \
#    -l$${FFMPEG_LIB_PATH}/avformat \
#    -l$${FFMPEG_LIB_PATH}/avutil \
#    -l$${FFMPEG_LIB_PATH}/swresample \
#    -l$${FFMPEG_LIB_PATH}/swscale

#PRE_TARGETDEPS += \
#    -L$${FFMPEG_LIB_PATH} -lavcodec -lavdevice -lavfilter -lavcodec -lavcodec -lavcodec -lavcodec

win32 {
    LIBS += \
        D3D11.lib \
        D3DCompiler.lib
}



# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    shader.hlsl
