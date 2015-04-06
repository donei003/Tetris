/* Partner 1 Dylan O'Neill donei003@ucr.edu:
* Partner 2 Alex Wong awong030@ucr.edu:
* Lab Section: 023
* Assignment: Custom Lab Project
* Exercise Description: Tetris
*
* I acknowledge all content contained herein, excluding template or example
* code, is my own original work.
*/


#include <avr/io.h>
#include <stdlib.h>
#include "C:\Users\Dylan\Desktop\Tetris_Files\io.c"
#include "C:\Users\Dylan\Desktop\Tetris_Files\HelperFunctions.c"

//#include "C:\Users\student\Downloads\io.c"

#define SET_BIT(p,i) ((p) |= (1 << (i)))
#define CLR_BIT(p,i) ((p) &= ~(1 << (i)))
#define GET_BIT(p,i) ((p) & (1 << (i)))


/* When to call: This is the display function for our score, thus it should only be called in the while loop. 
   Not inside of an function.
   
   Purpose: Converting a three digit score into three individual digits that can be output on the seven-segment
   display. This function multiplexes the three displays allowing us to output each digit individually to each
   respective display. This creates a "constant" three digit score.
      
   How to use: Call this function to output to the seven-segment display.
   
   Notes: The number passed in should be no larger than 999.
*/
void Shift_Seven_Seg(unsigned short number) {
	signed short digit = number, data = 0x00;
	if (digit >= 100) {
		data = Tetris_DecToSevenSeg(digit / 100);
		digit = digit % 100;
	}
	else {
		data = Tetris_DecToSevenSeg(0);
	}
	transmit_data(data);
	PORTA = (PORTA & 0xF8) | 0x04;
	
	
	if (digit >= 10) {
		data = Tetris_DecToSevenSeg(digit / 10);
		digit = digit % 10;
	}
	else {
		data = Tetris_DecToSevenSeg(0);
	}
	transmit_data(data);
	PORTA = (PORTA & 0xF8) | 0x02;
	
	if (digit >= 1) {
		data = Tetris_DecToSevenSeg(digit);
	}
	else {
		data = Tetris_DecToSevenSeg(0);
	}
	transmit_data(data);
	PORTA = (PORTA & 0xF8) | 0x01;
}

// Clears the LED screen. Called when the user restarts.
void Matrix_ClearScreen() {
	for(unsigned char i = 0; i < 8; ++i) {
		for(unsigned char j = 0; j < 8; ++j) {
			SET_MATRIX_BIT(i, j, 0);
		}
	}
}

// Fills the screen. Called when the player loses.
void Matrix_FillScreen() {
	for(unsigned char i = 0; i < 8; ++i) {
		for(unsigned char j = 0; j < 8; ++j) {
			SET_MATRIX_BIT(i, j, 1);
		}
	}
}

/* When to call: Called during the while loop since it's a state machine.
   
   Purpose: Displays the Tetris game onto the LED Matrix.
   
   How to use: Called to display the game on all eight rows of the matrix. 
   
   Notes:
*/
enum DispStates {DISPLAY};
int Tetris_LEDDisplay(int state) {
	switch(state) {
		case DISPLAY:
		state = DISPLAY;
		break;
	}
	switch(state) {
		case DISPLAY:
		for(int i = 0; i < 8; ++i) {
			transmit_row(0xFF);
			transmit_col(tetrisCols[i]);
			transmit_row(tetrisRows[i]);
		}
		Shift_Seven_Seg(score);
		break;
		
		default:
		break;
	}
	return state;
}



//Three functions are used for generation of random block. Functions are randonNum, FirstBlock and GenerateBlock. The reason there is a FirstBlock
// function AND Generate block exists is due to the order of which we want the game to happen. FirstBlock creates a block ONLY once, randomly
//choosing a block using range of 0-70 instead of 0-7 due to it being more "random." You CAN use 0-7 but expect the same block to appear again and again.
// Tetris program will go into randomNum before hitting the main while loop. The randomNum function will be used to produce the NEXT random piece
//that will come down and hold that value (the variable piece). Then the while loop will be next. In the while loop, the condition for a new block to appear
//is if the block in the matrix stops falling. Once it stops falling, the piece that randomNum generated before (variable piece) will be used as placed into
//GenerateBlock(piece) and will produce a new block. After GenerateBlock is randomNum again which generates the NEXT block piece and is held there (along with the new piece variable)
// until it loops again and the block in the matrix stops falling.


//TLDR. FirstBlock makes only first block. RandomNum creates and holds next piece to drop. GenerateBlock generates the piece RandomNum holds.



