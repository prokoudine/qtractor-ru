// qtractorMidiClip.cpp
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

#include "qtractorAbout.h"
#include "qtractorMidiClip.h"
#include "qtractorMidiEngine.h"

#include "qtractorSessionDocument.h"

#include "qtractorMidiEditor.h"
#include "qtractorMidiEditorForm.h"

#include "qtractorMainForm.h"

#include <QFileInfo>
#include <QPainter>


//----------------------------------------------------------------------
// class qtractorMidiClip -- Audio file/buffer clip.
//

// Constructor.
qtractorMidiClip::qtractorMidiClip ( qtractorTrack *pTrack )
	: qtractorClip(pTrack)
{
	m_pFile = NULL;
	m_pSeq  = new qtractorMidiSequence();

	m_iTrackChannel = 0;
	m_bSessionFlag  = false;

	m_pMidiEditorForm = NULL;

	m_bDirty = false;
}

// Copy constructor.
qtractorMidiClip::qtractorMidiClip ( const qtractorMidiClip& clip )
	: qtractorClip(clip.track())
{
	m_pFile = NULL;
	m_pSeq  = new qtractorMidiSequence();

	setFilename(clip.filename());
	setTrackChannel(clip.trackChannel());

	m_pMidiEditorForm = NULL;

	m_bDirty = false;
}


// Destructor.
qtractorMidiClip::~qtractorMidiClip (void)
{
	if (m_pMidiEditorForm)
		delete m_pMidiEditorForm;

	if (m_pSeq)
		delete m_pSeq;
	if (m_pFile)
		delete m_pFile;
}


// The main use method.
bool qtractorMidiClip::openMidiFile ( const QString& sFilename,
	int iTrackChannel, int iMode )
{
	if (m_pFile)
		delete m_pFile;
	
	// Create and open up the MIDI file...
	m_pFile = new qtractorMidiFile();
	if (!m_pFile->open(sFilename, iMode)) {
		delete m_pFile;
		m_pFile = NULL;
		return false;
	}

	// Open-process mode...
	return openMidiFile(m_pFile, iTrackChannel);
}


// Overloaded open method; reuse an already open MIDI file.
bool qtractorMidiClip::openMidiFile ( qtractorMidiFile *pFile,
	int iTrackChannel )
{
#ifdef CONFIG_DEBUG_0
	fprintf(stderr, "qtractorMidiClip::openMidiFile(\"%s\", %d, %d)\n",
		pFile->filename().toUtf8().constData(), iTrackChannel, pFile->mode());
#endif

	qtractorTrack *pTrack = track();
	if (pTrack == NULL)
		return false;

	qtractorSession *pSession = pTrack->session();
	if (pSession == NULL)
		return false;

	// Initialize event container...
	m_pSeq->clear();
	m_pSeq->setTicksPerBeat(pSession->ticksPerBeat());
	m_pSeq->setTimeOffset(pSession->tickFromFrame(clipOffset()));
	m_pSeq->setTimeLength(pSession->tickFromFrame(clipLength()));

	// Are we on a pre-writing status?
	if (pFile->mode() == qtractorMidiFile::Write) {
		// Set initial local properties...
		pFile->setTempo(pSession->tempo());
		pFile->setBeatsPerBar(pSession->beatsPerBar());
		// And initial clip name...
		m_pSeq->setName(QFileInfo(pFile->filename()).baseName());
		m_pSeq->setChannel(pTrack->midiChannel());
		// Nothing more as for writing...
	} else {
		// Read the event sequence in...
		if (!pFile->readTrack(m_pSeq, iTrackChannel))
			return false;
		// FIXME: On demand, set session time properties from MIDI file...
		if (m_bSessionFlag) {
			pSession->setTempo(pFile->tempo());
			pSession->setBeatsPerBar(pFile->beatsPerBar());
			pSession->updateTimeScale();
			// Reset session flag now.
			m_bSessionFlag = false;
		}
		// We should have events, otherwise this clip is of no use...
		//if (m_pSeq->events().count() < 1)
		//	return false;
	}

	// Clip name should be clear about it all.
	if (clipName().isEmpty())
		setClipName(m_pSeq->name());
	// Set local properties...
	setFilename(pFile->filename());
	setTrackChannel(iTrackChannel);

	// Default clip length will be whole sequence duration.
	if (clipLength() == 0 && m_pSeq->timeLength() > m_pSeq->timeOffset()) {
		setClipLength(pSession->frameFromTick(
			m_pSeq->timeLength() - m_pSeq->timeOffset()));
	}

	// Uh oh...
	m_playCursor.reset(m_pSeq);
	m_drawCursor.reset(m_pSeq);
	m_bDirty = false;

	return true;
}


