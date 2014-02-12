#ifndef papatextureeditor_H
#define papatextureeditor_H

#include <QtGui/QMainWindow>

class TextureListModel;
class QLabel;
class QModelIndex;
class QTreeView;

class PapaTextureEditor : public QMainWindow
{
Q_OBJECT

private:
	QImage *Image;
	QLabel *Label;
	TextureListModel* Model;
	QTreeView* TextureList;
	QLabel* InfoLabel;
	QAction* ImportAction;
	QAction* SaveAction;
	QAction* SaveAsAction;
    QAction* ExportAction;
public:
	PapaTextureEditor();
	virtual ~PapaTextureEditor();
	
public slots:
	void openDirectory();
	void importImage();
	void exportImage();
	void savePapa();
	void saveAsPapa();
	void textureClicked(const QModelIndex& index);
	void about();
	void help();
};

#endif // papatextureeditor_H
