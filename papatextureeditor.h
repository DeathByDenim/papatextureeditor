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
public:
    PapaTextureEditor();
    virtual ~PapaTextureEditor();
	
public slots:
	void openDirectory();
	void importImage();
	void exportImage();
    void saveImage();
    void putMeInIt();
	void textureClicked(const QModelIndex& index);
};

#endif // papatextureeditor_H
