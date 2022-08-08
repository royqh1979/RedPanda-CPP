/*
 * Minesweeper by Matthew Zegar
 * https://github.com/mzegar/Minesweeper-Cplusplus
 * app icon from https://www.flaticon.com/free-icon/mine_486968
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <utility>

#include <raylib.h>

using namespace std;

#define ROWS 9
#define COLUMNS 9

const int empty = -1;
const int mine = 0;

class block {
public:
	block();
	block(int x, int y, int btype);
	
	
	// Mutators
	setx(int x);
	sety(int y);
	setrevealed(bool reveal);
	setmarked(bool mark);
	settype(int btype);
	formatblock(int x, int y);
	
	// Accessors
	int getx();
	int gety();
	bool getrevealed();
	bool getmarked();
	int gettype();
	
private:
	// Coordinates of block
	int xcord;
	int ycord;
	
	// States
	bool revealed;
	bool marked;
	
	// Type
	int type; // Ranges from -1 to 8
	
};

// Create 2d array
block gameboard[ROWS][COLUMNS];


// Generate random number
int randomnum() {
	return rand() % 10;
}

// Generate number blocks
void numbergeneration(int i, int j);

// Update empty spots
void floodfill(int x, int y);

// Game states
bool gameover = false;
bool gamewon = false;
void restartgame();

// Block counts
int totalblocks = 0;


int main()
{
	
	// Initialization of window
	//--------------------------------------------------------------------------------------
	int screenWidth = 450;
	int screenHeight = 450;
	InitWindow(screenWidth, screenHeight, "Minesweeper by Matthew Zegar");
	SetTargetFPS(60);
	
	restartgame();
	
	// Mouse setup
	//--------------------------------------------------------------------------------------   
	Vector2 mouse; 
	
	// Main game loop
	while (!WindowShouldClose())    // Detect window close button or ESC key
	{
		// Mouse update
		//----------------------------------------------------------------------------------
		mouse = GetMousePosition();
		
		// Draw
		//----------------------------------------------------------------------------------
		BeginDrawing();
		
		// Mouse left click
		//----------------------------------------------------------------------------------
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			if (!gameover && !gamewon) {
				int y = floor(mouse.y/50);
				int x = floor(mouse.x/50);
				if (y >= 0 && y < ROWS
					&& x >=0 && x < COLUMNS ) {
					if (gameboard[y][x].getmarked() == false) {
						gameboard[y][x].setrevealed(true);
						if (gameboard[y][x].gettype() == -1) { // CHECK IF 'EMPTY'
							gameboard[y][x].setrevealed(false);
							floodfill(y, x);
						}
						
						if (gameboard[y][x].gettype() == 0) {
							gameover = true;
						}
					}
				}
			}
		}
		
		// Mouse right click
		//----------------------------------------------------------------------------------
		if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
			if (!gameover && !gamewon) {
				int y = floor(mouse.y/50);
				int x = floor(mouse.x/50);
				if (y >= 0 && y < ROWS
					&& x >=0 && x < COLUMNS ) {
					if (gameboard[y][x].getrevealed() == false) {
						if (gameboard[y][x].getmarked() == false) {
							gameboard[y][x].setmarked(true);
						} else {
							gameboard[y][x].setmarked(false);
						}
					}
				} 
			}
		}
		
		// Draw the blocks 
		//--------------------------------------------------------------------------------------
		for (int i = 0; i < ROWS; ++i) {
			for (int j = 0; j < COLUMNS; ++j) {
				if (gameboard[i][j].getrevealed() == false) {
					DrawRectangle(gameboard[i][j].gety()*50, gameboard[i][j].getx()*50, 50, 50, LIGHTGRAY);
					DrawRectangleLines(gameboard[i][j].gety()*50, gameboard[i][j].getx()*50, 50, 50, BLACK); 
				} else {
					if (gameboard[i][j].gettype() == 0) {
						DrawRectangle(gameboard[i][j].gety()*50, gameboard[i][j].getx()*50, 50, 50, RED);
						DrawRectangleLines(gameboard[i][j].gety()*50, gameboard[i][j].getx()*50, 50, 50, BLACK); 
					} else if (gameboard[i][j].gettype() == -1) {
						DrawRectangle(gameboard[i][j].gety()*50, gameboard[i][j].getx()*50, 50, 50, GRAY);
						DrawRectangleLines(gameboard[i][j].gety()*50, gameboard[i][j].getx()*50, 50, 50, BLACK); 
					} else if (gameboard[i][j].gettype() == 1) {
						DrawRectangle(gameboard[i][j].gety()*50, gameboard[i][j].getx()*50, 50, 50, DARKGRAY);
						DrawRectangleLines(gameboard[i][j].gety()*50, gameboard[i][j].getx()*50, 50, 50, BLACK);
						DrawText(TextFormat("%i", gameboard[i][j].gettype()), gameboard[i][j].gety()*50+20, gameboard[i][j].getx()*50+15, 25, SKYBLUE);
					} else if (gameboard[i][j].gettype() == 2) {
						DrawRectangle(gameboard[i][j].gety()*50, gameboard[i][j].getx()*50, 50, 50, DARKGRAY);
						DrawRectangleLines(gameboard[i][j].gety()*50, gameboard[i][j].getx()*50, 50, 50, BLACK);
						DrawText(TextFormat("%i", gameboard[i][j].gettype()), gameboard[i][j].gety()*50+20, gameboard[i][j].getx()*50+15, 25, GREEN);
					} else if (gameboard[i][j].gettype() == 3) {
						DrawRectangle(gameboard[i][j].gety()*50, gameboard[i][j].getx()*50, 50, 50, DARKGRAY);
						DrawRectangleLines(gameboard[i][j].gety()*50, gameboard[i][j].getx()*50, 50, 50, BLACK);
						DrawText(TextFormat("%i", gameboard[i][j].gettype()), gameboard[i][j].gety()*50+20, gameboard[i][j].getx()*50+15, 25, RED);
					} else if (gameboard[i][j].gettype() == 4) {
						DrawRectangle(gameboard[i][j].gety()*50, gameboard[i][j].getx()*50, 50, 50, DARKGRAY);
						DrawRectangleLines(gameboard[i][j].gety()*50, gameboard[i][j].getx()*50, 50, 50, BLACK);
						DrawText(TextFormat("%i", gameboard[i][j].gettype()), gameboard[i][j].gety()*50+20, gameboard[i][j].getx()*50+15, 25, DARKBLUE);
					} else if (gameboard[i][j].gettype() >= 5) {
						DrawRectangle(gameboard[i][j].gety()*50, gameboard[i][j].getx()*50, 50, 50, DARKGRAY);
						DrawRectangleLines(gameboard[i][j].gety()*50, gameboard[i][j].getx()*50, 50, 50, BLACK);
						DrawText(TextFormat("%i", gameboard[i][j].gettype()), gameboard[i][j].gety()*50+20, gameboard[i][j].getx()*50+15, 25, MAROON);
					} 
				}
			}
		}
		
		// Draw the marked blocks (flags)
		//----------------------------------------------------------------------------------
		for (int i = 0; i < ROWS; ++i) {
			for (int j = 0; j < COLUMNS; ++j) {
				if (gameboard[i][j].getmarked() == true and gameboard[i][j].getrevealed() == false) {
					DrawRectangle(gameboard[i][j].gety()*50+15, gameboard[i][j].getx()*50+10, 10, 10, RED);
					DrawRectangle(gameboard[i][j].gety()*50+25, gameboard[i][j].getx()*50+10, 5, 25, BLACK);
					DrawRectangle(gameboard[i][j].gety()*50+20, gameboard[i][j].getx()*50+35, 15, 5, BLACK);
				}
			}
		}
		
		// Update variable to determine if won
		//----------------------------------------------------------------------------------
		totalblocks = 0;
		for (int i = 0; i < ROWS; ++i) {
			for (int j = 0; j < COLUMNS; ++j) {                
				if (gameboard[i][j].gettype() >= 1 and gameboard[i][j].getrevealed() == false) {
					totalblocks++;
				}
			}
		}
		
		// Check for win
		//----------------------------------------------------------------------------------
		if (totalblocks == 0) {
			DrawText("Game won!", 103, 203, 50, BLACK);
			DrawText("Game won!", 100, 200, 50, GREEN);
			DrawText("right click to restart", 101, 251, 25, WHITE);
			DrawText("right click to restart", 100, 250, 25, BLACK); 
			gamewon = true;
			
			for (int i = 0; i < ROWS; ++i) {
				for (int j = 0; j < COLUMNS; ++j) {
					gameboard[i][j].setrevealed(true);
				}
			}
			
			if ((IsMouseButtonReleased(MOUSE_RIGHT_BUTTON))) {
				restartgame();
				gamewon = false;
			}            
		}
		
		// Check for gameover
		//----------------------------------------------------------------------------------
		if (gameover == true) {
			DrawText("Game Over", 103, 203, 50, BLACK);
			DrawText("Game Over", 100, 200, 50, RED);
			DrawText("right click to restart", 101, 251, 25, WHITE);
			DrawText("right click to restart", 100, 250, 25, BLACK);
			
			
			if ((IsMouseButtonReleased(MOUSE_RIGHT_BUTTON))) {
				restartgame();
				gameover = false;
			}
		}
		
		
		EndDrawing();
		//----------------------------------------------------------------------------------
	}
	
	// De-Initialization
	//--------------------------------------------------------------------------------------   
	CloseWindow();        // Close window and OpenGL context
	//--------------------------------------------------------------------------------------
	
	return 0;
}

// Floodfill algorithm 
void floodfill(int x, int y) {
	if (x >= 0 and x <ROWS and y >= 0 and y < COLUMNS) {
		if (gameboard[x][y].gettype() != -1) {
			gameboard[x][y].setrevealed(true);
		} else {
			if (gameboard[x][y].getrevealed() == false) {
				gameboard[x][y].setrevealed(true);
				floodfill(x-1, y);
				//floodfill(x-1, y-1);
				floodfill(x, y-1);
				//floodfill(x+1, y-1);
				floodfill(x+1, y);
				//floodfill(x+1, y+1);
				floodfill(x, y+1);
				//floodfill(x-1, y+1);
			}    
		}
	}
}

// Restart the game
void restartgame() {
	for (int i = 0; i < ROWS; ++i) {
		for (int j = 0; j < COLUMNS; ++j) {
			gameboard[i][j] = block(i,j,-1);
		}
	}
	
	// Generate mines
	//-------------------------------------------------------------------------------------- 
	srand(time(NULL));
	for (int i = 0; i < ROWS; ++i) {
		for (int j = 0; j < COLUMNS; ++j) {
			if (randomnum() == 1) {
				gameboard[i][j].settype(0);
			}
		}
	}
	
	// Generate numbers
	//--------------------------------------------------------------------------------------
	for (int i = 0; i < 9; ++i) {
		for (int j = 0; j < 9; ++j) {
			if (gameboard[i][j].gettype() != 0) {
				numbergeneration(i,j);
			}
		}
	}
}

// Generates the numbered blocks based on mines
void numbergeneration(int i, int j) {
	int count = 0;
	
	// Check left hand side
	if ((j-1)>=0) {
		if ((i-1) >= 0) {
			if (gameboard[i-1][j-1].gettype() == 0) {
				count++;
			}
		}
		if (gameboard[i][j-1].gettype() == 0) {
			count++;
		}
		if ((i+1) < ROWS) {
			if (gameboard[i+1][j-1].gettype() == 0) {
				count++;
			}
		}
	}
	
	
	// Check middle
	if ((i-1) >= 0) {
		if (gameboard[i-1][j].gettype() == 0) {
			count++;
		}
	}
	
	if ((i+1) < ROWS) {
		if (gameboard[i+1][j].gettype() == 0) {
			count++;
		}
	} 
	
	// Check right
	if ((j+1)<COLUMNS) {
		if ((i-1) >= 0) {
			if (gameboard[i-1][j+1].gettype() == 0) {
				count++;
			}
		}
		if (gameboard[i][j+1].gettype() == 0) {
			count++;
		}
		if ((i+1) <= ROWS) {
			if (gameboard[i+1][j+1].gettype() == 0) {
				count++;
			}
		} 		
	}
	
	if (count>0)
		gameboard[i][j].settype(count);
	
}

block::block() {
	xcord = 0;
	ycord = 0;
	revealed = false;
	marked = false;
	type = 0;
}

block::block(int x, int y, int btype) {
	xcord = x;
	ycord = y;
	revealed = false;
	marked = false;
	type = btype;
}

///////////////////////
// Mutators
///////////////////////

block::setx(int x) {
	xcord = x;
}

block::sety(int y) {
	ycord = y;
}

block::setrevealed(bool reveal) {
	revealed = reveal;
}

block::setmarked(bool mark) {
	marked = mark;
}

block::settype(int btype) {
	type = btype;
}

block::formatblock(int x, int y) {
}

///////////////////////
// Accessors
///////////////////////

int block::getx() {
	return xcord;
}

int block::gety() {
	return ycord;
}

bool block::getrevealed() {
	return revealed;
}

bool block::getmarked() {
	return marked;
}

int block::gettype() {
	return type;
}