// Generates a random number for the generateblock function and makes random block.
unsigned char piece;
void randomNum()
{
	unsigned long num;
	num = count % 70;
	
	// Intializes random number generator 
	//num = 23;

	if((num>= 0 && num <= 4) || (num>= 65 && num <= 69))
	{
		LCD_WriteData(0x00);
		piece=1;
	}
	if((num>= 10 && num <= 14) || (num>= 55 && num <= 59))
	{
		LCD_WriteData(0x01);
		piece=2;
	}
	if((num>= 20 && num <= 24) || (num>= 45 && num <= 49))
	{
		LCD_WriteData(0x02);
		piece=3;
	}
	if((num>= 30 && num <= 34) || (num>= 35 && num <= 39))
	{
		LCD_WriteData(0x03);
		piece=4;
	}
	if((num>= 40 && num <= 44) || (num>= 25 && num <= 29))
	{
		LCD_WriteData(0x04);
		piece=5;
	}
	if((num>= 50 && num <= 54) || (num>= 15 && num <= 19))
	{
		LCD_WriteData(0x05);
		piece=6;
	}
	if((num>= 60 && num <= 64) || (num>= 5 && num <= 9))
	{
		LCD_WriteData(0x06);
		piece=7;
	}
	
	
}


/* When to call: Called when a block can no longer fall.
   
   Purpose: Checks if the block that just fell cleared a row. If so clear it and add points to the score.
   
   How to use: Called to delete rows with all columns occupied.
   
   Notes: Score is dependent on how manys rows are cleared. The more rows, the more points.
*/
void clearRows() {
	unsigned char rowsCleared = 0x00;
	for(unsigned char i = 1; i < 9; ++i) {
		if(tetrisCols[i-1] == 0xFF) {
			rowsCleared += 1;
			for(unsigned char k = 0; k < 4; ++k) {
				if(GPLocations[k].row == i - 1) {
					GPLocations[k].row == TetrisRow;
				}
			}
			if(i-1 == 7) {
				for(unsigned char x = 0; x < 8; ++x) {
					SET_MATRIX_BIT(7, x, 0);
				}
			}
			for(unsigned char j = i-1; j < 7; ++j) {
				tetrisCols[j] = tetrisCols[j+1];
			}
			--i;
		}
	}
	if(rowsCleared == 1) {
		score += 10;
	}
	else if(rowsCleared == 2) {
		score += 20;
	}
	else if(rowsCleared == 3) {
		score += 40;
	}
	else if(rowsCleared == 4) {
		score += 80;
	}	
}



/* The state machine should have a period about half of the Tetris SM. This will be handling all of the inputs from the joystick. 
It will check to see if the user has made a move then check if it's allowed and finally shift if it is allowed. 

It sets up something close to a queue. Since moveLeft has higher precedence it will do that first if high, but on the next tick moveLeft would be low and
anything left will be shifted.
*/


enum Tetris_MoveStates {CHECK, SHIFT_LEFT, SHIFT_RIGHT, SHIFT_DOWN};
int Tetris_JoystickMoveBlock(int state) {
	switch(state) {
		case CHECK:
		if(!gameOver) {
			Tetris_Joystick();
			if(moveLeft) {
				checkMoveLeft();
				if(canMoveLeft) {
					moveLeft = 0;
					state = SHIFT_LEFT;
				}
				else {
					moveLeft = 0;
				}
			}
			else if(moveRight) {
				checkMoveRight();
				if(canMoveRight) {
					moveRight = 0;
					state = SHIFT_RIGHT;
				}
				else {
					moveRight = 0;
				}
			}
			else if(moveDown) {
				checkMoveDown();
				if(canMoveDown) {
					moveDown = 0;
					state = SHIFT_DOWN;
				}
				else {
					moveDown = 0;
				}
			}
		}
		else {
			state = CHECK;
		}
		break;
		
		case SHIFT_LEFT:
		state = CHECK;
		break;
		
		case SHIFT_RIGHT:
		state = CHECK;
		break;

		case SHIFT_DOWN:
		state = CHECK;
		break;				
	}
	switch(state) {
		case CHECK: 
		break;
		
		case SHIFT_LEFT:
		checkMoveLeft();
		if(canMoveLeft) {
			for(unsigned char i = 0; i < 4; ++i) {
				if(GPLocations[i].row < 8) {
					SET_MATRIX_BIT(GPLocations[i].row, GPLocations[i].col, 0);
				}
				GPLocations[i].col -= 1;
			}
			for(unsigned char i = 0; i < 4; ++i) {
				if(GPLocations[i].row < 8) {
					SET_MATRIX_BIT(GPLocations[i].row, GPLocations[i].col, 1);
				}
			}
		}
		break;
		
		case SHIFT_RIGHT:
		checkMoveRight();
		if(canMoveRight) {		
			for(unsigned char i = 0; i < 4; ++i) {
				if(GPLocations[i].row < 8) {
					SET_MATRIX_BIT(GPLocations[i].row, GPLocations[i].col, 0);
				}
				GPLocations[i].col += 1;
			}
			for(unsigned char i = 0; i < 4; ++i) {
				if(GPLocations[i].row < 8) {
					SET_MATRIX_BIT(GPLocations[i].row, GPLocations[i].col, 1);
				}
			}
		}
		break;
		
		case SHIFT_DOWN:
		checkMoveDown();
		if(canMoveDown) {
			for(unsigned char i = 0; i < 4; ++i) {
				if(GPLocations[i].row < 8) {
					SET_MATRIX_BIT(GPLocations[i].row, GPLocations[i].col, 0);
				}
				GPLocations[i].row -= 1;
			}
			for(unsigned char i = 0; i < 4; ++i) {
				if(GPLocations[i].row < 8) {
					SET_MATRIX_BIT(GPLocations[i].row, GPLocations[i].col, 1);
				}
			}
		}
		break;
	}
	return state;
}

