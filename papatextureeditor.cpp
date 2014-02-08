#include "papatextureeditor.h"

#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QAction>
#include <QPainter>
#include <QImageReader>
#include <QSplitter>
#include <QScrollArea>
#include <QTreeView>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QVBoxLayout>
#include <qimagewriter.h>
#include <QDebug>
#include <QSettings>
#include "texturelistmodel.h"
#include "papafile.h"

#define VERSION "0.1"

PapaTextureEditor::PapaTextureEditor()
 : Image(NULL), Label(NULL), InfoLabel(NULL), Model(NULL), TextureList(NULL)
{
	setMinimumSize(1000, 700);

	QSplitter *horsplitter = new QSplitter(Qt::Horizontal, this);
	QWidget *rightSideWidget = new QWidget(this);

	QVBoxLayout *rightSideLayout = new QVBoxLayout(rightSideWidget);

	QScrollArea *scrollarea = new QScrollArea(rightSideWidget);
	Label = new QLabel(scrollarea);
	scrollarea->setWidget(Label);

	TextureList = new QTreeView(this);
	Model = new TextureListModel(this);
	TextureList->setModel(Model);
	TextureList->setRootIsDecorated(false);
	TextureList->setMaximumWidth(400);
	TextureList->setSelectionMode(QAbstractItemView::SingleSelection);
	connect(TextureList, SIGNAL(activated(const QModelIndex &)), SLOT(textureClicked(const QModelIndex &)));
	
	InfoLabel = new QLabel(rightSideWidget);
	InfoLabel->setText("Info");
//	InfoLabel->setFixedHeight(50);

	horsplitter->addWidget(TextureList);
	rightSideLayout->addWidget(scrollarea);
	rightSideLayout->addWidget(InfoLabel);
	
	horsplitter->addWidget(rightSideWidget);


	setCentralWidget( horsplitter );
	QAction* quitAction = new QAction(this);
	quitAction->setText( "&Quit" );
	quitAction->setShortcut(QKeySequence("Ctrl+q"));
	QAction* saveAction = new QAction(this);
	saveAction->setText( "&Save" );
	saveAction->setShortcut(QKeySequence("Ctrl+s"));
	QAction* saveAsAction = new QAction(this);
	saveAsAction->setText( "S&ave as..." );
	saveAsAction->setShortcut(QKeySequence("Ctrl+Shift+s"));
	ImportAction = new QAction(this);
	ImportAction->setText( "&Import..." );
	ImportAction->setShortcut(QKeySequence("Ctrl+i"));
	QAction* exportAction = new QAction(this);
	exportAction->setText( "&Export..." );
	exportAction->setShortcut(QKeySequence("Ctrl+e"));
	QAction* openAction = new QAction(this);
	openAction->setText( "&Open directory..." );
	openAction->setShortcut(QKeySequence("Ctrl+o"));
	connect(quitAction, SIGNAL(triggered()), SLOT(close()));
	connect(saveAction, SIGNAL(triggered()), SLOT(savePapa()));
	connect(saveAsAction, SIGNAL(triggered()), SLOT(saveAsPapa()));
	connect(ImportAction, SIGNAL(triggered()), SLOT(importImage()));
	connect(exportAction, SIGNAL(triggered()), SLOT(exportImage()));
	connect(openAction, SIGNAL(triggered()), SLOT(openDirectory()));
	QMenu *menu = menuBar()->addMenu("&File");
	menu->addAction(openAction);
	menu->addAction(ImportAction);
	menu->addAction(exportAction);
	menu->addAction(saveAction);
	menu->addAction(saveAsAction);
	menu->addAction(quitAction);

	QAction* aboutAction = new QAction(this);
	aboutAction->setText("&About...");
	connect(aboutAction, SIGNAL(triggered()), SLOT(about()));
	menuBar()->addMenu("&Help")->addAction(aboutAction);

	ImportAction->setEnabled(false);
}

PapaTextureEditor::~PapaTextureEditor()
{
}

void PapaTextureEditor::savePapa()
{
	if(!Model->savePapa(TextureList->currentIndex()))
		QMessageBox::critical(this, "Save failed", "Couldn't save file, reason: " + Model->lastError());
}

