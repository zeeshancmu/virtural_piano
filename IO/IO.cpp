#include "IO.h"

void overallPiano::checkModeChanged(void) {
	// Check mouse options to change modes

	int leftButton, middleButton, rightButton, mouseX, mouseY;
	int mouseEvent;
	int cX, cY, radius;
	float distance;

	modeChanged = false;

	mouseEvent = FsGetMouseEvent(leftButton, middleButton, rightButton, mouseX, mouseY);
	if (mouseEvent == FSMOUSEEVENT_LBUTTONDOWN) {
		// Upon left click, check if its in a valid mode-changing region
		for (int checkingMode = 0; checkingMode < qtyModes; checkingMode++) {
			getModeButtonPosition(checkingMode, cX, cY, radius);
			distance = (float)((cX - mouseX)*(cX - mouseX)) + (float)((cY - mouseY)*(cY - mouseY));

			if (distance < radius*radius) {
				// if it's inside a valid region, change the mode
				setCurrentMode(checkingMode);
			}
		}
	}
}

void overallPiano::checkKeyStates(void) {
	/*
	Check keyboard

	Will temporarily record the old value for each key.
	If the value changes, it will record that in notesPlaying
	so audio can play the note
	*/

	bool keyWasDepressed;

	for (int i = 0; i < 15; i++) {
		keyWasDepressed = userKeyDepressed[i];								// record old state
		userKeyDepressed[i] = FsGetKeyState(orderOfComputerKeys[i]);		// Get key state: 1=pressed, 0=unpressed
		userKeyFirstPressed[i] = (!keyWasDepressed && userKeyDepressed[i]);	// set FirstPressed boolean
	}
}

void overallPiano::setCurrentMode(int newMode)
{
	modeChanged = true;
	currentMode = newMode;
}

void overallPiano::resetSongNotes(void)
{
	std::fill_n(songNotes, 15, 0);
}

void overallPiano::resetUserNotes(void)
{
	std::fill_n(userKeyDepressed, 15, 0);
	std::fill_n(userKeyFirstPressed, 15, 0);
}

int overallPiano::checkValidNote(std::string noteToCheck)
{
	// Checks if a string passed in is a valid note for the piano.
	// If so, it will return the position on the piano (i.e., the index of the note)

	// Look for the noteToCheck within the full array of piano notes available (i.e. orderOfPianoKeys)
	std::vector<std::string>::iterator i = std::find(orderOfPianoKeys.begin(), orderOfPianoKeys.end(), noteToCheck);
	if (i != orderOfPianoKeys.end()) {					// if noteToCheck exists within the array
		return distance(orderOfPianoKeys.begin(), i);	//		return its index
	}
	else {												// Otherwise
		return -1;										//		return -1
	}
}

int overallPiano::getCurrentMode(void)
{
	return currentMode;
}

bool overallPiano::didModeChange(void)
{
	return modeChanged;
}

bool overallPiano::didUserESC(void)
{
	return userESC;
}

int overallPiano::howManyModes(void)
{
	return qtyModes;
}

bool overallPiano::didUserMakeInput(void)
{

	bool oldPressed[15], oldFirstPressed[15];

	std::memcpy(oldPressed, userKeyDepressed, 15);
	std::memcpy(oldFirstPressed, userKeyFirstPressed, 15);

	getUserInput();

	for (int i = 0; i < 15; i++) {
		if (userKeyFirstPressed[i] != oldFirstPressed[i]) {
			return true;
		}
	}
	
	std::memcpy(userKeyFirstPressed, oldFirstPressed, 15);
	
	return modeChanged || userESC;
}

void overallPiano::clearModeFlagChange(void)
{
	modeChanged = false;
}