/* When to call: Called in while loop.
   
   Purpose: The state machine that handles all of the game logic.
   
   How to use: Should not be called other than in the while loop. 
   
   Notes:
*/
enum Tetris_States{PLAY, FALL, END};
int Tetris_SM(int state) {
	static unsigned char dispOnce = 0x00;
	switch(state) {
		case PLAY:
		checkBlockFall();
		if(canFall) {
			canFall = 0x00;
			state = FALL;
		}
		else {
			clearRows(); // Check and clears row and adds score
			for(unsigned char i = 0; i < 4; ++i) {
				if(GPLocations[i].row >= TetrisRow) {
					gameOver = 1;
				}
			}
			LCD_ClearScreen();
			if(!gameOver) {
				GenerateBlock(piece);
				randomNum();
				state = PLAY;
			}
			else {
				dispOnce = 1;
				Matrix_ClearScreen();
				Matrix_FillScreen();
				state = END;
			}
		}
		break;
		
		case FALL:
		state = PLAY;
		break;
		
		case END:
		if((PINB & 0x10) == 0x00) {
			score = 0;
			gameOver = 0;
			LCD_ClearScreen();
			Matrix_ClearScreen();
			FirstBlock();
			count = count + ((count%70 + 1)*(count%7 + 1));
			randomNum();
			state = PLAY;
		}
		break;
		
		default:
		break;
	}
	switch(state) {
		case PLAY:
		break;
		
		case FALL:
		for(int i = 0; i < 4; ++i) {
			if(GPLocations[i].row < TetrisRow) {
				SET_MATRIX_BIT(GPLocations[i].row, GPLocations[i].col, 0);
			}
			GPLocations[i].row -= 1;
		}
		for(int i = 0; i < 4; ++i) {
			if(GPLocations[i].row < TetrisRow) {
				SET_MATRIX_BIT(GPLocations[i].row, GPLocations[i].col, 1);
			}
		}
		break;
		
		case END:
		if(dispOnce) {
			dispOnce = 0x00;
			LCD_ClearScreen();
			LCD_DisplayString(4, endMessage);
		}
		break;		
		
		default:
		break;
	}
	return state;
}





int main(void)
{
	DDRA = 0xC7; PORTA = 0x38;
	DDRB = 0xEF; PORTB = 0x10; 
	DDRC = 0xFF; PORTC = 0x00; // LCD data lines
	DDRD = 0xFF; PORTD = 0x00; // LCD control lines
	
	unsigned char i=0;
	tasks[i].state = PLAY;
	tasks[i].period = periodTetrisSM;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &Tetris_SM;
	++i;
	tasks[i].state = CHECK;
	tasks[i].period = periodJoyStick;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &Tetris_JoystickMoveBlock;
	++i;
	tasks[i].state = SAMPLE;
	tasks[i].period = periodOrientation;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &Orientation;
	++i;
	tasks[i].state = DISPLAY;
	tasks[i].period = periodDisplay;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &Tetris_LEDDisplay;		

	TimerSet(tasksPeriodGCD);
	TimerOn();
	
	LCD_init();
	init_adc();
	Tetris_PieceGen(); // Writes to Tetris pieces to LCD RAM
	
	FirstBlock();
	randomNum();
    while(1)
    {
		for (unsigned char z = 0; z < tasksNum; ++z) { // Heart of the scheduler code
			if ( tasks[z].elapsedTime >= tasks[z].period ) { // Ready
				tasks[z].state = tasks[z].TickFct(tasks[z].state);
				tasks[z].elapsedTime = 0;
			}
			++count;
			tasks[z].elapsedTime += tasksPeriodGCD;
		}
		while(!TimerFlag) {++count;}
		
		TimerFlag = 0;
    }
}

