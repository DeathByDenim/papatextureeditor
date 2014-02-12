/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  Jarno van der Kolk <jarno@jarno.ca>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QTextBrowser>
#include "helpdialog.h"

HelpDialog::HelpDialog(QWidget* parent)
 : QDialog(parent)
{
	setWindowTitle("Help");
	setBaseSize(200, 1000);

	QVBoxLayout *layout = new QVBoxLayout(this);
	setLayout(layout);
	
	QTextBrowser *helpTextEdit = new QTextBrowser(this);
	helpTextEdit->setReadOnly(true);
	helpTextEdit->setHtml(getHelp());
	helpTextEdit->setOpenLinks(true);
	helpTextEdit->setOpenExternalLinks(true);
	layout->addWidget(helpTextEdit);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, this);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	layout->addWidget(buttonBox);

	buttonBox->setFocus();
	
	resize(800, 800);
}

HelpDialog::~HelpDialog()
{
}

QString HelpDialog::getHelp()
{
	return
		"<html>"
		"<body>"
		"<h1>Usage</h1>"
		"<p>Use \"File\" -&gt; \"Open directory...\" to navigate to the directory containing the papa files. It will open all of the textures in the directory and display them in the list on the left. Clicking on them will show the texture on the left. Note that the editor ignores the papa files which only contain models. Use <a href=\"https://forums.uberent.com/threads/rel-blender-importer-exporter-for-papa-files-v0-5.47964/\">raevn's Blender plugin</a> for those.</p>"
		"<p>Use \"File\" -&gt; \"Import...\" to overwrite the currently selected texture with your own image. You still need to use \"File\" -&gt; \"Save\" to save it into the papa file. This menu item will only be available for textures in the A8R8G8B8 or X8R8G8B8 format for now, since encoding in DXT1 or DXT5 doesn't work yet. You can see the current encoding of the texture in the status bar at the bottom. Finally, the imported texture needs to have the same resolution as the original texture.</p>"
		"<p>Use \"File\" -&gt; \"Export...\" to export the currently selected texture. Many formats are supported, but PNG is to be preferred.</p>"
		"<p>Use \"File\" -&gt; \"Save...\" or \"Save as...\" to save the currently selected texture to the papa file.</p>"
		"<h1>More info</h1>"
		"<p>For more information, see the <a href=\"https://forums.uberent.com/threads/wip-papa-texture-editor.56309/\">Papa Texture Editor thread</a> in the Planetary Annihilation forums.</p>"
		"</body>"
		"</html>";
}

#include "helpdialog.moc"
