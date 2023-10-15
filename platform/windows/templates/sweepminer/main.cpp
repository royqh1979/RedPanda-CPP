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

#define EMPTY 0
#define MINE  -1

class Block {
public:
	Block();
	Block(int x, int y, int btype);
	
	
	// Mutators
	void setX(int x);
	void setY(int y);
	void setRevealed(bool reveal);
	void setMarked(bool mark);
	void setType(int btype);
	
	// Accessors
	int getX();
	int getY();
	bool isRevealed();
	bool isMarked();
	int getType();
	
private:
	// Coordinates of block
	int mX;
	int mY;
	
	// States
	bool mRevealed;
	bool mMarked;
	
	// Type
	int mType; // Ranges from -1 to 8	
};

// Create 2d array
Block gameboard[ROWS][COLUMNS];


// Generate random number
int randomnum() {
	return rand() % 10;
}

// Generate number blocks
void countMinesInNeighbors(int i, int j);

// Update empty spots
void revealBlocks(int x, int y);

// Game states
bool gameover = false;
bool gamewon = false;
void restartGame();

int main()
{
	
	// Initialization of window
	//--------------------------------------------------------------------------------------
	int screenWidth = 450;
	int screenHeight = 450;
	InitWindow(screenWidth, screenHeight, "Minesweeper by Matthew Zegar");
	SetTargetFPS(60);
	
	restartGame();
	
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
					if (gameboard[y][x].isMarked() == false) {
						gameboard[y][x].setRevealed(true);
						if (gameboard[y][x].getType() == EMPTY) { // CHECK IF 'EMPTY'
							gameboard[y][x].setRevealed(false);
							revealBlocks(y, x);
						}
						
						if (gameboard[y][x].getType() == MINE) {
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
					if (gameboard[y][x].isRevealed() == false) {
						if (gameboard[y][x].isMarked() == false) {
							gameboard[y][x].setMarked(true);
						} else {
							gameboard[y][x].setMarked(false);
						}
					}
				} 
			}
		}
		
		// Draw the blocks 
		//--------------------------------------------------------------------------------------
		for (int i = 0; i < ROWS; ++i) {
			for (int j = 0; j < COLUMNS; ++j) {
				if (gameboard[i][j].isRevealed() == false) {
					DrawRectangle(gameboard[i][j].getY()*50, gameboard[i][j].getX()*50, 50, 50, LIGHTGRAY);
					DrawRectangleLines(gameboard[i][j].getY()*50, gameboard[i][j].getX()*50, 50, 50, BLACK); 
				} else {
					if (gameboard[i][j].getType() == MINE) {
						DrawRectangle(gameboard[i][j].getY()*50, gameboard[i][j].getX()*50, 50, 50, RED);
						DrawRectangleLines(gameboard[i][j].getY()*50, gameboard[i][j].getX()*50, 50, 50, BLACK); 
					} else if (gameboard[i][j].getType() == EMPTY) {
						DrawRectangle(gameboard[i][j].getY()*50, gameboard[i][j].getX()*50, 50, 50, GRAY);
						DrawRectangleLines(gameboard[i][j].getY()*50, gameboard[i][j].getX()*50, 50, 50, BLACK); 
					} else if (gameboard[i][j].getType() == 1) {
						DrawRectangle(gameboard[i][j].getY()*50, gameboard[i][j].getX()*50, 50, 50, DARKGRAY);
						DrawRectangleLines(gameboard[i][j].getY()*50, gameboard[i][j].getX()*50, 50, 50, BLACK);
						DrawText(TextFormat("%i", gameboard[i][j].getType()), gameboard[i][j].getY()*50+20, gameboard[i][j].getX()*50+15, 25, SKYBLUE);
					} else if (gameboard[i][j].getType() == 2) {
						DrawRectangle(gameboard[i][j].getY()*50, gameboard[i][j].getX()*50, 50, 50, DARKGRAY);
						DrawRectangleLines(gameboard[i][j].getY()*50, gameboard[i][j].getX()*50, 50, 50, BLACK);
						DrawText(TextFormat("%i", gameboard[i][j].getType()), gameboard[i][j].getY()*50+20, gameboard[i][j].getX()*50+15, 25, GREEN);
					} else if (gameboard[i][j].getType() == 3) {
						DrawRectangle(gameboard[i][j].getY()*50, gameboard[i][j].getX()*50, 50, 50, DARKGRAY);
						DrawRectangleLines(gameboard[i][j].getY()*50, gameboard[i][j].getX()*50, 50, 50, BLACK);
						DrawText(TextFormat("%i", gameboard[i][j].getType()), gameboard[i][j].getY()*50+20, gameboard[i][j].getX()*50+15, 25, RED);
					} else if (gameboard[i][j].getType() == 4) {
						DrawRectangle(gameboard[i][j].getY()*50, gameboard[i][j].getX()*50, 50, 50, DARKGRAY);
						DrawRectangleLines(gameboard[i][j].getY()*50, gameboard[i][j].getX()*50, 50, 50, BLACK);
						DrawText(TextFormat("%i", gameboard[i][j].getType()), gameboard[i][j].getY()*50+20, gameboard[i][j].getX()*50+15, 25, DARKBLUE);
					} else if (gameboard[i][j].getType() >= 5) {
						DrawRectangle(gameboard[i][j].getY()*50, gameboard[i][j].getX()*50, 50, 50, DARKGRAY);
						DrawRectangleLines(gameboard[i][j].getY()*50, gameboard[i][j].getX()*50, 50, 50, BLACK);
						DrawText(TextFormat("%i", gameboard[i][j].getType()), gameboard[i][j].getY()*50+20, gameboard[i][j].getX()*50+15, 25, MAROON);
					} 
				}
			}
		}
		
		// Draw the marked blocks (flags)
		//----------------------------------------------------------------------------------
		for (int i = 0; i < ROWS; ++i) {
			for (int j = 0; j < COLUMNS; ++j) {
				if (gameboard[i][j].isMarked() && !gameboard[i][j].isRevealed() ) {
					DrawRectangle(gameboard[i][j].getY()*50+15, gameboard[i][j].getX()*50+10, 10, 10, RED);
					DrawRectangle(gameboard[i][j].getY()*50+25, gameboard[i][j].getX()*50+10, 5, 25, BLACK);
					DrawRectangle(gameboard[i][j].getY()*50+20, gameboard[i][j].getX()*50+35, 15, 5, BLACK);
				}
			}
		}
		
		// Update variable to determine if won
		//----------------------------------------------------------------------------------
		int totalblocks = 0;
		for (int i = 0; i < ROWS; ++i) {
			for (int j = 0; j < COLUMNS; ++j) {                
				if (gameboard[i][j].getType() >= 1 && gameboard[i][j].isRevealed() == false) {
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
					gameboard[i][j].setRevealed(true);
				}
			}
			
			if ((IsMouseButtonReleased(MOUSE_RIGHT_BUTTON))) {
				restartGame();
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
			
			for (int i = 0; i < ROWS; ++i) {
				for (int j = 0; j < COLUMNS; ++j) {
					gameboard[i][j].setRevealed(true);
				}
			}
			
			if ((IsMouseButtonReleased(MOUSE_RIGHT_BUTTON))) {
				restartGame();
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
void revealBlocks(int x, int y) {
	if (x >= 0 && x <ROWS && y >= 0 && y < COLUMNS) {
		if (gameboard[x][y].getType() != EMPTY) {
			gameboard[x][y].setRevealed(true);
		} else {
			if (gameboard[x][y].isRevealed() == false) {
				gameboard[x][y].setRevealed(true);
				revealBlocks(x-1, y);
				revealBlocks(x-1, y-1);
				revealBlocks(x-1, y+1);
				
				revealBlocks(x, y-1);
				revealBlocks(x, y+1);
				
				revealBlocks(x+1, y-1);
				revealBlocks(x+1, y);
				revealBlocks(x+1, y+1);
			}    
		}
	}
}

// Restart the game
void restartGame() {
	for (int i = 0; i < ROWS; ++i) {
		for (int j = 0; j < COLUMNS; ++j) {
			gameboard[i][j] = Block(i,j,EMPTY);
		}
	}
	
	// Generate mines
	//-------------------------------------------------------------------------------------- 
	srand(time(NULL));
	for (int i = 0; i < ROWS; ++i) {
		for (int j = 0; j < COLUMNS; ++j) {
			if (randomnum() == 1) {
				gameboard[i][j].setType(MINE);
			}
		}
	}
	
	// Generate numbers
	//--------------------------------------------------------------------------------------
	for (int i = 0; i < 9; ++i) {
		for (int j = 0; j < 9; ++j) {
			if (gameboard[i][j].getType() != MINE) {
				countMinesInNeighbors(i,j);
			}
		}
	}
}

// Generates the numbered blocks based on mines
void countMinesInNeighbors(int i, int j) {
	int count = 0;
	
	// Check left hand side
	if ((j-1)>=0) {
		if ((i-1) >= 0) {
			if (gameboard[i-1][j-1].getType() == MINE) {
				count++;
			}
		}
		if (gameboard[i][j-1].getType() == MINE) {
			count++;
		}
		if ((i+1) < ROWS) {
			if (gameboard[i+1][j-1].getType() == MINE) {
				count++;
			}
		}
	}
	
	
	// Check middle
	if ((i-1) >= 0) {
		if (gameboard[i-1][j].getType() == MINE) {
			count++;
		}
	}
	
	if ((i+1) < ROWS) {
		if (gameboard[i+1][j].getType() == MINE) {
			count++;
		}
	} 
	
	// Check right
	if ((j+1)<COLUMNS) {
		if ((i-1) >= 0) {
			if (gameboard[i-1][j+1].getType() == MINE) {
				count++;
			}
		}
		if (gameboard[i][j+1].getType() == MINE) {
			count++;
		}
		if ((i+1) < ROWS) {
			if (gameboard[i+1][j+1].getType() == MINE) {
				count++;
			}
		} 		
	}
	
	if (count>0)
		gameboard[i][j].setType(count);
	
}

Block::Block() {
	mX = 0;
	mY = 0;
	mRevealed = false;
	mMarked = false;
	mType = 0;
}

Block::Block(int x, int y, int btype) {
	mX = x;
	mY = y;
	mRevealed = false;
	mMarked = false;
	mType = btype;
}

///////////////////////
// Mutators
///////////////////////

void Block::setX(int x) {
	mX = x;
}

void Block::setY(int y) {
	mY = y;
}

void Block::setRevealed(bool reveal) {
	mRevealed = reveal;
}

void Block::setMarked(bool mark) {
	mMarked = mark;
}

void Block::setType(int btype) {
	mType = btype;
}

///////////////////////
// Accessors
///////////////////////

int Block::getX() {
	return mX;
}

int Block::getY() {
	return mY;
}

bool Block::isRevealed() {
	return mRevealed;
}

bool Block::isMarked() {
	return mMarked;
}

int Block::getType() {
	return mType;
}

