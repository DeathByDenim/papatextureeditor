#include <QtGui/QApplication>
#include <QDir>
#include "papatextureeditor.h"


int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    PapaTextureEditor foo;
	foo.setWindowTitle("PAPA Texture Editor");
    foo.show();
	if(app.arguments().count() > 1)
	{
		QDir openmedir(app.arguments()[1]);
		if(openmedir.exists())
		{
			openmedir.makeAbsolute();
			foo.openDir(openmedir);
		}
	}
    return app.exec();
}
