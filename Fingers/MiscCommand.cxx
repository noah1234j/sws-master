#include "stdafx.h"

#include <cstdlib>
#include <cmath>

#include "MiscCommands.hxx"
#include "CommandHandler.h"
#include "RprItem.hxx"
#include "RprTake.hxx"
#include "RprMidiTake.hxx"
#include "FNG_Settings.h"
#include "TimeMap.h"
#include "RprStateChunk.hxx"

static bool RunMidiCommand(int commandId);

static void CreatePatternItem(int flag, void *data);
static void ActivateMediaExplorerShell(int flag, void *data);
static void ShowVersion(int flag, void *data);
static void SelectWindow(int flag, void *data);
static void EmulateMidiHardware(int flag, void *data);
static void GetEmulationSettings(int flag, void *data);
static void InsertMidiNote(int flag, void *data);
static void InsertMidiNoteSemitone(int flag, void *data);
static void SelectAllNearestEditCursor(int flag, void *data);
static void MoveMidiViewForwardABar(int flag, void *data);
static void MakeAllNotesSameVelocityAsSelected(int flag, void *data);

static void SelectMutedMidiNotes(int flag, void *data);
static void SelectNone(RprMidiTakePtr &midiTake);

static RprMidiNote *getFirstSelected(RprMidiTakePtr &midiTake);

static void QuantizeAllToGrid(int flag, void *data);

static bool convertToInProjectMidi(RprItemCtrPtr &ctr)
{
	bool hasMidiFile = false;
	for(int i = 0; i < ctr->size(); i++) {
		RprTake take(ctr->getAt(i).getActiveTake());
		if(!take.isMIDI())
			continue;
		if(take.isFile()) {
			hasMidiFile = true;
			break;
		}
	}
	if(hasMidiFile) {
		if(MessageBox(GetMainHwnd(),
			"Current selection has takes with MIDI files.\r\nTo apply this action these takes to be converted to in-project takes.\r\nDo you want to continue?",
			"Warning", MB_YESNO) == IDNO) {
			return false;
		}
		Main_OnCommandEx(40684, 0 , 0);
	}
	return true;
}


void MiscCommands::Init()
{
	RprCommand::registerCommand("SWS/FNG: Apply MIDI hardware emulation to selected midi takes", "FNG_MIDI_HW_EMULATION_APPLY", &EmulateMidiHardware, UNDO_STATE_ITEMS);
	RprCommand::registerCommand("SWS/FNG: MIDI hardware emulation settings", "FNG_MIDI_HW_EMULATION_SETTINGS", &GetEmulationSettings, NO_UNDO);
	RprCommand::registerCommand("SWS/FNG MIDI: select muted MIDI notes", "FNG_SELECT_MUTED", &SelectMutedMidiNotes, NO_UNDO);
	RprCommand::registerCommand("SWS/FNG: Quantize item positions and MIDI note positions to grid", "FNG_QUANTIZE_TO_GRID", &QuantizeAllToGrid, UNDO_STATE_ITEMS);
	RprCommand::registerCommand("SWS/FNG MIDI: select notes nearest edit cursor", "FNG_SELECT_NOTES_NEAR_EDIT_CURSOR", &SelectAllNearestEditCursor, UNDO_STATE_ITEMS);
}

static void SelectMutedMidiNotes(int flag, void *data)
{
	RprMidiTakePtr midiTake = RprMidiTake::createFromMidiEditor();
	for(int i = 0; i < midiTake->countNotes(); i++)
		midiTake->getNoteAt(i)->setSelected(midiTake->getNoteAt(i)->isMuted());
}

static bool RunMidiCommand(int commandId)
{
	return MIDIEditor_OnCommand(MIDIEditor_GetActive(), commandId);
}

static RprMidiNote *getFirstSelected(RprMidiTakePtr &midiTake)
{
	for(int i = 0; i < midiTake->countNotes(); i++) {
		if (midiTake->getNoteAt(i)->isSelected()) {
			return midiTake->getNoteAt(i);
		}
	}
}

static void SelectAllNearestEditCursor(int flag, void *data)
{
	double editCursor = GetCursorPosition();
	RprMidiTakePtr midiTake = RprMidiTake::createFromMidiEditor();

	if (midiTake->countNotes() == 0)
		return;

	int closest = midiTake->getNoteAt(0)->getItemPosition();
	double closestDifference = std::fabs(editCursor - midiTake->getNoteAt(0)->getPosition());
	for(int i = 1; i < midiTake->countNotes(); i++) {
		RprMidiNote *note = midiTake->getNoteAt(i);
		double difference = std::fabs(editCursor - note->getPosition());
		if (difference < closestDifference) {
			closest = note->getItemPosition();
			closestDifference = difference;
		}
	}

	for(int i = 0; i < midiTake->countNotes(); i++) {
		RprMidiNote *note = midiTake->getNoteAt(i);
		if (note->getItemPosition() == closest) {
			note->setSelected(true);
		} else {
			note->setSelected(false);
		}
	}
}



static void MakeAllNotesSameVelocityAsSelected(int flag, void *data)
{
	RprMidiTakePtr midiTake = RprMidiTake::createFromMidiEditor();
	RprMidiNote *note = getFirstSelected(midiTake);
	if (note == NULL)
		return;

	for(int i = 0; i < midiTake->countNotes(); i++) {
		midiTake->getNoteAt(i)->setVelocity(note->getVelocity());
	}
}

