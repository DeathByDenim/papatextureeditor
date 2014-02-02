#include <QtGui/QApplication>
#include "papatextureeditor.h"


int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    PapaTextureEditor foo;
    foo.show();
    return app.exec();
}
