// qtractorFiles.cpp
//
/****************************************************************************
   Copyright (C) 2005-2008, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#include "qtractorAbout.h"
#include "qtractorFiles.h"

#include "qtractorMainForm.h"

#include <QTabWidget>
#include <QHBoxLayout>
#include <QToolButton>

#include <QApplication>
#include <QClipboard>

#include <QContextMenuEvent>


//-------------------------------------------------------------------------
// qtractorFiles - File/Groups dockable window.
//

// Constructor.
qtractorFiles::qtractorFiles ( QWidget *pParent )
	: QDockWidget(pParent)
{
	// Surely a name is crucial (e.g.for storing geometry settings)
	QDockWidget::setObjectName("qtractorFiles");

	// Create file type selection tab widget.
	const QFont& font = QDockWidget::font();
	m_pTabWidget = new QTabWidget(this);
	m_pTabWidget->setFont(QFont(font.family(), font.pointSize() - 1));
	m_pTabWidget->setTabPosition(QTabWidget::South);
	// Create local tabs.
	m_pAudioListView = new qtractorAudioListView();
	m_pMidiListView  = new qtractorMidiListView();
	// Add respective...
	m_pTabWidget->addTab(m_pAudioListView, tr("Audio"));
	m_pTabWidget->addTab(m_pMidiListView, tr("MIDI"));
	// Icons...
	m_pTabWidget->setTabIcon(qtractorFiles::Audio, QIcon(":/icons/trackAudio.png"));
	m_pTabWidget->setTabIcon(qtractorFiles::Midi,  QIcon(":/icons/trackMidi.png"));
#if QT_VERSION >= 0x040201
	m_pTabWidget->setUsesScrollButtons(false);
#endif

	// Player button (initially disabled)...
	m_pPlayWidget = new QWidget(m_pTabWidget);
	m_pPlayLayout = new QHBoxLayout(/*m_pPlayWidget*/);
	m_pPlayLayout->setMargin(2);
	m_pPlayLayout->setSpacing(2);
	m_pPlayWidget->setLayout(m_pPlayLayout);

	m_iPlayUpdate = 0;
	m_pPlayButton = new QToolButton(m_pPlayWidget);
	m_pPlayButton->setIcon(QIcon(":/icons/transportPlay.png"));
	m_pPlayButton->setToolTip(tr("Play file"));
	m_pPlayButton->setCheckable(true);
	m_pPlayButton->setEnabled(false);
	m_pPlayLayout->addWidget(m_pPlayButton);
	m_pTabWidget->setCornerWidget(m_pPlayWidget, Qt::BottomRightCorner);

	// Common file list-view actions...
	m_pNewGroupAction = new QAction(
		QIcon(":/icons/itemGroup.png"), tr("New &Group"), this);
	m_pOpenFileAction = new QAction(
		QIcon(":/icons/itemFile.png"), tr("Add &Files..."), this);
	m_pCutItemAction = new QAction(
		QIcon(":/icons/editCut.png"), tr("Cu&t"), this);
	m_pCopyItemAction = new QAction(
		QIcon(":/icons/editCopy.png"), tr("&Copy"), this);
	m_pPasteItemAction = new QAction(
		QIcon(":/icons/editPaste.png"), tr("P&aste"), this);
	m_pRenameItemAction = new QAction(
		QIcon(":/icons/formEdit.png"), tr("R&ename"), this);
	m_pDeleteItemAction = new QAction(
		QIcon(":/icons/formRemove.png"), tr("&Delete"), this);
	m_pPlayItemAction = new QAction(
		QIcon(":/icons/transportPlay.png"), tr("Play"), this);
	m_pPlayItemAction->setCheckable(true);

	m_pNewGroupAction->setShortcut(tr("Ctrl+G"));
	m_pOpenFileAction->setShortcut(tr("Ctrl+F"));
//	m_pCutItemAction->setShortcut(tr("Ctrl+X"));
//	m_pCopyItemAction->setShortcut(tr("Ctrl+C"));
//	m_pPasteItemAction->setShortcut(tr("Ctrl+V"));
	m_pRenameItemAction->setShortcut(tr("Ctrl+E"));
	m_pDeleteItemAction->setShortcut(tr("Ctrl+D"));