// MIDI file properties accessors.
void qtractorMidiClip::setTrackChannel ( unsigned short iTrackChannel )
{
	m_iTrackChannel = iTrackChannel;
}

unsigned short qtractorMidiClip::trackChannel (void) const
{
	return m_iTrackChannel;
}


// (Meta)Session flag accessors.
void qtractorMidiClip::setSessionFlag ( bool bSessionFlag )
{
	m_bSessionFlag = bSessionFlag;
}

bool qtractorMidiClip::isSessionFlag (void) const
{
	return m_bSessionFlag;
}


// Sequence properties accessors.
qtractorMidiSequence *qtractorMidiClip::sequence (void) const
{
	return m_pSeq;
}

unsigned short qtractorMidiClip::channel (void) const
{
	return m_pSeq->channel();
}

int qtractorMidiClip::bank (void) const
{
	return m_pSeq->bank();
}

int qtractorMidiClip::program (void) const
{
	return m_pSeq->program();
}


// Intra-clip playback frame positioning.
void qtractorMidiClip::seek ( unsigned long iFrame )
{
#ifdef CONFIG_DEBUG_0
	fprintf(stderr, "qtractorMidiClip::seek(%p, %lu)\n", this, iFrame);
#endif

	qtractorTrack *pTrack = track();
	if (pTrack == NULL)
		return;
	qtractorSession *pSession = pTrack->session();
	if (pSession == NULL)
		return;

	// Seek for the nearest sequence event...
	m_playCursor.seek(m_pSeq, pSession->tickFromFrame(iFrame));
}


// Reset clip state.
void qtractorMidiClip::reset ( bool bLooping )
{
#ifdef CONFIG_DEBUG_0
	fprintf(stderr, "qtractorMidiClip::reset(%p, %d)\n", this, (int) bLooping);
#endif

	// Reset to the first sequence event...
	m_playCursor.reset(m_pSeq);

	// Take the time from loop-start?
	if (bLooping && clipLoopStart() < clipLoopEnd())
		seek(clipLoopStart());
}


// Loop positioning.
void qtractorMidiClip::set_loop ( unsigned long /* iLoopStart */,
	unsigned long /* iLoopEnd */ )
{
	// Do nothing?
}


// Clip close-commit (record specific)
void qtractorMidiClip::close ( bool bForce )
{
#ifdef CONFIG_DEBUG_0
	fprintf(stderr, "qtractorMidiClip::close(%p)\n", this, int(bForce));
#endif

	qtractorTrack *pTrack = track();
	if (pTrack == NULL)
		return;
	qtractorSession *pSession = pTrack->session();
	if (pSession == NULL)
		return;

	// Actual sequence closure...
	m_pSeq->close();

	// Commit the final clip length...
	if (clipLength() < 1)
		setClipLength(pSession->frameFromTick(m_pSeq->duration()));
	
	// Now's time to write the whole thing, maybe as a SMF format 1...
	bool bNewFile = (m_pFile && m_pFile->mode() == qtractorMidiFile::Write);
	if (bNewFile && clipLength() > 0) {
		m_pFile->writeHeader(1, 2, m_pSeq->ticksPerBeat());
		m_pFile->writeTrack(NULL);   // Setup track.
		m_pFile->writeTrack(m_pSeq); // Channel track.
		m_pFile->close();
	}

	// Get rid of owned allocations...
	if (m_pFile) {
		delete m_pFile;
		m_pFile = NULL;
	}

	m_pSeq->clear();

	// If proven empty, remove the file.
	if (bForce && bNewFile && clipLength() < 1)
		QFile::remove(filename());

	// Get rid of editor form, if any.
	if (m_pMidiEditorForm)
		delete m_pMidiEditorForm;
	m_pMidiEditorForm = NULL;
}


