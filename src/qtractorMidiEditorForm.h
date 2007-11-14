// qtractorMidiEditorForm.h
//
/****************************************************************************
   Copyright (C) 2005-2007, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qtractorMidiEditorForm_h
#define __qtractorMidiEditorForm_h

#include "ui_qtractorMidiEditorForm.h"


// Forward declarations...
class qtractorMidiEditor;
class qtractorMidiClip;
class qtractorMidiSequence;
class qtractorTimeScale;

class QContextMenuEvent;
class QActionGroup;
class QComboBox;
class QLabel;


//----------------------------------------------------------------------------
// qtractorMidiEditorForm -- UI wrapper form.

class qtractorMidiEditorForm : public QMainWindow
{
	Q_OBJECT

public:

	// Constructor.
	qtractorMidiEditorForm(QWidget *pParent = 0, Qt::WindowFlags wflags = 0);
	// Destructor.
	~qtractorMidiEditorForm();

	// MIDI editor widget accessor.
	qtractorMidiEditor *editor() const;

	// Local time-scale accessor.
	qtractorTimeScale *timeScale() const;

	// MIDI clip sequence accessors.
	void setMidiClip(qtractorMidiClip *pMidiClip);
	qtractorMidiClip *midiClip() const;

	// MIDI clip properties accessors.
	const QString& filename() const;
	unsigned short trackChannel() const;
	unsigned short format() const;

	qtractorMidiSequence *sequence() const;

	// Special executive setup method.
	void setup(qtractorMidiClip *pMidiClip = NULL);

	// Pre-close event handler.
	bool queryClose();

public slots:

	void fileSave();
	void fileSaveAs();
	void fileProperties();
	void fileClose();

	void editModeOn();
	void editModeOff();

	void editUndo();
	void editRedo();

	void editCut();
	void editCopy();
	void editPaste();
	void editDelete();

	void editSelectNone();
	void editSelectAll();
	void editSelectInvert();

	void toolsQuantize();
	void toolsTranspose();
	void toolsNormalize();
	void toolsRandomize();
	void toolsResize();

	void viewMenubar(bool bOn);
	void viewStatusbar(bool bOn);
	void viewToolbarFile(bool bOn);
	void viewToolbarEdit(bool bOn);
	void viewToolbarView(bool bOn);
	void viewToolbarTransport(bool bOn);
	void viewNoteDuration(bool bOn);
	void viewNoteColor(bool bOn);
	void viewValueColor(bool bOn);
	void viewPreview(bool bOn);
	void viewFollow(bool bOn);
	void viewRefresh();

	void helpAbout();
	void helpAboutQt();

	void sendNote(int iNote, int iVelocity);

	void selectionChanged(qtractorMidiEditor *);
	void contentsChanged(qtractorMidiEditor *);

	void stabilizeForm();

protected slots:

	// Event type selection slots.
	void viewTypeChanged(int);
	void eventTypeChanged(int);
	void controllerChanged(int);

	void snapPerBeatChanged(int iSnap);

protected:

	// On-open/close event handlers.
	void showEvent(QShowEvent *pShowEvent);
	void closeEvent(QCloseEvent *pCloseEvent);

	// Context menu request.
	void contextMenuEvent(QContextMenuEvent *pContextMenuEvent);

	// Save current clip track-channel sequence.
	bool saveClipFile(bool bPrompt);

private:

	// The Qt-designer UI struct...
	Ui::qtractorMidiEditorForm m_ui;

	// Instance variables...
	qtractorMidiEditor *m_pMidiEditor;

	unsigned long m_iClipLength;

	int m_iDirtyCount;

	// Edit-mode action group up.
	QActionGroup *m_pEditModeActionGroup;

	// Edit snap mode.
	QComboBox *m_pSnapPerBeatComboBox;

	// Event type selection widgets...
	QComboBox *m_pViewTypeComboBox;
	QComboBox *m_pEventTypeComboBox;
	QComboBox *m_pControllerComboBox;

	// Status items.
	QLabel *m_pTrackNameLabel;
	QLabel *m_pFileNameLabel;
	QLabel *m_pTrackChannelLabel;
	QLabel *m_pStatusModLabel;
	QLabel *m_pDurationLabel;
};


#endif	// __qtractorMidiEditorForm_h


// end of qtractorMidiEditorForm.h
