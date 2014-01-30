#include <QtGui/QApplication>
#include "papatexturereader.h"


int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    papatexturereader foo;
    foo.show();
    return app.exec();
}