// MIDI clip (re)open method.
void qtractorMidiClip::open (void)
{
	openMidiFile(filename(), m_iTrackChannel);
}


// Audio clip special process cycle executive.
void qtractorMidiClip::process ( unsigned long iFrameStart,
	unsigned long iFrameEnd )
{
#ifdef CONFIG_DEBUG_0
	fprintf(stderr, "qtractorMidiClip::process(%p, %lu, %lu)\n",
		this, iFrameStart, iFrameEnd);
#endif

	qtractorTrack *pTrack = track();
	if (pTrack == NULL)
		return;
	qtractorSession *pSession = pTrack->session();
	if (pSession == NULL)
		return;

	// Track mute state...
	bool bMute = (pTrack->isMute()
		|| (pSession->soloTracks() && !pTrack->isSolo()));

	unsigned long iTimeClip  = pSession->tickFromFrame(clipStart());
	unsigned long iTimeStart = pSession->tickFromFrame(iFrameStart);
	unsigned long iTimeEnd   = pSession->tickFromFrame(iFrameEnd);

	// Set precise event cursory positioning...
	unsigned long iTimeSeek = 0;
	if (iTimeStart > iTimeClip)
		iTimeSeek = iTimeStart - iTimeClip;

	// Enqueue the requested events...
	qtractorMidiEvent *pEvent = m_playCursor.seek(m_pSeq, iTimeSeek);
	while (pEvent) {
		unsigned long iTimeEvent = iTimeClip + pEvent->time();
		if (iTimeEvent > iTimeEnd)
			break;
		if (iTimeEvent >= iTimeStart
			&& (!bMute || pEvent->type() != qtractorMidiEvent::NOTEON))
			pSession->midiEngine()->enqueue(pTrack, pEvent, iTimeEvent,
				gain(pSession->frameFromTick(pEvent->time())));
		pEvent = pEvent->next();
	}
}


// Audio clip paint method.
void qtractorMidiClip::drawClip ( QPainter *pPainter, const QRect& clipRect,
	unsigned long iClipOffset )
{
	// Fill clip background...
	qtractorClip::drawClip(pPainter, clipRect, iClipOffset);

	qtractorSession *pSession = track()->session();
	if (pSession == NULL)
		return;

	// Check maximum note span...
	int iNoteSpan = (m_pSeq->noteMax() - m_pSeq->noteMin()) + 1;
	if (iNoteSpan < 4)
		iNoteSpan = 4;

	unsigned long iTimeStart = pSession->tickFromFrame(iClipOffset);
	unsigned long iTimeEnd
		= iTimeStart + pSession->tickFromPixel(clipRect.width());

	const QColor fg = track()->foreground();
	pPainter->setPen(fg);
	pPainter->setBrush(fg.light());

	int cx = pSession->pixelFromTick(iTimeStart);
	int h1 = clipRect.height() - 4;
	int h  = h1 / iNoteSpan;
	if (h < 4) h = 4;

	qtractorMidiEvent *pEvent = m_drawCursor.reset(m_pSeq, iTimeStart);
	while (pEvent && pEvent->time() < iTimeEnd) {
		if (pEvent->type() == qtractorMidiEvent::NOTEON
			&& pEvent->time() + pEvent->duration() >= iTimeStart) {
			int x = clipRect.x()
				+ pSession->pixelFromTick(pEvent->time()) - cx;
			int y = clipRect.bottom()
				- (h1 * (pEvent->note() - m_pSeq->noteMin() + 1)) / iNoteSpan;
			int w = pSession->pixelFromTick(pEvent->duration());
			if (h > 4) {
				if (w < 5) w = 5;
				pPainter->fillRect(x, y, w, h - 1, fg);
				pPainter->fillRect(x + 1, y + 1, w - 4, h - 4, fg.light());
			} else {
				if (w < 3) w = 3;
				pPainter->drawRect(x, y, w, h - 1);
			}
		}
		pEvent = pEvent->next();
	}
}


