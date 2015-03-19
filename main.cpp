#include <QApplication>
#include <QDesktopWidget>

#include "window.h"
#include "data.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    setlocale(LC_NUMERIC,"C");

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    Window window;
    window.resize(window.sizeHint());
    int desktopArea = QApplication::desktop()->width() *
                     QApplication::desktop()->height();
    int widgetArea = window.width() * window.height();

    window.setWindowTitle("ADMesh GUI");

    if (((float)widgetArea / (float)desktopArea) < 0.75f)
        window.show();
    else
        window.showMaximized();

    for(int i=1;i<argc;i++){
        window.openByFilename(argv[i]);
    }
    return app.exec();
}
