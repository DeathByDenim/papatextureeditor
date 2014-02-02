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
#include <QDebug>
#include "texturelistmodel.h"

PapaTextureEditor::PapaTextureEditor()
 : 	Image(NULL), Label(NULL), Model(NULL), TextureList(NULL)
{
	setMinimumSize(1000, 700);

	QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
	
	QScrollArea *scrollarea = new QScrollArea(this);
	Label = new QLabel(scrollarea);
	scrollarea->setWidget(Label);

	TextureList = new QTreeView(this);
	Model = new TextureListModel(this);
	TextureList->setModel(Model);
	TextureList->setRootIsDecorated(false);
	TextureList->setMaximumWidth(300);
	connect(TextureList, SIGNAL(activated(const QModelIndex &)), SLOT(textureClicked(const QModelIndex &)));

	splitter->addWidget(TextureList);
	splitter->addWidget(scrollarea);


	setCentralWidget( splitter );
	QAction* quitAction = new QAction(this);
	quitAction->setText( "&Quit" );
	quitAction->setShortcut(QKeySequence("Ctrl+Q"));
/*	QAction* saveAction = new QAction(this);
	saveAction->setText( "&Save" );
	saveAction->setShortcut(QKeySequence("Ctrl+S"));
*/	QAction* importAction = new QAction(this);
	importAction->setText( "&Import..." );
	importAction->setShortcut(QKeySequence("Ctrl+i"));
	QAction* openAction = new QAction(this);
	openAction->setText( "&Open directory..." );
	openAction->setShortcut(QKeySequence("Ctrl+O"));
	connect(quitAction, SIGNAL(triggered()), SLOT(close()) );
//	connect(saveAction, SIGNAL(triggered()), SLOT(saveImage()) );
	connect(importAction, SIGNAL(triggered()), SLOT(importImage()));
	connect(openAction, SIGNAL(triggered()), SLOT(openDirectory()));
	QMenu *menu = menuBar()->addMenu("&File");
	menu->addAction(openAction);
	menu->addAction(importAction);
//	menu->addAction(saveAction);
	menu->addAction(quitAction);
}

PapaTextureEditor::~PapaTextureEditor()
{
}
/*
void papatexturereader::displayImage()
{
	Image->fill(0);

	QFile papaFile("skybox_01_back.papa");
	if(!papaFile.open(QIODevice::ReadOnly))
		return;

	papaFile.seek(0x80);
	QByteArray data = papaFile.read(4*1024*1024);
	for(int i = 0; i < 4 * 1024 * 1024; i += 4)
	{
		int pixelindex = i / 4;
		      Image->setPixel(pixelindex / 1024, pixelindex % 1024, qRgba(data[i], data[i+1], data[i+2], data[i+3]));
	}
}
*/
void PapaTextureEditor::saveImage()
{
	QFile papaFile("skybox_01_back.papa");
	if(!papaFile.open(QIODevice::ReadWrite))
		return;

	papaFile.seek(0x80);
	QByteArray data(4*1024*1024, Qt::Uninitialized);
	for(int i = 0; i < 4 * 1024 * 1024; i += 4)
	{
		int pixelindex = i / 4;
		QRgb colour = Image->pixel(pixelindex / 1024, pixelindex % 1024);
		data[i] = (unsigned char)qRed(colour);
		data[i+1] = (unsigned char)qGreen(colour);
		data[i+2] = (unsigned char)qBlue(colour);
		data[i+3] = (unsigned char)qAlpha(colour);
	}
	papaFile.write(data);
}

void PapaTextureEditor::putMeInIt()
{
	QPainter p(Image);
	QImageReader *reader = new QImageReader("/home/jarno/Afbeeldingen/blackmage.png");
	QImage modNewIcon = reader->read();
	delete reader;
	p.setCompositionMode(QPainter::CompositionMode_SourceOver);
	QRect size(QPoint(512-75,512-75), modNewIcon.size());
	p.drawImage(size, modNewIcon);

	Label->setPixmap(QPixmap::fromImage(*Image));
}

void PapaTextureEditor::openDirectory()
{
	QString foldername = QFileDialog::getExistingDirectory(this, "Open texture folder", "~/.local");
	if(Model && foldername.length() > 0)
	{
		if(!Model->loadFromDirectory(foldername))
		{
			QMessageBox::critical(this, "I/O error", QString("Couldn't open \"%1\".").arg(foldername));
		}
	}
}

void PapaTextureEditor::textureClicked(const QModelIndex& index)
{
	Label->setPixmap(QPixmap::fromImage(Model->image(index)));
	Label->setMinimumSize(Model->image(index).size());
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

	QString filename = QFileDialog::getOpenFileName(this, "Open image", "~/Afbeeldingen", filter);
	if(TextureList && Model && filename.length() > 0)
	{
		if(Model->importImage(filename, TextureList->currentIndex()))
			textureClicked(TextureList->currentIndex());
		else
			QMessageBox::critical(this, "Import failed", "I can't tell you why it failed, but this might be it:\n\tThe import image must be the same resolution at the current texture.\n\tThe texture format must be RGBA. I haven't finshed the others yet.\n\tThe papa file was read-only.");
	}
}

#include "papatextureeditor.moc"