// Clip editor method.
bool qtractorMidiClip::startEditor ( QWidget *pParent )
{
	qtractorTrack *pTrack = track();
	if (pTrack == NULL)
		return false;

	if (m_pMidiEditorForm == NULL) {
		// Build up the editor form...
		m_pMidiEditorForm = new qtractorMidiEditorForm(pParent);
		m_pMidiEditorForm->setMidiClip(this);
		m_pMidiEditorForm->setForeground(pTrack->foreground());
		m_pMidiEditorForm->setBackground(pTrack->background());
		// Setup connections to main widget...
		qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
		if (pMainForm) {
			QObject::connect(m_pMidiEditorForm->editor(),
				SIGNAL(changeNotifySignal()),
				pMainForm, SLOT(changeNotifySlot()));
		}
	}

	// Show up the editor form...
	m_pMidiEditorForm->show();
	m_pMidiEditorForm->raise();
	m_pMidiEditorForm->activateWindow();

	return true;
}


// Local dirty flag.
void qtractorMidiClip::setDirty ( bool bDirty )
{
	m_bDirty = bDirty;
}

bool qtractorMidiClip::isDirty (void) const
{
	return m_bDirty;
}


// Virtual document element methods.
bool qtractorMidiClip::loadClipElement (
	qtractorSessionDocument * /* pDocument */, QDomElement *pElement )
{
	// Load track children...
	for (QDomNode nChild = pElement->firstChild();
			!nChild.isNull();
				nChild = nChild.nextSibling()) {
		// Convert node to element...
		QDomElement eChild = nChild.toElement();
		if (eChild.isNull())
			continue;
		// Load track state..
		if (eChild.tagName() == "filename")
			qtractorMidiClip::setFilename(eChild.text());
		else if (eChild.tagName() == "track-channel")
			qtractorMidiClip::setTrackChannel(eChild.text().toUShort());
	}

	return true;
}


bool qtractorMidiClip::saveClipElement (
	qtractorSessionDocument *pDocument, QDomElement *pElement )
{
	// Trap dirty clips...
	if (qtractorMidiClip::isDirty()) {
		// We'll need the correct time-scale...
		qtractorSession *pSession = NULL;
		qtractorTrack *pTrack = qtractorMidiClip::track();
		if (pTrack)
			pSession = pTrack->session();
		if (pSession) {
			// Have a new filename revision...
			const QString& sFilename
				= qtractorMidiEditor::createFilenameRevision(
					qtractorMidiClip::filename());
			// Save/replace the clip track...
			qtractorMidiEditor::saveCopyFile(sFilename,
				qtractorMidiClip::filename(), 
				qtractorMidiClip::trackChannel(),
				qtractorMidiClip::sequence(),
				pSession->timeScale());
			// Pre-commit dirty changes...
			qtractorMidiClip::setFilename(sFilename);
			qtractorMidiClip::setDirty(false);
		}
	}

	QDomElement eMidiClip = pDocument->document()->createElement("midi-clip");
	pDocument->saveTextElement("filename", qtractorMidiClip::filename(), &eMidiClip);
	pDocument->saveTextElement("track-channel",
		QString::number(qtractorMidiClip::trackChannel()), &eMidiClip);
	pElement->appendChild(eMidiClip);

	return true;
}


// end of qtractorMidiClip.cpp