void PapaTextureEditor::saveAsPapa()
{
	QSettings settings("DeathByDenim", "papatextureeditor");
	QString filename = QFileDialog::getSaveFileName(this, "Save texture as papa", settings.value("saveasdirectory").toString(), "Papa files(*.papa)");
	if(Model && filename.length() > 0)
	{
		settings.setValue("saveasdirectory", QFileInfo(filename).canonicalPath());
		if(!Model->savePapa(TextureList->currentIndex(), filename))
			QMessageBox::critical(this, "Save failed", "Couldn't save file, reason: " + Model->lastError());
	}
}

void PapaTextureEditor::openDirectory()
{
	QSettings settings("DeathByDenim", "papatextureeditor");
	QString foldername = QFileDialog::getExistingDirectory(this, "Open texture folder", settings.value("texturedirectory").toString());
	if(Model && foldername.length() > 0)
	{
		settings.setValue("texturedirectory", QFileInfo(foldername).canonicalPath());
		if(!Model->loadFromDirectory(foldername))
		{
			QMessageBox::critical(this, "I/O error", QString("Couldn't open \"%1\".").arg(foldername));
		}
	}
}

void PapaTextureEditor::textureClicked(const QModelIndex& index)
{
	PapaFile *papa = Model->papa(index);
	if(papa)
	{
		const QImage *im = papa->image(0);
		if(im)
		{
			Label->setPixmap(QPixmap::fromImage((*im)));
			Label->setFixedSize((*im).size());
		}
		else
		{
			QPixmap p;
			Label->setPixmap(p);
		}

		InfoLabel->setText(Model->info(index));
		
		ImportAction->setEnabled(Model->info(index).contains("A8R8G8B8"));
	}
	else
		ImportAction->setEnabled(false);
}

void PapaTextureEditor::importImage()
{
	if(!TextureList->currentIndex().isValid())
		return;

	QString filter = "Image Files (";
	QList<QByteArray> supportedImageFormats = QImageReader::supportedImageFormats();
	for(QList<QByteArray>::const_iterator format = supportedImageFormats.constBegin(); format != supportedImageFormats.constEnd(); ++format)
	{
		if(format != supportedImageFormats.constBegin())
			filter += ' ';
		filter += "*." + QString(*format).toLower();
	}
	filter += ')';

	QSettings settings("DeathByDenim", "papatextureeditor");
	QString filename = QFileDialog::getOpenFileName(this, "Open image", settings.value("importdirectory").toString(), filter);
	if(TextureList && Model && filename.length() > 0)
	{
		settings.setValue("importdirectory", QFileInfo(filename).canonicalPath());
		if(Model->importImage(filename, TextureList->currentIndex()))
			textureClicked(TextureList->currentIndex());
		else
			QMessageBox::critical(this, "Import failed", "I won't tell you why it failed, but this might be it:\n\tThe import image must be the same resolution at the current texture.\n\tThe texture format must be RGBA. I haven't finshed the others yet.\n\tThe papa file was read-only.");
	}
}

void PapaTextureEditor::exportImage()
{
	if(!TextureList->currentIndex().isValid())
		return;

	PapaFile *papa = Model->papa(TextureList->currentIndex());
	if(!papa)
		return;
	
	const QImage *im = papa->image(0);
	if(!im)
		return;

	QString filter = "Portable Network Graphics (PNG)(*.png)";
	QList<QByteArray> supportedImageFormats = QImageWriter::supportedImageFormats();
	for(QList<QByteArray>::const_iterator format = supportedImageFormats.constBegin(); format != supportedImageFormats.constEnd(); ++format)
	{
		if((*format).toLower() == "png")
			continue;

		filter += ";;";
		filter += QString(*format).toLower() + "(*." + QString(*format).toLower() + ')';
	}

	QSettings settings("DeathByDenim", "papatextureeditor");
	QString filename = QFileDialog::getSaveFileName(this, "Save image", settings.value("exportdirectory").toString(), filter);
	if(filename.length() > 0)
	{
		settings.setValue("exportdirectory", QFileInfo(filename).canonicalPath());

		QImageWriter writer(filename);
		QByteArray format;
		format.append(QFileInfo(filename).suffix());
		writer.setFormat(format);
		writer.write((*im));
	}
}

void PapaTextureEditor::about()
{
	QMessageBox::information(this, "About", "Created by DeathByDenim\nVersion " VERSION);
}

#include "papatextureeditor.moc"
