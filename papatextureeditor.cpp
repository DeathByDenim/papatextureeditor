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
	quitAction->setShortcut(QKeySequence("Ctrl+Q"));
/*	QAction* saveAction = new QAction(this);
	saveAction->setText( "&Save" );
	saveAction->setShortcut(QKeySequence("Ctrl+S"));
*/	QAction* importAction = new QAction(this);
	importAction->setText( "&Import..." );
	importAction->setShortcut(QKeySequence("Ctrl+i"));
	QAction* exportAction = new QAction(this);
	exportAction->setText( "&Export..." );
	exportAction->setShortcut(QKeySequence("Ctrl+e"));
	QAction* openAction = new QAction(this);
	openAction->setText( "&Open directory..." );
	openAction->setShortcut(QKeySequence("Ctrl+O"));
	connect(quitAction, SIGNAL(triggered()), SLOT(close()) );
//	connect(saveAction, SIGNAL(triggered()), SLOT(saveImage()) );
	connect(importAction, SIGNAL(triggered()), SLOT(importImage()));
	connect(exportAction, SIGNAL(triggered()), SLOT(exportImage()));
	connect(openAction, SIGNAL(triggered()), SLOT(openDirectory()));
	QMenu *menu = menuBar()->addMenu("&File");
	menu->addAction(openAction);
	menu->addAction(importAction);
	menu->addAction(exportAction);
//	menu->addAction(saveAction);
	menu->addAction(quitAction);
	
	importAction->setEnabled(false);
}

PapaTextureEditor::~PapaTextureEditor()
{
}

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
	QSettings settings("DeathByDenim", "papatextureeditor");
	QString foldername = QFileDialog::getExistingDirectory(this, "Open texture folder", settings.value("texturedirectory").toString());
	if(Model && foldername.length() > 0)
	{
		settings.setValue("texturedirectory", QFileInfo(foldername).canonicalPath() + "/.");
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
		Label->setPixmap(QPixmap::fromImage(papa->image()));
		Label->setFixedSize(papa->image().size());

		InfoLabel->setText(Model->info(index));
	}
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

	QString filter;
	QList<QByteArray> supportedImageFormats = QImageWriter::supportedImageFormats();
	for(QList<QByteArray>::const_iterator format = supportedImageFormats.constBegin(); format != supportedImageFormats.constEnd(); ++format)
	{
		if(format != supportedImageFormats.constBegin())
			filter += ";;";
		filter += QString(*format).toLower() + " (*." + QString(*format).toLower() + ')';
	}

	QSettings settings("DeathByDenim", "papatextureeditor");
	QString filename = QFileDialog::getSaveFileName(this, "Save image", settings.value("exportdirectory").toString(), filter);

	if(filename.length() > 0)
		qDebug() << filename;

	settings.setValue("exportdirectory", QFileInfo(filename).canonicalPath());


	QImageWriter writer(filename);
	QByteArray format;
	format.append(QFileInfo(filename).suffix());
	writer.setFormat(format);
	writer.write(papa->image());
}

#include "papatextureeditor.moc"