//	m_pPlayItemAction->setShortcut(tr("Ctrl+P"));

	// Some actions surely need those
	// shortcuts firmly attached...
	QDockWidget::addAction(m_pNewGroupAction);
	QDockWidget::addAction(m_pOpenFileAction);
	QDockWidget::addAction(m_pCutItemAction);
	QDockWidget::addAction(m_pCopyItemAction);
	QDockWidget::addAction(m_pPasteItemAction);
	QDockWidget::addAction(m_pRenameItemAction);
	QDockWidget::addAction(m_pDeleteItemAction);
	QDockWidget::addAction(m_pPlayItemAction);

	// Prepare the dockable window stuff.
	QDockWidget::setWidget(m_pTabWidget);
	QDockWidget::setFeatures(QDockWidget::AllDockWidgetFeatures);
	QDockWidget::setAllowedAreas(
		Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	// Some specialties to this kind of dock window...
	QDockWidget::setMinimumWidth(120);

	// Finally set the default caption and tooltip.
	const QString& sCaption = tr("Files");
	QDockWidget::setWindowTitle(sCaption);
	QDockWidget::setWindowIcon(QIcon(":/icons/viewFiles.png"));
	QDockWidget::setToolTip(sCaption);

	// Make it initially stable...
	stabilizeSlot();

	// Child widgets signal/slots... 
	QObject::connect(m_pTabWidget,
		SIGNAL(currentChanged(int)),
		SLOT(stabilizeSlot()));
	QObject::connect(m_pAudioListView,
		SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		SLOT(stabilizeSlot()));
	QObject::connect(m_pMidiListView,
		SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		SLOT(stabilizeSlot()));
	QObject::connect(m_pNewGroupAction,
		SIGNAL(triggered(bool)),
		SLOT(newGroupSlot()));
	QObject::connect(m_pOpenFileAction,
		SIGNAL(triggered(bool)),
		SLOT(openFileSlot()));
	QObject::connect(m_pCutItemAction,
		SIGNAL(triggered(bool)),
		SLOT(cutItemSlot()));
	QObject::connect(m_pCopyItemAction,
		SIGNAL(triggered(bool)),
		SLOT(copyItemSlot()));
	QObject::connect(m_pPasteItemAction,
		SIGNAL(triggered(bool)),
		SLOT(pasteItemSlot()));
	QObject::connect(m_pRenameItemAction,
		SIGNAL(triggered(bool)),
		SLOT(renameItemSlot()));
	QObject::connect(m_pDeleteItemAction,
		SIGNAL(triggered(bool)),
		SLOT(deleteItemSlot()));
	QObject::connect(m_pPlayItemAction,
		SIGNAL(triggered(bool)),
		SLOT(playSlot(bool)));
	QObject::connect(m_pPlayButton,
		SIGNAL(toggled(bool)),
		SLOT(playSlot(bool)));

	QObject::connect(QApplication::clipboard(),
		SIGNAL(dataChanged()),
		SLOT(stabilizeSlot()));
}


// Destructor.
qtractorFiles::~qtractorFiles (void)
{
	delete m_pNewGroupAction;
	delete m_pOpenFileAction;
	delete m_pCutItemAction;
	delete m_pCopyItemAction;
	delete m_pPasteItemAction;
	delete m_pRenameItemAction;
	delete m_pDeleteItemAction;
	delete m_pPlayItemAction;

	// No need to delete child widgets, Qt does it all for us.
}


// Just about to notify main-window that we're closing.
void qtractorFiles::closeEvent ( QCloseEvent * /*pCloseEvent*/ )
{
	QDockWidget::hide();

	qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
	if (pMainForm)
		pMainForm->stabilizeForm();
}


// Audio file list view accessor.
qtractorAudioListView *qtractorFiles::audioListView (void) const
{
	return m_pAudioListView;
}


// MIDI file list view accessor.
qtractorMidiListView *qtractorFiles::midiListView (void) const
{
	return m_pMidiListView;
}


// Clear everything on sight.
void qtractorFiles::clear (void)
{
	m_pAudioListView->clear();
	m_pMidiListView->clear();

	setPlayState(false);
}


// Audio file addition convenience method.
void qtractorFiles::addAudioFile ( const QString& sFilename )
{
	m_pTabWidget->setCurrentIndex(qtractorFiles::Audio);
	m_pAudioListView->addFileItem(sFilename);
}


// MIDI file addition convenience method.
void qtractorFiles::addMidiFile ( const QString& sFilename )
{
	m_pTabWidget->setCurrentIndex(qtractorFiles::Midi);
	m_pMidiListView->addFileItem(sFilename);
}


// Audio file selection convenience method.
void qtractorFiles::selectAudioFile ( const QString& sFilename )
{
	m_pTabWidget->setCurrentIndex(qtractorFiles::Audio);
	m_pAudioListView->selectFileItem(sFilename);
}


// MIDI file selection convenience method.
void qtractorFiles::selectMidiFile ( const QString& sFilename,
	int iTrackChannel )
{
	m_pTabWidget->setCurrentIndex(qtractorFiles::Midi);
	m_pMidiListView->selectFileItem(sFilename, iTrackChannel);
}


