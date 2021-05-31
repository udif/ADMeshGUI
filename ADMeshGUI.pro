#-------------------------------------------------
#
# Project created by QtCreator 2014-10-31T11:28:53
#
# [Information about Qt + OpenGL classes (e.g. how to use the API) obtained from Qt Reference Pages
# http://doc.qt.io/qt-5/reference-overview.html 9. 5. 2015]
#
#-------------------------------------------------

QT       += core gui opengl widgets svg

lessThan(QT_MAJOR_VERSION, 5) {
   error(ADMeshGUI requires Qt 5.4 to run. Older version detected.)
}

equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 4) {
   error(ADMeshGUI requires Qt 5.4 to run. Older version detected.)
}

QMAKE_CXXFLAGS += $$(CXXFLAGS)
QMAKE_CFLAGS += $$(CFLAGS)
QMAKE_LFLAGS += $$(LDFLAGS)

TARGET = admeshgui
TEMPLATE = app

load(uic)
uic.commands += -tr _

SOURCES += main.cpp\
        window.cpp \    
    renderingwidget.cpp \
    admeshcontroller.cpp \
    meshobject.cpp \
    historylist.cpp \
    propertiesdialog.cpp \
    

HEADERS  += window.h \
    data.h \
    renderingwidget.h \
    admeshcontroller.h \
    meshobject.h \
    historylist.h \
    propertiesdialog.h \
    admeshEventFilter.h

FORMS    += window.ui \
    propertiesdialog.ui

LIBS += -ladmesh -lstlsplit -lpoly2tri -lstlcut
macx {
    LIBS += -lintl
    TARGET = ADMeshGUI
    ICON = Resources/admeshgui.icns
    QMAKE_INFO_PLIST = Distribution/Info.plist
    include(homebrew.pri)
    app.files += ADMesGUI.app
    app.path = /Applications
    INSTALLS += app
}

win32 {
    LIBS += -lintl -liconv -lopengl32 -ladmesh.dll
    RC_FILE = admeshgui.rc
}

unix {
    isEmpty(PREFIX):PREFIX = /usr
    bin.files += admeshgui
    bin.path = $$PREFIX/bin
    mainico.files += Resources/admeshgui.svg
    mainico.path = $$PREFIX/share/icons/hicolor/scalable/apps
    16ico.files += Distribution/16x16/admeshgui.png
    16ico.path = $$PREFIX/share/icons/hicolor/16x16/apps
    32ico.files += Distribution/32x32/admeshgui.png
    32ico.path = $$PREFIX/share/icons/hicolor/32x32/apps
    48ico.files += Distribution/48x48/admeshgui.png
    48ico.path = $$PREFIX/share/icons/hicolor/48x48/apps
    symbico.files += Distribution/symbolic/admeshgui-symbolic.svg
    symbico.path = $$PREFIX/share/icons/hicolor/symbolic/apps
    desktop.files += Distribution/admeshgui.desktop
    desktop.path = $$PREFIX/share/applications
    appdata.files += Distribution/admeshgui.appdata.xml
    appdata.path = $$PREFIX/share/appdata
    INSTALLS += bin desktop mainico 16ico 32ico 48ico symbico appdata
}

DISTFILES += \
    fshader.glsl \
    vshader.glsl

RESOURCES += \
    shaders.qrc \
    Resources.qrc

OTHER_FILES += \
    picking_vshader.glsl \
    picking_fshader.glsl \
    Info.plist \
    homebrew.pri \
    Distribution/admeshgui.ico \
    admeshgui.rc