static void MakeAllNotesOfSamePitchSameVelocityAsSelected(int flag, void *data)
{
	RprMidiTakePtr midiTake = RprMidiTake::createFromMidiEditor();
	RprMidiNote *note = getFirstSelected(midiTake);
	if (note == NULL)
		return;

	for(int i = 0; i < midiTake->countNotes(); i++) {
		if (note->getPitch() == midiTake->getNoteAt(i)->getPitch())
			midiTake->getNoteAt(i)->setVelocity(note->getVelocity());
	}
}

static bool sortMidiPositions(const RprMidiNote *lhs, const RprMidiNote *rhs)
{
	if (lhs->getPosition() == rhs->getPosition())
		return lhs->getPitch() > rhs->getPitch();
	
	return lhs->getPosition() < rhs->getPosition();
}

static double getJitter(int jitter)
{
	if (jitter == 0)
		return 0.0;
	int jitterMs = (std::rand() % (2 * jitter)) - jitter;
	return (double)jitterMs / 1000000.0;
}

static void EmulateMidiHardware(int flag, void *data)
{
	std::list<RprMidiTake *> midiTakes;
	std::list<RprMidiNote *> midiNotes;
	double serialDelay = 0.001;
	std::string val = getReaperProperty("midihw_delay");
	if(!val.empty()) {
		serialDelay = ::atof(val.c_str()) / 1000.0;
	}

	int jitter = 1000;
	val = getReaperProperty("midihw_jitter");
	if(!val.empty()) {
		jitter = (int)(::atof(val.c_str()) * 1000.0 + 0.5);
	}
	
	RprItemCtrPtr itemCtr = RprItemCollec::getSelected();
	if(!convertToInProjectMidi(itemCtr))
		return;

	for(int i = 0; i < itemCtr->size(); ++i) {
		RprTake take = itemCtr->getAt(i).getActiveTake();
		if(!take.isMIDI())
			continue;
		if(take.isFile())
			continue;
		RprMidiTake *midiTake = new RprMidiTake(take);
		midiTakes.push_back(midiTake);
		for(int j = 0; j < midiTake->countNotes(); ++j) {
			midiNotes.push_back(midiTake->getNoteAt(j));			
		}
	 }
	midiNotes.sort(sortMidiPositions);

	std::list<RprMidiNote *>::iterator i = midiNotes.begin();
	if(i == midiNotes.end()) {
		for(std::list<RprMidiTake *>::iterator k = midiTakes.begin(); k != midiTakes.end(); ++k)
		delete *k;
		return;
	}

	double lastPosition = (*i)->getPosition();
	(*i)->setPosition((*i)->getPosition() + getJitter(jitter));
	++i;
	for(; i != midiNotes.end(); ++i) {
		double currentPosition = (*i)->getPosition();
		if(currentPosition < lastPosition)
			currentPosition = lastPosition;
		double diff = currentPosition - lastPosition;
		double newPos = currentPosition + getJitter(jitter);
		if(diff < serialDelay) {
			newPos += serialDelay;
			diff = serialDelay;
		}
		(*i)->setPosition(newPos);
		lastPosition += diff;
	}
	for(std::list<RprMidiTake *>::iterator k = midiTakes.begin(); k != midiTakes.end(); ++k)
		delete *k;
}

static void GetEmulationSettings(int flag, void *data)
{
	char returnStrings[512] = "1,1";
	std::string delay = getReaperProperty("midihw_delay");
	std::string jitter = getReaperProperty("midihw_jitter");
	if(!delay.empty() && !jitter.empty()) {
		std::string temp = delay + "," + jitter;
		::strcpy(returnStrings, temp.c_str());
	}
	if(GetUserInputs("MIDI hardware emulation", 2, "Serial delay (ms),Max jitter (ms)", returnStrings, 512)) {
		std::string results(returnStrings);
		setReaperProperty("midihw_delay", results.substr(0, results.find_first_of(',')));
		setReaperProperty("midihw_jitter", results.substr(results.find_first_of(',') + 1));
	}
}

double getQuantizedPosition(double pos, double gridSize)
{
	double beat = TimeToBeat(pos);
	double posMod = fmod(beat, gridSize);
	if(gridSize / 2 > posMod) {
		return BeatToTime(beat - posMod);
	} else {
		return BeatToTime(beat + gridSize - posMod);
	}
}

static void QuanitzeMidi(RprTake &take, double gridSize)
{
	RprMidiTake midiTake(take);
	for(int i = 0; i < midiTake.countNotes(); ++i) {
		midiTake.getNoteAt(i)->setPosition(getQuantizedPosition(midiTake.getNoteAt(i)->getPosition(), gridSize));		
	}
}

static void QuanitzeAudio(RprItem &item, double gridSize)
{
	item.setPosition(getQuantizedPosition(item.getPosition(), gridSize));
}

static void QuantizeAllToGrid(int flag, void *data)
{
	RprItemCtrPtr itemCtr = RprItemCollec::getSelected();
	double gridSize = *(double *)getInternalReaperProperty("projgriddiv");
	if(!convertToInProjectMidi(itemCtr))
		return;

	for(int i = 0; i < itemCtr->size(); ++i) {
		RprTake take = itemCtr->getAt(i).getActiveTake();
		if(take.isMIDI())
			QuanitzeMidi(take, gridSize);
		else
		{
			RprItem item = itemCtr->getAt(i); 
			QuanitzeAudio(item, gridSize);
		}
	 }

}