// Audition/pre-listening player methods.
void qtractorFiles::setPlayState ( bool bOn )
{
	m_iPlayUpdate++;
	m_pPlayItemAction->setChecked(bOn);
	m_pPlayButton->setChecked(bOn);
	m_iPlayUpdate--;
}

bool qtractorFiles::isPlayState (void) const
{
	return m_pPlayItemAction->isChecked();
}


// Retrieve current selected file list view.
qtractorFileListView *qtractorFiles::currentFileListView (void) const
{
	switch (m_pTabWidget->currentIndex()) {
	case qtractorFiles::Audio:
		return m_pAudioListView;
	case qtractorFiles::Midi:
		return m_pMidiListView;
	default:
		return NULL;
	}
}


// Open and add a new file item below the current group one.
void qtractorFiles::openFileSlot (void)
{
	qtractorFileListView *pFileListView = currentFileListView();
	if (pFileListView)
		pFileListView->openFile();
}


// Add a new group item below the current one.
void qtractorFiles::newGroupSlot (void)
{
	qtractorFileListView *pFileListView = currentFileListView();
	if (pFileListView)
		pFileListView->newGroup();
}


// Cut current file item(s) to clipboard.
void qtractorFiles::cutItemSlot (void)
{
	qtractorFileListView *pFileListView = currentFileListView();
	if (pFileListView)
		pFileListView->copyItem(true);
}


// Copy current file item(s) to clipboard.
void qtractorFiles::copyItemSlot (void)
{
	qtractorFileListView *pFileListView = currentFileListView();
	if (pFileListView)
		pFileListView->copyItem(false);
}


// Paste file item(s) from clipboard.
void qtractorFiles::pasteItemSlot (void)
{
	qtractorFileListView *pFileListView = currentFileListView();
	if (pFileListView)
		pFileListView->pasteItem();
}


// Rename current group/file item.
void qtractorFiles::renameItemSlot (void)
{
	qtractorFileListView *pFileListView = currentFileListView();
	if (pFileListView)
		pFileListView->renameItem();
}


// Remove current group/file item.
void qtractorFiles::deleteItemSlot (void)
{
	qtractorFileListView *pFileListView = currentFileListView();
	if (pFileListView)
		pFileListView->deleteItem();	
}


// Current item change slots.
void qtractorFiles::stabilizeSlot (void)
{
	qtractorFileListView *pFileListView = currentFileListView();
	if (pFileListView) {
		QTreeWidgetItem *pItem = pFileListView->currentItem();
		m_pCutItemAction->setEnabled(
			pItem && pItem->type() == qtractorFileListView::FileItem);
		m_pCopyItemAction->setEnabled(
			pItem && pItem->type() == qtractorFileListView::FileItem);
		m_pPasteItemAction->setEnabled(
			(QApplication::clipboard()->mimeData())->hasUrls());
		m_pRenameItemAction->setEnabled(
			pItem && pItem->type() == qtractorFileListView::GroupItem);
		m_pDeleteItemAction->setEnabled(
			pItem && pItem->type() != qtractorFileListView::ChannelItem);
		bool bPlayEnabled = (
			pItem && pItem->type() == qtractorFileListView::FileItem
			&& m_pTabWidget->currentIndex() == qtractorFiles::Audio);
		m_pPlayItemAction->setEnabled(bPlayEnabled);
		m_pPlayButton->setEnabled(bPlayEnabled);
	}
}


// Audition/pre-listening player slot.
void qtractorFiles::playSlot ( bool bOn )
{
	if (m_iPlayUpdate > 0)
		return;

	setPlayState(bOn);

	qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	QString sFilename;
	if (bOn && m_pTabWidget->currentIndex() == qtractorFiles::Audio) {
		qtractorFileListItem *pFileItem = m_pAudioListView->currentFileItem();
		if (pFileItem)
			sFilename = pFileItem->path();
	}

	pMainForm->activateAudioFile(sFilename);
}


// Context menu request event handler.
void qtractorFiles::contextMenuEvent (
	QContextMenuEvent *pContextMenuEvent )
{
	QMenu menu(this);

	// Construct context menu.
	menu.addAction(m_pOpenFileAction);
	menu.addAction(m_pNewGroupAction);
	menu.addSeparator();
	menu.addAction(m_pCutItemAction);
	menu.addAction(m_pCopyItemAction);
	menu.addAction(m_pPasteItemAction);
	menu.addSeparator();
	menu.addAction(m_pRenameItemAction);
	menu.addAction(m_pDeleteItemAction);
	menu.addSeparator();
	menu.addAction(m_pPlayItemAction);

	menu.exec(pContextMenuEvent->globalPos());
}


// end of qtractorFiles.cpp
