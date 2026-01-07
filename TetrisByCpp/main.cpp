#include <iostream>
#include <Windows.h>
#include <thread>
#include <vector>

using namespace std;

wstring blocks[7];
int fieldWidth = 12;
int fieldHeight = 18;
// w & h of console window
int screenWidth = 120;
int screenHeight = 30;

unsigned char *pField = nullptr;

int Rotate(int x, int y, int r) {
	switch (r % 4) {
	case 0: return y * 4 + x;         //   0 degree
	case 1: return 12 + y - x * 4;    //  90 degree
	case 2: return 15 - y * 4 - x;    // 180 degree
	case 3:return 3 - y + x * 4;      // 270 degree
	}
	return 0;
}

bool IsPieceFit(int block, int rotation, int posX, int posY) {
	for (int x = 0; x < 4; x++) {
		for (int y = 0; y < 4; y++) {
			// index in piece
			int pi = Rotate(x, y, rotation);
			// index in field
			int fi = (posY + y) * fieldWidth + (posX + x);

			if (posX + x >= 0 && posX + x < fieldWidth) {
				if (posY + y >= 0 && posY + y < fieldHeight) {
					if (blocks[block][pi] == L'X' && pField[fi] != 0) {
						return false;
					}
				}
			}
		}
	}
	return true;
}

int main() {
	// create blocks
	blocks[0].append(L"..X.");
	blocks[0].append(L"..X.");
	blocks[0].append(L"..X.");
	blocks[0].append(L"..X.");

	blocks[1].append(L"....");
	blocks[1].append(L".XX.");
	blocks[1].append(L"..X.");
	blocks[1].append(L"..X.");

	blocks[2].append(L"....");
	blocks[2].append(L"..XX");
	blocks[2].append(L"..X.");
	blocks[2].append(L"..X.");

	blocks[3].append(L"....");
	blocks[3].append(L".XX.");
	blocks[3].append(L".XX.");
	blocks[3].append(L"....");

	blocks[4].append(L"....");
	blocks[4].append(L"..X.");
	blocks[4].append(L".XX.");
	blocks[4].append(L"..X.");

	blocks[5].append(L"..X.");
	blocks[5].append(L".XX.");
	blocks[5].append(L".X..");
	blocks[5].append(L"....");

	blocks[6].append(L".X..");
	blocks[6].append(L".XX.");
	blocks[6].append(L"..X.");
	blocks[6].append(L"....");

	// init
	pField = new unsigned char[fieldWidth * fieldHeight];
	for (int x = 0; x < fieldWidth; x++) {
		for (int y = 0; y < fieldHeight; y++) {
			pField[y * fieldWidth + x] = (x == 0 || x == fieldWidth - 1 || y == fieldHeight - 1) ? 9 : 0;
		}
	}

	wchar_t *screen = new wchar_t[screenWidth * screenHeight];
	for (int i = 0; i < screenWidth * screenHeight; i++) screen[i] = L' ';
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;
	
	bool isGameOver = false;

	int curPiece = 1;
	int curRotation = 0;
	int curX = fieldWidth / 2;
	int curY = 0;
	
	bool key[4];
	bool isRotateHold = false;

	int fallSpd = 20;
	int fallSpdCounter = 0;
	bool isForceDown = false;
	int pieceCount = 0;
	int score = 0;

	vector<int> lines;

	// frame
	while (!isGameOver) {
		// game runtime
		this_thread::sleep_for(50ms);
		fallSpdCounter++;
		isForceDown = (fallSpdCounter == fallSpd);
		// input
		for (int k = 0; k < 4; k++) {                           // R   L   D Z
			key[k] = (0x8000 & GetAsyncKeyState((unsigned char)("\x27\x25\x28Z"[k]))) != 0;
		}
		// logic
		if (key[0]) {
			if (IsPieceFit(curPiece, curRotation, curX + 1, curY)) {
				curX = curX + 1;
			}
		}
		if (key[1]) {
			if (IsPieceFit(curPiece, curRotation, curX - 1, curY)) {
				curX = curX - 1;
			}
		}
		if (key[2]) {
			if (IsPieceFit(curPiece, curRotation, curX, curY + 1)) {
				curY = curY + 1;
			}
		}
		if (key[3]) {
			if (IsPieceFit(curPiece, curRotation + 1, curX, curY) && !isRotateHold) {
				curRotation = curRotation + 1;
			}
			isRotateHold = true;
		}
		else {
			isRotateHold = false;
		}

		if (isForceDown) {
			if (IsPieceFit(curPiece, curRotation, curX, curY + 1)) {
				curY++;
			}
			else {
				// lock the cur piece
				for (int x = 0; x < 4; x++) {
					for (int y = 0; y < 4; y++) {
						if (blocks[curPiece][Rotate(x, y, curRotation)] == L'X') {
							pField[(curY + y) * fieldWidth + (curX + x)] = curPiece + 1;
						}
					}
				}

				pieceCount++;
				if (pieceCount % 10 == 0) {
					if (fallSpd >= 10) fallSpd--;
				}

				// check is there any lines
				for (int y = 0; y < 4; y++) {
					if (curY + y < fieldHeight - 1) {
						bool isLine = true;
						for (int x = 1; x < fieldWidth - 1; x++) {
							isLine &= (pField[(curY + y) * fieldWidth + x]) != 0;
						}

						if (isLine) {
							// remove line
							for (int x = 1; x < fieldWidth - 1; x++) {
								pField[(curY + y) * fieldWidth + x] = 8;
							}

							lines.push_back(curY + y);
						}
					}
				}

				score += 25;
				if (!lines.empty()) score += (1 << lines.size()) * 100;
				
				// create next piece
				curX = fieldWidth / 2;
				curY = 0;
				curRotation = 0;
				curPiece = rand() % 7;

				// if piece not fit => game over
				isGameOver = !IsPieceFit(curPiece, curRotation, curX, curY);
			}

			fallSpdCounter = 0;
		}
		// render output

		// draw field
		for (int x = 0; x < fieldWidth; x++) {
			for (int y = 0; y < fieldHeight; y++) {
				screen[(y + 2) * screenWidth + (x + 2)] = L" ABCDEFG=#"[pField[y * fieldWidth + x]];
			}
		}

		//draw cur piece
		for (int x = 0; x < 4; x++) {
			for (int y = 0; y < 4; y++) {
				if (blocks[curPiece][Rotate(x, y, curRotation)] == L'X')
					screen[(curY + y + 2) * screenWidth + (curX + x + 2)] = curPiece + 65;
			}
		}

		// draw score
		swprintf_s(&screen[2 * screenWidth + screenHeight + 6], 16, L"SCORE: %8d", score);

		if (!lines.empty()) {
			WriteConsoleOutputCharacter(hConsole, screen, screenWidth * screenHeight, { 0,0 }, &dwBytesWritten);
			this_thread::sleep_for(400ms);

			for (auto &v : lines) {
				for (int x = 1; x < fieldWidth - 1; x++) {
					for (int y = v; y > 0; y--) {
						pField[y * fieldWidth + x] = pField[(y - 1) * fieldWidth + x];
					}
					pField[x] = 0;
				}
			}

			lines.clear();
		}

		// display
		WriteConsoleOutputCharacter(hConsole, screen, screenWidth * screenHeight, { 0, 0 }, &dwBytesWritten);
	}

	CloseHandle(hConsole);
	cout << "Game Over!! Score:" << score << endl;
	system("pause");

	return 0;
}