bool overallPiano::readSong(std::ifstream &songToRead)
{
	// Read next line of song
	// Returns:
	//	true if no errors (has set songNotes array)
	//	false if error reading next line (generally end of file)
	int position;
	char peek;
	std::string nextLine;

	resetSongNotes();				// reset songNotes array

	if (!songToRead.eof())			// If it hasn't gotten to the end of the file
		peek = songToRead.peek();	//		Look at the next character
	else							// If it has reach EoF
		return false;				//		return false

	if (peek != '[') {								// If the next character is not the multi-note indicator
		std::getline(songToRead, nextLine, ' ');	//	Pull characters until the next space
		position = checkValidNote(nextLine);		//	Check that string to make sure it's a valid note
		if (position != -1) {						//	If the note is valid
			songNotes[position] = true;				//		Set the songNotes array appropriately
		}
	}
	else {											// Otherwise, the next character should be the multi-note indicator
		std::getline(songToRead, nextLine, ']');	//	Pull characters until the close of the multi-note indicator (i.e., ']')
		nextLine = nextLine.substr(1, nextLine.length()) + ' ';	// remove leading '[' and add trailing space to enforce trailing space after each note

		std::size_t location = nextLine.find(' ');	// Find the first note's ending space
		while (location <= nextLine.length()) {		// For each valid note to be played simultaneously
			position = checkValidNote(nextLine.substr(0, location));	// Check note
			if (position != -1) {										// Set the songNotes array
				songNotes[position] = true;
			}
			nextLine = nextLine.substr(location+1, nextLine.length());	// Remove the note from the read line
			location = nextLine.find(' ');								// Look for subsequent notes
		}
		//std::getline(songToRead, nextLine, ' ');
		songToRead.get();
	}
	return true;
}

void overallPiano::getUserInput(void)
{
	// Get all user inputs

	checkKeyStates();	// Check keyboard
	checkModeChanged();	// Check mouse

	// break early?
	FsPollDevice();
	if (FSKEY_ESC == FsInkey())
		userESC = true;
}

void overallPiano::getModeButtonPosition(const int mode, int & centerX, int & centerY, int & radius)
{
	centerX = modeButtonX[mode];
	centerY = modeButtonY[mode];
	radius = modeButtonRadius;
}

void overallPiano::drawBasicPiano(void)
{
	int cX, cY, r;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glColor3ub(0, 0, 0);
	for (int modeToDraw = 0; modeToDraw < howManyModes(); modeToDraw++) {
		getModeButtonPosition(modeToDraw, cX, cY, r);
		drawCircle_testing(cX, cY, r, true);
	}

	FsSwapBuffers();
}

void overallPiano::songNextNotes(bool whereToStore[15])
{
	// deep copy songNotes to whereToStore
	std::memcpy(whereToStore, songNotes, 15);
}

void overallPiano::userNotesToPlay(bool whereToStore[15])
{
	// deep copy userKeyFirstPressed to whereToStore
	std::memcpy(whereToStore, userKeyFirstPressed, 15);
}

void overallPiano::userNotesToDisplay(bool whereToStore[15])
{
	// deep copy userKeyDepressed to whereToStore
	std::memcpy(whereToStore, userKeyDepressed, 15);
}

void overallPiano::printUsersInput(void)
{
	int i;

	for (i = 0; i < 15; i++) {
		std::cout << userKeyDepressed[i];
	}
	std::cout << std::endl;

	for (i = 0; i < 15; i++) {
		if (userKeyFirstPressed[i])
			std::cout << "*";
		else
			std::cout << " ";
	}
	std::cout << std::endl;
}

void drawCircle_testing(int cx, int cy, int rad, bool fill)
{
	const double PI = 3.1415927;

	if (fill)
		glBegin(GL_POLYGON);
	else
		glBegin(GL_LINE_LOOP);

	int i;
	for (i = 0; i < 64; i++) {
		double angle = (double)i * PI / 32.0;
		double x = (double)cx + cos(angle)*(double)rad;
		double y = (double)cy + sin(angle)*(double)rad;
		glVertex2d(x, y);
	}
	glEnd();
}
