/* Partner 1 Dylan O'Neill donei003@ucr.edu:
* Partner 2 Alex Wong awong030@ucr.edu:
* Lab Section: 023
* Assignment: Custom Lab Project
* Exercise Description: Tetris
*
* I acknowledge all content contained herein, excluding template or example
* code, is my own original work.
*/

//---------------------------------GLOBALS------------------------------------//
#define TetrisCol 8
#define TetrisRow 8
unsigned short score = 0x0000;
unsigned long count = 0;
unsigned char gameOver = 0x00;
const unsigned char endMessage[] = "Game Over!";
unsigned char moveLeft = 0, moveRight = 0, moveDown = 0; // Acts as a boolean value, if the joystick is moved in a particular direction the variable is set to 1
unsigned char canMoveLeft = 0x00, canMoveRight = 0x00, canMoveDown = 0x00, canOrient = 0x00;
unsigned char shiftLeft = 0x00, shiftRight = 0x00;
unsigned char canFall = 0x00, generatePiece = 0x00;
unsigned char piecesLeft = 0x00;
const unsigned char tetrisRows[TetrisRow] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F};
unsigned char tetrisCols[TetrisCol] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
typedef struct GamePiece {
	unsigned char row, col;
}GamePiece;
GamePiece GPLocations[4];

typedef struct task {
	int state;                  // Task's current state
	unsigned long period;       // Task period
	unsigned long elapsedTime;  // Time elapsed since last task tick
	int (*TickFct)(int);        // Task tick function
} task;
task tasks[4];

const unsigned char tasksNum = 4;
const unsigned long tasksPeriodGCD = 1;
const unsigned long periodTetrisSM = 300;
const unsigned long periodJoyStick = 40;
const unsigned long periodOrientation = 30;
const unsigned long periodDisplay = 2;


//----------------------------------------------------------------------------//

// Checks a position on the Tetris board to see if it is occupied.
unsigned char GET_MATRIX_BIT(unsigned char row, unsigned char column) {
	unsigned char returnBit = 0x00;
	returnBit = (tetrisCols[row] >> column) & 0x001;
	return returnBit;
}

// Sets the position to 1 or 0 which in turn will turn an LED on or off.
void SET_MATRIX_BIT(unsigned char row, unsigned char column, unsigned char bitVal) {
	if(bitVal) {
		tetrisCols[row] |= (0x01 << column);
	}
	else {
		tetrisCols[row] = tetrisCols[row] & ~(0x01 << column);
	}
}

// Timer functions we given in lab
volatile unsigned char TimerFlag = 0;
// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks
void TimerOn() {
	// AVR timer/counter controller register TCCR1
	// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s
	TCCR1B = 0x0B;
	// AVR output compare register OCR1A.
	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	OCR1A = 125;
	// AVR timer interrupt mask register
	// bit1: OCIE1A -- enables compare match interrupt
	TIMSK1 = 0x02;
	//Initialize avr counter
	TCNT1=0;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds
	_avr_timer_cntcurr = _avr_timer_M;
	//Enable global interrupts: 0x80: 1000000
	SREG |= 0x80;
}
void TimerOff() {
	// bit3bit1bit0=000: timer off
	TCCR1B = 0x00;
}
void TimerISR() {
	TimerFlag = 1;
}
// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1
	// (every 1 ms per TimerOn settings)
	// Count down to 0 rather than up to TOP (results in a more efficient comparison)
	_avr_timer_cntcurr--;
	if (_avr_timer_cntcurr == 0) {
		// Call the ISR that the user uses
		TimerISR();
		_avr_timer_cntcurr = _avr_timer_M;
	}
}
// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}


/* When to call: This function, as it's current state, is designed to only be called once.
   
   Purpose: The function accesses the LiquidCrystal display's CG RAM and writes 7 custom
   characters it. Leaving one remaining free space in RAM. 
   
   How to use: The seven characters are now at ASCII indexes 0 through 6. To display a game piece
   on the secondary display (LiquidCrystal), simply call LCD_WriteData() and give it a number between 
   0 and 6 (or 7 if we define another later on). Ex: LCD_WriteData(1);
   
   Notes: Currently, the game pieces are local variables, we will determine if they should be
   global or local when we get our new LCD.
*/
void Tetris_PieceGen() {
	const unsigned char GP_T[8] = {0x00, 0x0C, 0x0C, 0x0F, 0x0F, 0x0C, 0x0C, 0x00};
	const unsigned char GP_O[8] = {0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00};
	const unsigned char GP_I[8] = {0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06};
	const unsigned char GP_J[8] = {0x00, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x1E, 0x00};
	const unsigned char GP_L[8] = {0x00, 0x0C, 0x0C, 0x0C, 0x0C, 0x0F, 0x0F, 0x00};
	const unsigned char GP_S[8] = {0x00, 0x0C, 0x0C, 0x0F, 0x0F, 0x03, 0x03, 0x00};
	const unsigned char GP_Z[8] = {0x00, 0x03, 0x03, 0x0F, 0x0F, 0x0C, 0x0C, 0x00};
	unsigned char i;		
	LCD_WriteCommand(0x40);
	
	for(i = 0; i < 8; ++i) {
		LCD_WriteData(GP_T[i]);
	}
	for(i = 0; i < 8; ++i) {
		LCD_WriteData(GP_O[i]);
	}
	for(i = 0; i < 8; ++i) {
		LCD_WriteData(GP_I[i]);
	}
	
	for(i = 0; i < 8; ++i) {
		LCD_WriteData(GP_J[i]);
	}
	
	for(i = 0; i < 8; ++i) {
		LCD_WriteData(GP_L[i]);
	}
	
	for(i = 0; i < 8; ++i) {
		LCD_WriteData(GP_S[i]);
	}
	
	for(i = 0; i < 8; ++i) {
		LCD_WriteData(GP_Z[i]);
	}
	
	LCD_WriteCommand(0x80);
}


/* When to call: This function will be called inside of our score or display function to convert
   the integer score into an 8-bit control signal.
   
   Purpose: Dec_To_SevenSeg converts "integers" between 0-9 into control signals. The reason for this is outlined below
      
   How to use: This function will be used with our score function. Our score is four digits. So, we will be using
   the division and modulus operator to access each specific digit. They will be converted and each will be output 
   to it's respective display.
   
   Notes: This function may change in the future. We may want to have it take in the large number and 
   convert the whole thing and store it in four global variables.
*/
unsigned char Tetris_DecToSevenSeg(unsigned char score) {
	unsigned char controlSig = 0x00;
	if(score == 9) {
		controlSig = 0x0B;
	}
	else if(score == 8) {
		controlSig = 0x08;
	}
	else if(score == 7) {
		controlSig = 0xCB;
	}
	else if(score == 6) {
		controlSig = 0x18;
	}
	else if(score == 5) {
		controlSig = 0x19;
	}
	else if(score == 4) {
		controlSig = 0x2B;
	}
	else if(score == 3) {
		controlSig = 0x49;
	}
	else if(score == 2) {
		controlSig = 0x4C;
	}
	else if(score == 1) {
		controlSig = 0xEB;
	}
	else if(score == 0) {
		controlSig = 0x88;
	}
	return controlSig;
}

/* When to call: Never. Only called inside of Shift_Seven_Seg().
   
   Purpose: To transmit data from PORTB serially into a shift register which controls the eight LEDs of the three
   seven-segment displays.
      
   How to use: Call this function to output to shift register.
   
   Notes: Can only send eight bits.
*/
void transmit_data(unsigned char data) {
	int i;
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTB = (PORTB & 0xF0) | 0x08;
		// set SER = next bit of data to be sent.
		PORTB |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTB |= 0x02;
	}
	// set RCLK = 1. Rising edge copies data from â€œShiftâ€ register to â€œStorageâ€ register
	PORTB |= 0x04;
	// clears all lines in preparation of a new transmission
	PORTB = PORTB & 0xF0;
}


/* When to call: Never. Only called inside of the Tetris_LEDDisplay SM.
   
   Purpose: To transmit data from PORTD serially into a shift register which will turn a 
   particular row on or off. In the case of our LED Matrix, connecting the row to ground will
   allow the columns to be able to illuminate.
      
   How to use: Call this function to output to shift register.
   
   Notes: Can only send eight bits.
*/
void transmit_row(unsigned char data) {
	int i;
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTD = (PORTD & 0xF0) | 0x08;
		// set SER = next bit of data to be sent.
		PORTD |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTD |= 0x02;
	}
	// set RCLK = 1. Rising edge copies data from â€œShiftâ€ register to â€œStorageâ€ register
	PORTD |= 0x04;
	// clears all lines in preparation of a new transmission
	PORTD = PORTD & 0xF0;
}


/* When to call: Never. Only called inside of the Tetris_LEDDisplay SM.
   
   Purpose: To transmit data from PORTD serially into a shift register which will turn a 
   particular column on or off. In the case of our LED Matrix, connecting the LED to power if the
   row is connected to ground will illuminate the LED in that column.
      
   How to use: Call this function to output to shift register.
   
   Notes: Can only send eight bits.
*/
void transmit_col(unsigned char data) {
	int i;
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTD = (PORTD & 0x0F) | 0x80;
		// set SER = next bit of data to be sent.
		PORTD |= ((data >> i) & 0x01) << 4;
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTD |= 0x20;
	}
	// set RCLK = 1. Rising edge copies data from â€œShiftâ€ register to â€œStorageâ€ register
	PORTD |= 0x40;
	// clears all lines in preparation of a new transmission
	PORTD = PORTD & 0x0F;
}


// Initializes the ADC
void init_adc(void) // Function to initialise the ADC feature
{
	ADCSRA=0X00;
	ADMUX=0X40;    //0x40 (ob01000000) for 10 bits //0x60 (ob01100000) for 8 bits
	ADCSRA=0X87;
	ADCSRA=0X80;
}

// Reads the value from the ADC and returns and unsigned long
unsigned long read_adc(unsigned char pinnum)
{
	unsigned char i=0;
	unsigned long digval=0;
	ADCH=0x00; //initialising ADCH to 0 erases previous values

	i=pinnum&0x07;     //holds the pin-number of PORTA where from the Analog Value is being read
	ADMUX=i|0x60;   //0x40 (ob01000000) for 10 bits //0x60 (ob01100000) for 8 bits
	ADCSRA|=1<<ADSC;   //conversion is started writing 1 as high bit to ADSCRA

	while(ADCSRA & (1<<ADSC));  //conversion continues till ADSCRA returns interrupt value
	//in other words ADC conversion takes place till ADSC has high value

	//unsigned char temp=ADCH;  //reads ADCH value into temp variable
	 unsigned long temp=ADC; //for 10 bits its reads ADCL first then ADCH automatically

	//if(pinnum==0) //taking example of 1st pin (PA0) itself â€¦ can be extended to other pins as whole or individually
	//{
		/*if(temp>=120) // here i have decided my required value is 120 above which analog value is Digitally HIGH
		digval=1;
		else
		digval=0;*/
	//}
	digval=temp;
	return digval;
}



/* When to call: Is only called in the JoyStick SM.
   
   Purpose: Checks if the user has moved the joystick left, right, or down and sets
   each of their respective control bits high.
      
   How to use: Call in JoyStick SM to see if the user has moved the JoyStick. 
   
   Notes:
*/
void Tetris_Joystick() {
	unsigned long x,y;
	x = read_adc(3);
	y = read_adc(4);
	if(y < 32000) {
		moveRight = 1;
		//MOVE RIGHT
	}
	if(y > 38000) {
		moveLeft = 1;
		//MOVE LEFT
	}
	if(x < 32000) {
		moveDown = 1;
		//MOVE DOWN
	}
}

/* When to call: This function is called in the Orientation() function but can be used elsewhere.
   
   Purpose: Checks to see if the piece at a particular row and col is a piece of the Tetris block.
   Returns 1 if the position had a block piece there and zero if it was not.
      
   How to use: This is used in the Oreintation function since for some objects, changing its orientation will
   put a piece in the same position as another so this checks to see if that is the case. 
   
   Notes:
*/
unsigned char isMatrixPiece(unsigned char row, unsigned char col) {
	unsigned char isPiece = 0x00;
	for(unsigned char i = 0; i < 4; ++i) {
		if((GPLocations[i].row == row) && (GPLocations[i].col == col)) {
			isPiece = 1;
			return isPiece;
		}
	}
	return isPiece;
}

unsigned char checkPieceFall(GamePiece p) {
	unsigned char PieceFall = 0x00;
	if(p.row > 0 && (!(GET_MATRIX_BIT(p.row - 1, p.col)) || isMatrixPiece(p.row - 1, p.col))) {
		PieceFall = 1;
	}
	return PieceFall;
}

/* When to call: Called once.
   
   Purpose: Generates the initial block that drops.
      
   How to use: Call once before the while loop. 
   
   Notes:
*/
unsigned char BlockHolder = 0x00;
enum BlockStates {A, B, C, D} BlockState;
void FirstBlock()
{
	//int num;
	
	unsigned long num;
	num = count % 70;
	//num = read_adc(5) % 70;
	// Intializes random number generator
	if((num>= 0 && num <= 4) || (num>= 65 && num <= 69))
	{
		//T
		GPLocations[0].row = TetrisRow;
		GPLocations[0].col = 3;
		GPLocations[1].row = TetrisRow + 1;
		GPLocations[1].col = 4;
		GPLocations[2].row = TetrisRow;
		GPLocations[2].col = 4;
		GPLocations[3].row = TetrisRow;
		GPLocations[3].col = 5;
		for(unsigned char j = 0; j < 4; ++j) {
			SET_MATRIX_BIT(GPLocations[j].row, GPLocations[j].col, 1);
		}
		BlockHolder = 1;
		BlockState=A;
	}
	else if((num>= 10 && num <= 14) || (num>= 55 && num <= 59))
	{
		//square
		// O
		GPLocations[0].row = TetrisRow + 1;
		GPLocations[0].col = 3;
		GPLocations[1].row = TetrisRow + 1;
		GPLocations[1].col = 4;
		GPLocations[2].row = TetrisRow;
		GPLocations[2].col = 3;
		GPLocations[3].row = TetrisRow;
		GPLocations[3].col = 4;
		/*for(unsigned char j = 0; j < 4; ++j) {
			SET_MATRIX_BIT(GPLocations[j].row, GPLocations[j].col, 1);
		}*/
		BlockHolder = 2;
		BlockState=A;
	}
	else if((num>= 20 && num <= 24) || (num>= 45 && num <= 49))
	{
		// I
		GPLocations[0].row = TetrisRow + 3;
		GPLocations[0].col = 3;
		GPLocations[1].row = TetrisRow + 2;
		GPLocations[1].col = 3;
		GPLocations[2].row = TetrisRow + 1;
		GPLocations[2].col = 3;
		GPLocations[3].row = TetrisRow;
		GPLocations[3].col = 3;
		/*for(unsigned char j = 0; j < 4; ++j) {
			SET_MATRIX_BIT(GPLocations[j].row, GPLocations[j].col, 1);
		}*/
		BlockHolder = 3;
		BlockState=A;
	}
	else if((num>= 30 && num <= 34) || (num>= 35 && num <= 39))
	{
		// J
		GPLocations[0].row = TetrisRow + 2;
		GPLocations[0].col = 4;
		GPLocations[1].row = TetrisRow + 1;
		GPLocations[1].col = 4;
		GPLocations[2].row = TetrisRow;
		GPLocations[2].col = 4;
		GPLocations[3].row = TetrisRow;
		GPLocations[3].col = 3;
		/*for(unsigned char j = 0; j < 4; ++j) {
			SET_MATRIX_BIT(GPLocations[j].row, GPLocations[j].col, 1);
		}*/
		BlockHolder = 4;
		BlockState=A;
	}
	else if((num>= 40 && num <= 44) || (num>= 25 && num <= 29))
	{
		// L
		GPLocations[0].row = TetrisRow + 2;
		GPLocations[0].col = 3;
		GPLocations[1].row = TetrisRow + 1;
		GPLocations[1].col = 3;
		GPLocations[2].row = TetrisRow;
		GPLocations[2].col = 3;
		GPLocations[3].row = TetrisRow;
		GPLocations[3].col = 4;
		/*for(unsigned char j = 0; j < 4; ++j) {
			SET_MATRIX_BIT(GPLocations[j].row, GPLocations[j].col, 1);
		}*/
		BlockHolder = 5;
		BlockState=A;
	}
	else if((num>= 50 && num <= 54) || (num>= 15 && num <= 19))
	{
		// S
		GPLocations[0].row = TetrisRow;
		GPLocations[0].col = 3;
		GPLocations[1].row = TetrisRow;
		GPLocations[1].col = 4;
		GPLocations[2].row = TetrisRow + 1;
		GPLocations[2].col = 4;
		GPLocations[3].row = TetrisRow + 1;
		GPLocations[3].col = 5;
		/*for(unsigned char j = 0; j < 4; ++j) {
			SET_MATRIX_BIT(GPLocations[j].row, GPLocations[j].col, 1);
		}*/
		BlockHolder = 6;
		BlockState=A;
	}
	else if((num>= 60 && num <= 64) || (num>= 5 && num <= 9))
	{
		// Z
		GPLocations[0].row = TetrisRow + 1;
		GPLocations[0].col = 3;
		GPLocations[1].row = TetrisRow + 1;
		GPLocations[1].col = 4;
		GPLocations[2].row = TetrisRow;
		GPLocations[2].col = 4;
		GPLocations[3].row = TetrisRow;
		GPLocations[3].col = 5;
		/*for(unsigned char j = 0; j < 4; ++j) {
			SET_MATRIX_BIT(GPLocations[j].row, GPLocations[j].col, 1);
		}*/
		BlockHolder = 7;
		BlockState=A;
	}
}

/* When to call: Is only called in the Orientation function.
   
   Purpose: Sets a control bit high if a block is allowed to re-orient itself.
   
   How to use: Call in Orientation and then check if canOrient is 1 or 0. 
   
   Notes:
*/
void checkOrientation() {
	unsigned char piecesFree = 0x00;
	unsigned char checkPieceLeft = 0x00, checkPieceRight = 0x00;
	if(BlockHolder==1) {
		// T
		if(BlockState==A)
		{
			//				1								0
			//		0		2		3		becomes			2	1
			//												3
			if(GPLocations[2].row > 0) {
				piecesFree += 2; // This for pieces 0 and 1 which take the positions of 1 and 3 respectively.
				if(!(GET_MATRIX_BIT(GPLocations[3].row - 1, GPLocations[3].col - 1))) {
					piecesFree +=1;
				}
			}
		}
		else if(BlockState==B)
		{
			//				0								
			//				2	1			becomes		3	2	0
			//				3								1
			
			if(GET_MATRIX_BIT(GPLocations[3].row + 1, GPLocations[3].col - 1)) {
				//shiftBlockRight();
				++shiftRight;
			}
			
			if((GPLocations[0].row > 0) && (GPLocations[0].col < TetrisCol)) {
				if(GPLocations[0].col == 0 || shiftRight) {
					piecesFree += 1; // This is for piece 3 which takes piece 2's current position.
					if(!(GET_MATRIX_BIT(GPLocations[0].row - 1, GPLocations[0].col + 2)) && ((GPLocations[0].col + 2) < TetrisCol)) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[1].row - 1, GPLocations[1].col))) {
						piecesFree += 1;
					}
				}
				else {
					piecesFree += 2; // This for pieces 0 and 1 which take the positions of 1 and 3 respectively.
					if(!(GET_MATRIX_BIT(GPLocations[3].row + 1, GPLocations[3].col - 1))) {
						piecesFree +=1;
					}
				}
			}
		}
		else if(BlockState==C)
		{
			//												3
			//			3	2	0			becomes		1	2	
			//				1								0
			
			piecesFree += 2; // This for pieces 0 and 1 which take the positions of 1 and 3 respectively.
			if(!(GET_MATRIX_BIT(GPLocations[3].row + 1, GPLocations[3].col + 1))) {
				piecesFree +=1;
			}
			
		}
		else if(BlockState==D)
		{
			//				3								1
			//			1	2				becomes		0	2	3
			//				0	
			
			if(GET_MATRIX_BIT(GPLocations[3].row - 1, GPLocations[3].col + 1)) {
				//shiftBlockLeft();
				++shiftLeft;
			}
			
			if((GPLocations[0].row >= 0) && (GPLocations[0].col < TetrisCol)) {
				if(GPLocations[0].col == TetrisCol - 1 || shiftLeft) {
					piecesFree += 1; // This is for piece 3 which takes piece 2's current position.
					if(!(GET_MATRIX_BIT(GPLocations[0].row + 1, GPLocations[0].col - 2)) && ((GPLocations[0].col - 1) < TetrisCol)) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[1].row + 1, GPLocations[1].col))) {
						piecesFree += 1;
					}
				}
				/*else if(shiftLeft) {
					piecesFree += 1; // This is for piece 3 which takes piece 2's current position.
					if(!(GET_MATRIX_BIT(GPLocations[0].row + 1, GPLocations[0].col - 1)) && ((GPLocations[0].col - 1) < TetrisCol)) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[1].row + 1, GPLocations[1].col + 1))) {
						piecesFree += 1;
					}
				}*/
				else {
					piecesFree += 2;
					if(!(GET_MATRIX_BIT(GPLocations[3].row - 1, GPLocations[3].col + 1))) {
						piecesFree +=1;
					}
				}	
			}
		}
	}
				
	else if(BlockHolder == 2) {
	}
				
	else if(BlockHolder==3) { //if piece is I
		if(BlockState==A)
		{		
			//				0
			//				1				becomes
			//				2								3 2 1 0
			//				3
			
			if(GET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col - 1)) {
				++shiftRight;
			}
			
			else if(GET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col + 1)) {
				shiftLeft = 2;
			}
			
			else if(GET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col + 2)) {
				shiftLeft = 1;
			}
			
			if((GPLocations[0].row > 1) && (GPLocations[0].col < TetrisCol)) {
				if((GPLocations[0].col == 0) || shiftRight) {
					checkPieceRight = 1;
					piecesFree += 1; // This is for piece 3 since it only moves up, the only thing that can occupy that spot is piece 2
					if(!(GET_MATRIX_BIT(GPLocations[0].row - 2, GPLocations[0].col + 3)) && ((GPLocations[0].col + 3) < TetrisCol)) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[1].row - 1, GPLocations[1].col + 2))) {
						piecesFree += 1;
					}
					if((GET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col + checkPieceRight))) {
						piecesFree = 0;
					}
				}
				else if((GPLocations[0].col == TetrisCol - 2) || (shiftLeft == 1)) {
					checkPieceLeft = 1;
					piecesFree += 1; // This is for piece 1 since it only moves down, the only thing that can occupy that spot is piece 2
					if(!(GET_MATRIX_BIT(GPLocations[0].row - 2, GPLocations[0].col + 1))) {
						piecesFree += 1;
					}
					if((GET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col - checkPieceLeft))) {
						piecesFree = 0;
					}
					if(!(GET_MATRIX_BIT(GPLocations[3].row + 1, GPLocations[3].col - 2)) && (((GPLocations[3].col - 2) >= 0) && ((GPLocations[3].col - 2) < TetrisCol))) {
						piecesFree +=1;
					}
				}
				else if((GPLocations[0].col == TetrisCol - 1) || (shiftLeft == 2)) {
					checkPieceLeft = 2;
					piecesFree += 1; // This is for piece 0 since it only moves down, the only thing that can occupy that spot is piece 2
					if(!(GET_MATRIX_BIT(GPLocations[1].row - 1, GPLocations[1].col - 1))) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[3].row - 1, GPLocations[3].col - 3)) && (((GPLocations[3].col - 3) >= 0) && ((GPLocations[3].col - 3) < TetrisCol))) {
						piecesFree += 1;
					}
					if((GET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col - checkPieceLeft))) {
						piecesFree = 0;
					}
				}
				else {
					if(!(GET_MATRIX_BIT(GPLocations[0].row - 2, GPLocations[0].col + 2))) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[1].row - 1, GPLocations[1].col + 1))) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[3].row + 1, GPLocations[3].col - 1))) {
						piecesFree +=1;
					}
				}
			}				
		}
		else if(BlockState==B)
		{
						
			//												0
			//								becomes			1
			//			3 2 1 0								2
			//												3
			
			if(!(GET_MATRIX_BIT(GPLocations[0].row + 2, GPLocations[0].col - 2))) {
				piecesFree += 1;
			}
			if(!(GET_MATRIX_BIT(GPLocations[1].row + 1, GPLocations[1].col - 1))) {
				piecesFree += 1;
			}
			if(!(GET_MATRIX_BIT(GPLocations[3].row - 1, GPLocations[3].col + 1))) {
				piecesFree +=1;
			}
			
		}
	}


	else if(BlockHolder==4) { // J piece
		if(BlockState==A)
		{
						
						
			//			*	0	*
			//			*	1	*			becomes				*  3  *  *
			//			3	2	*								*  2  1  0
			//			*	*	*								*  *  *  *
			
			if(GET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col + 1)) {
				shiftLeft = 2;
			}
			else if(GET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col + 2)) {
				shiftLeft = 1;
			}
			
			if((GPLocations[0].row > 1) && (GPLocations[0].col < TetrisCol)) {
				if((GPLocations[0].col == TetrisCol - 2) || (shiftLeft == 1)) {
					piecesFree += 1; // This is for piece 1 since the location it moves to is always occupied by a block piece.
					if(!(GET_MATRIX_BIT(GPLocations[0].row - 2, GPLocations[0].col + 1))) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[3].row + 1, GPLocations[3].col))) {
						piecesFree +=1;
					}
				}
				else if((GPLocations[0].col == TetrisCol - 1) || (shiftLeft == 2)) {
					piecesFree += 2; // This is for pieces 0 and 1, they are moving into spots already occupied by pieces 2 and 3.
					if(!(GET_MATRIX_BIT(GPLocations[3].row + 1, GPLocations[3].col - 1)) && (((GPLocations[3].col - 1) >= 0) && ((GPLocations[3].col - 1) < TetrisCol))) {
						piecesFree +=1;
					}
					if(GET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col - 2)) {
						piecesFree = 0;
					}
				}
				else {
					if(!(GET_MATRIX_BIT(GPLocations[0].row - 2, GPLocations[0].col + 2))) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[1].row - 1, GPLocations[1].col + 1))) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[3].row + 1, GPLocations[3].col + 1))) {
						piecesFree +=1;
					}
					else if(isMatrixPiece(GPLocations[3].row + 1, GPLocations[3].col + 1)) {
						piecesFree += 1;
					}
				}
			}
		}
		else if(BlockState==B)
		{
						
			//			3
			//			2	1	0			becomes			2	3
			//												1
			//												0
			
			piecesFree += 1; // This is for piece 3 which takes 1's current position.
			if(!(GET_MATRIX_BIT(GPLocations[0].row - 2, GPLocations[0].col - 2)) && (GPLocations[0].row > 1)) {
				piecesFree += 1;
			}
			if(!(GET_MATRIX_BIT(GPLocations[1].row - 1, GPLocations[1].col - 1))) {
				piecesFree += 1;
			}
			
		}
		else if(BlockState==C)
		{
						
						
			//
			//			2   3		becomes      0	 1   2
			//			1								 3
			//			0
			
			if(GET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col - 1)) {
				shiftRight = 2;
			}
			else if(GET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col - 2)) {
				shiftRight = 1;
			}
			
			if((GPLocations[0].row >= 0) && (GPLocations[0].col < TetrisCol)) {
				if((GPLocations[0].col == 0) || (shiftRight == 2)) {
					piecesFree += 2; // This is for pieces 0 and 1, they are moving into spots already occupied by pieces 2 and 3.
					if(!(GET_MATRIX_BIT(GPLocations[3].row - 1, GPLocations[3].col + 1)) && (((GPLocations[3].col + 1) >= 0) && ((GPLocations[3].col + 1) < TetrisCol))) {
						piecesFree +=1;
					}
					if((GET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col + 2))) {
						piecesFree = 0;
					}
				}
				else if((GPLocations[0].col == 1) || (shiftRight == 1)) {
					piecesFree += 1; // This is for piece 1, it is moving into a spot already occupied by piece 2.
					if(!(GET_MATRIX_BIT(GPLocations[0].row + 2, GPLocations[0].col - 1))) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[3].row - 1, GPLocations[3].col))) {
						piecesFree +=1;
					}
				}
				else {
					piecesFree += 1; // This is for piece 3 which takes 1's current position. 
					if(!(GET_MATRIX_BIT(GPLocations[0].row + 2, GPLocations[0].col - 2))) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[1].row + 1, GPLocations[1].col - 1))) {
						piecesFree += 1;
					}
					
				}
			}
		}
		else if(BlockState==D)
		{
						
			//													0
			//													1
			//			0	 1	  2			becomes			3	2
			//					  3
			
			piecesFree += 1; // This is for piece 3 since it will be moving into 1's current position.
			if(!(GET_MATRIX_BIT(GPLocations[0].row + 2, GPLocations[0].col + 2))) {
				piecesFree += 1;
			}
			if(!(GET_MATRIX_BIT(GPLocations[1].row + 1, GPLocations[1].col + 1))) {
				piecesFree += 1;
			}
		}
	}
				
	else if(BlockHolder==5) { // L piece
		if(BlockState==A)
		{
						
						
			//			0
			//			1					becomes				  2	 1  0
			//			2	3									  3
			//
			
			if(GET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col + 2)) {
				++shiftLeft;
			}
			
			if((GPLocations[3].row > 0) && (GPLocations[3].col < TetrisCol)) {
				if((GPLocations[0].col == TetrisCol - 2) || shiftLeft) {
					piecesFree += 2; // This is for pieces 0 and 1 which move into 2 and 3's current position.
					if(!(GET_MATRIX_BIT(GPLocations[3].row - 1, GPLocations[3].col - 2))) {
						piecesFree += 1;
					}
					if((GET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col - 1)) || !(((GPLocations[2].col - 1) >= 0) && ((GPLocations[2].col - 1) < TetrisCol))) {
						piecesFree = 0;
					}
				}
				else {
					piecesFree += 1; // This is for piece 1 since it will be moving into 3's current position.
					if(!(GET_MATRIX_BIT(GPLocations[0].row - 2, GPLocations[0].col + 2))) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[3].row - 1, GPLocations[3].col - 1))) {
						piecesFree += 1;
					}
				}
			}
		}
		else if(BlockState==B)
		{
						
			//
			//			2	1	0			becomes		3	2
			//			3									1
			//												0
			
			if(GET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col - 1)) {
				++shiftRight;
			}
			
			if((GPLocations[0].row > 1) && (GPLocations[0].col < TetrisCol)) {
				if((GPLocations[2].col == 0) || shiftRight) {
					piecesFree +=1; // This is for piece 3 since piece 2 occupies the spot 3 moves into.
					if(!(GET_MATRIX_BIT(GPLocations[0].row - 2, GPLocations[0].col - 1))) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[1].row - 1, GPLocations[1].col))) {
						piecesFree += 1;
					}
				}
				else {
					piecesFree += 1; // This is for piece 1 which moves into 3's current position.
					if(!(GET_MATRIX_BIT(GPLocations[0].row - 2, GPLocations[0].col - 2))) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[3].row + 1, GPLocations[3].col - 1))) {
						piecesFree +=1;
					}
				}
			}
		}
		else if(BlockState==C)
		{
						
						
			//
			//		3	2   		becomes			     3
			//			1						 0	 1   2
			//			0
			
			if(GET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col - 2)) {
				++shiftRight;
			}
			
			if(GPLocations[0].col < TetrisCol) {
				if((GPLocations[3].col == 0) || shiftRight) {
					piecesFree += 2;
					if(!(GET_MATRIX_BIT(GPLocations[3].row + 1, GPLocations[3].col + 2))) {
						piecesFree +=1;
					}
					if(GET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col + 1) || !((GPLocations[2].col + 1) < TetrisCol)) {
						piecesFree = 0;
					}
				}
				else {
					piecesFree += 1; // This is for piece 1 which moves into piece 3's current position.
					if(!(GET_MATRIX_BIT(GPLocations[0].row + 2, GPLocations[0].col - 2))) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[3].row + 1, GPLocations[3].col + 1))) {
						piecesFree +=1;
					}
				}
			}
		}
		else if(BlockState==D)
		{
						
			//													0
			//					  3								1
			//			0	 1	  2			becomes				2	3
			//
			
			if(GET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col + 1)) {
				++shiftLeft;
			}
			
			if(GPLocations[2].col < TetrisCol) {
				if((GPLocations[2].col == TetrisCol - 1) || shiftLeft) {
					piecesFree += 1;
					if(!(GET_MATRIX_BIT(GPLocations[0].row + 2, GPLocations[0].col + 1))) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[1].row + 1, GPLocations[1].col))) {
						piecesFree += 1;
					}
				}
				else {
					piecesFree += 1; // This is for piece 1 since it will be moving into 3's current position.
					if(!(GET_MATRIX_BIT(GPLocations[0].row + 2, GPLocations[0].col + 2))) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[3].row - 1, GPLocations[3].col + 1))) {
						piecesFree +=1;
					}
				}
			}
		}
	}
				
				
	else if(BlockHolder == 6) { // S
		if(BlockState==A)
		{
			//		*	2	3					*	0	*
			//		0	1	*		becomes		*	1	2
			//		*	*	*					*	*	3
			
			if((GPLocations[0].row > 0) && (GPLocations[3].col < TetrisCol)) {
				piecesFree += 1; // This is for piece 0 since it will take piece 2's position.
				if(!(GET_MATRIX_BIT(GPLocations[2].row - 1, GPLocations[2].col + 1))) {
					piecesFree += 1;
				}
				if(!(GET_MATRIX_BIT(GPLocations[3].row - 2, GPLocations[3].col))) {
					piecesFree +=1;
				}
			}
		}
		else if(BlockState==B)
		{
			//		*	0	*					*	2	3
			//		*	1	2		becomes		0	1	*
			//		*	*	3					*	*	*
			
			if(GET_MATRIX_BIT(GPLocations[1].row, GPLocations[1].col - 1)) {
				++shiftRight;
			}
			
			if((GPLocations[1].row > 0) && (GPLocations[1].col < TetrisCol)) {
				if((GPLocations[0].col == 0) || shiftRight) {
					piecesFree += 1; // This is for piece 0 since it will take piece 1's position.
					if(!(GET_MATRIX_BIT(GPLocations[2].row + 1, GPLocations[2].col))) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[3].row + 2, GPLocations[3].col + 1)) && (((GPLocations[3].col + 1) >= 0) && ((GPLocations[3].col + 1) < TetrisCol))) {
						piecesFree +=1;
					}
				}
				else {
					piecesFree += 1; // This is for piece 2 which takes piece 0's position.
					if(!(GET_MATRIX_BIT(GPLocations[0].row - 1, GPLocations[0].col - 1))) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[3].row + 2, GPLocations[3].col))) {
						piecesFree +=1;
					}
				}
			}
		}
	}
				
	else if(BlockHolder == 7) { // Z
		if(BlockState==A)
		{
			//		0	1	*					*	*	0
			//		*	2	3		becomes		*	2	1
			//		*	*	*					*	3	*
			
			if((GPLocations[3].row > 0) && (GPLocations[3].col < TetrisCol)) {
				piecesFree += 1; // This is for piece 1 which takes 3's current position.
				if(!(GET_MATRIX_BIT(GPLocations[0].row, GPLocations[0].col + 2))) {
					piecesFree += 1;
				}
				if(!(GET_MATRIX_BIT(GPLocations[3].row - 1, GPLocations[3].col - 1))) {
					piecesFree +=1;
				}
			}
		}
		else if(BlockState==B)
		{
			//		*	*	0					0	1	*
			//		*	2	1		becomes		*	2	3
			//		*	3	*					*	*	*
			
			if(GET_MATRIX_BIT(GPLocations[2].row + 1, GPLocations[2].col - 1)) {
				++shiftRight;
			}
			
			if((GPLocations[2].row > 0) && (GPLocations[1].col < TetrisCol)) {
				if((GPLocations[2].col == 0) || shiftRight) {
					piecesFree += 1;
					if(!(GET_MATRIX_BIT(GPLocations[0].row, GPLocations[0].col - 1))) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[3].row + 1, GPLocations[3].col + 2)) && (((GPLocations[3].col + 2) >= 0) && ((GPLocations[3].col + 2) < TetrisCol))) {
						piecesFree +=1;
					}
				}
				else {
					piecesFree += 1; // This is for piece 3 which takes piece 1's current position.
					if(!(GET_MATRIX_BIT(GPLocations[0].row, GPLocations[0].col - 2))) {
						piecesFree += 1;
					}
					if(!(GET_MATRIX_BIT(GPLocations[1].row + 1, GPLocations[1].col - 1))) {
						piecesFree += 1;
					}
				}
			}
		}
	}
			
	if(piecesFree == 3) {
		canOrient = 1;
	}
	else {
		canOrient = 0x00;
		shiftLeft = 0;
		shiftRight = 0;
	}
}

/* When to call: This is a state machine and should be called in the while loop.
   
   Purpose: This state machine will re-orient the falling block upon the press of a button.
      
   How to use: Call in while loop. 
   
   Notes:
*/
enum Tetris_OrientStates {SAMPLE, REORIENT};
int Orientation(int state)						//if button pressed, go into orientation
{
	static unsigned char alreadyReOr = 0x00;
	unsigned char MovePieceLeft = 0x00, MovePieceRight = 0x00;
	switch(state) {
		case SAMPLE:
		if(gameOver) {
			state = SAMPLE;
		}
		else if((PINB & 0x10) == 0x00) {
			state = REORIENT;
		}
		break;
		
		case REORIENT:
		if((PINB & 0x10) == 0x10) {
			alreadyReOr = 0x00;
			state = SAMPLE;
		}
		else {
			state = REORIENT;
		}
		break;
	}
	switch(state) {
		case SAMPLE:
		break;
		
		case REORIENT:
		checkOrientation();
		if(!alreadyReOr && canOrient) {
			if(GPLocations[0].row < TetrisRow) {SET_MATRIX_BIT(GPLocations[0].row, GPLocations[0].col, 0);}
			if(GPLocations[1].row < TetrisRow) {SET_MATRIX_BIT(GPLocations[1].row, GPLocations[1].col, 0);}
			if(GPLocations[2].row < TetrisRow) {SET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col, 0);}
			if(GPLocations[3].row < TetrisRow) {SET_MATRIX_BIT(GPLocations[3].row, GPLocations[3].col, 0);}
			if(BlockHolder==1)
			{
				checkOrientation();
				if(BlockState==A && canOrient)
				{
					//				1								0
					//		0		2		3		becomes			2	1
					//												3
					GPLocations[0].row += 1;
					GPLocations[0].col += 1;
					GPLocations[1].row -= 1;
					GPLocations[1].col += 1;
					GPLocations[3].row -= 1;
					GPLocations[3].col -= 1;
					BlockState=B;
				}
				else if(BlockState==B && canOrient)
				{
					//		0
					//		2	1			becomes			3	2	0
					//		3									1
					
					
					
					
					if(GPLocations[0].col == 0 || shiftRight) {
						GPLocations[0].row -= 1;
						GPLocations[0].col += 2;
						GPLocations[1].row -= 1;
						GPLocations[2].col += 1;
						GPLocations[3].row += 1;
					}
					else {
						GPLocations[0].row -= 1;
						GPLocations[0].col += 1;
						GPLocations[1].row -= 1;
						GPLocations[1].col -= 1;
						GPLocations[3].row += 1;
						GPLocations[3].col -= 1;
					}
					
					BlockState=C;
				}
				else if(BlockState==C && canOrient)
				{
					//											3
					//			3	2	0	becomes			1	2
					//			 	1							0
					
					GPLocations[0].row -= 1;
					GPLocations[0].col -= 1;
					GPLocations[1].row += 1;
					GPLocations[1].col -= 1;						
					GPLocations[3].row += 1 ;
					GPLocations[3].col += 1;
					
					BlockState=D;
				}
				else if(BlockState==D && canOrient)
				{
					//				3							1
					//			1	2		becomes			0	2   3
					//			 	0
					
					if(GPLocations[0].col == TetrisCol - 1 || shiftLeft) {
						GPLocations[0].row += 1;
						GPLocations[0].col -= 2;
						GPLocations[1].row += 1;
						GPLocations[2].col -= 1;
						GPLocations[3].row -= 1;
					}
					else {
						GPLocations[0].row += 1;
						GPLocations[0].col -= 1;
						GPLocations[1].row += 1;
						GPLocations[1].col += 1;
						GPLocations[3].row -= 1;
						GPLocations[3].col += 1;
					}
					
					
					
					BlockState=A;
				}
			}
			else if(BlockHolder ==2) {} // Cannot re-orient the square(O) piece
			
			else if(BlockHolder==3) { //if piece is I
				checkOrientation();
				if(BlockState==A && canOrient)
				{
					
					
					//				0
					//				1				becomes
					//				2								3 2 1 0
					//				3
					
					if(GPLocations[0].col == 0 || shiftRight) {
						MovePieceRight = 1;
						GPLocations[0].row -= 2;
						GPLocations[0].col += 3;
						GPLocations[1].row -= 1;
						GPLocations[1].col += 2;
						GPLocations[2].col += MovePieceRight;
						GPLocations[3].row += 1;
					}
					else if(GPLocations[0].col == TetrisCol - 2 || shiftLeft == 1) {
						MovePieceLeft = 1;
						GPLocations[0].row -= 2;
						GPLocations[0].col += 1;
						GPLocations[1].row -= 1;
						GPLocations[2].col -= MovePieceLeft;
						GPLocations[3].row += 1;
						GPLocations[3].col -= 2;
					}
					else if(GPLocations[0].col == TetrisCol - 1 || shiftLeft == 2) {
						MovePieceLeft = 2;
						GPLocations[0].row -= 2;
						GPLocations[1].row -= 1;
						GPLocations[1].col -= 1;
						GPLocations[2].col -= MovePieceLeft;
						GPLocations[3].row += 1;
						GPLocations[3].col -= 3;
					}
					else {
						GPLocations[0].row -= 2;
						GPLocations[0].col += 2;
						GPLocations[1].row -= 1;
						GPLocations[1].col += 1;
						GPLocations[3].row += 1;
						GPLocations[3].col -= 1;
					}
					
					
					BlockState=B;
				}
				else if(BlockState==B && canOrient)
				{
					
					//												0
					//								becomes			1
					//			3 2  1 0							2
					//												3
					
					GPLocations[0].row += 2;
					GPLocations[0].col -= 2;
					GPLocations[1].row += 1;
					GPLocations[1].col -= 1;
					GPLocations[3].row -= 1;
					GPLocations[3].col += 1;
					
					BlockState=A;
				}
			}


			else if(BlockHolder==4) { // J piece
				checkOrientation();
				if(BlockState==A && canOrient)
				{
					
					
					//				0
					//				1				becomes				  3
					//			3	2									  2  1  0
					//
					
					if((GPLocations[0].col == TetrisCol - 2) || (shiftLeft == 1)) {
						GPLocations[0].row -= 2;
						GPLocations[0].col += 1;
						GPLocations[1].row -= 1;
						GPLocations[2].col -= 1;
						GPLocations[3].row += 1;
					}
					else if((GPLocations[0].col == TetrisCol - 1) || (shiftLeft == 2)) {
						GPLocations[0].row -= 2;
						GPLocations[1].row -= 1;
						GPLocations[1].col -= 1;
						GPLocations[2].col -= 2;
						GPLocations[3].row += 1;
						GPLocations[3].col -= 1;
					}
					else {
						GPLocations[0].row -= 2;
						GPLocations[0].col += 2;
						GPLocations[1].row -= 1;
						GPLocations[1].col += 1;
						GPLocations[3].row += 1 ;
						GPLocations[3].col += 1;
					}
					
					BlockState=B;
					
					
				}
				else if(BlockState==B && canOrient)
				{
					
					//			3
					//			2	1	0			becomes			2	3
					//												1
					//												0
					
					
					GPLocations[0].row -= 2;
					GPLocations[0].col -= 2;
					GPLocations[1].row -= 1;
					GPLocations[1].col -= 1;
					GPLocations[3].row -= 1 ;
					GPLocations[3].col += 1;
					
					BlockState=C;
				}
				else if(BlockState==C && canOrient)
				{
					
					
					//
					//			2   3		becomes      0	 1   2
					//			1								 3
					//			0
					
					if((GPLocations[0].col == 0) || (shiftRight == 2)) {
						GPLocations[0].row += 2;
						GPLocations[1].row += 1;
						GPLocations[1].col += 1;
						GPLocations[2].col += 2;
						GPLocations[3].row -= 1;
						GPLocations[3].col += 1;
					}
					else if((GPLocations[0].col == 1) || (shiftRight == 1)) {
						GPLocations[0].row += 2;
						GPLocations[0].col -= 1;
						GPLocations[1].row += 1;
						GPLocations[2].col += 1;
						GPLocations[3].row -= 1;
					}
					else {
						GPLocations[0].row += 2;
						GPLocations[0].col -= 2;
						GPLocations[1].row += 1;
						GPLocations[1].col -= 1;
						GPLocations[3].row -= 1;
						GPLocations[3].col -= 1;
					}
					
					BlockState=D;
					
					
				}
				else if(BlockState==D && canOrient)
				{
					
					//													0
					//													1
					//			0	 1	  2			becomes			3	2
					//					  3
					
					
					GPLocations[0].row += 2;
					GPLocations[0].col += 2;
					GPLocations[1].row += 1;
					GPLocations[1].col += 1;
					GPLocations[3].row += 1 ;
					GPLocations[3].col -= 1;
					
					BlockState=A;
				}
			}
			
			else if(BlockHolder==5) { // L piece
				checkOrientation();
				if(BlockState==A && canOrient)
				{
					
					
					//			0
					//			1					becomes				  2	 1  0
					//			2	3									  3
					//
					
					if((GPLocations[3].col == TetrisCol - 1) || shiftLeft) {
						GPLocations[0].row -= 2;
						GPLocations[0].col += 1;
						GPLocations[1].row -= 1;
						GPLocations[2].col -= 1;
						GPLocations[3].row -= 1;
						GPLocations[3].col -= 2;
					}
					else {
						GPLocations[0].row -= 2;
						GPLocations[0].col += 2;
						GPLocations[1].row -= 1;
						GPLocations[1].col += 1;
						GPLocations[3].row -= 1;
						GPLocations[3].col -= 1;
					}
					
					BlockState=B;
					
					
				}
				else if(BlockState==B && canOrient)
				{
					
					//
					//			2	1	0			becomes		3	2
					//			3									1
					//												0
					
					if((GPLocations[3].col == 0) || shiftRight) {
						GPLocations[0].row -= 2;
						GPLocations[0].col -= 1;
						GPLocations[1].row -= 1;
						GPLocations[2].col += 1;
						GPLocations[3].row += 1;
					}
					else {
						GPLocations[0].row -= 2;
						GPLocations[0].col -= 2;
						GPLocations[1].row -= 1;
						GPLocations[1].col -= 1;
						GPLocations[3].row += 1;
						GPLocations[3].col -= 1;
					}
					
					BlockState=C;
				}
				else if(BlockState==C && canOrient)
				{
					
					
					//
					//		3	2   		becomes			     3
					//			1						 0	 1   2
					//			0
					
					if((GPLocations[3].col == 0) || shiftRight) {
						GPLocations[0].row += 2;
						GPLocations[0].col -= 1;
						GPLocations[1].row += 1;
						GPLocations[2].col += 1;
						GPLocations[3].row += 1;
						GPLocations[3].col += 2;
					}
					else {
						GPLocations[0].row += 2;
						GPLocations[0].col -= 2;
						GPLocations[1].row += 1;
						GPLocations[1].col -= 1;
						GPLocations[3].row += 1;
						GPLocations[3].col += 1;
					}
					
					BlockState=D;
					
					
				}
				else if(BlockState==D && canOrient)
				{
					
					//													0
					//					  3								1
					//			0	 1	  2			becomes				2	3
					//
					
					if((GPLocations[3].col == TetrisCol - 1) || shiftLeft) {
						GPLocations[0].row += 2;
						GPLocations[0].col += 1;
						GPLocations[1].row += 1;
						GPLocations[2].col -= 1;
						GPLocations[3].row -= 1;
					}
					else {
						GPLocations[0].row += 2;
						GPLocations[0].col += 2;
						GPLocations[1].row += 1;
						GPLocations[1].col += 1;
						GPLocations[3].row -= 1;
						GPLocations[3].col += 1;
					}
					
					BlockState=A;
				}
			}
			
			
			else if(BlockHolder == 6) { // S
				checkOrientation();
				if(BlockState==A && canOrient)
				{
					//		*	2	3					*	0	*
					//		0	1	*		becomes		*	1	2
					//		*	*	*					*	*	3
					
					GPLocations[0].row += 1;
					GPLocations[0].col += 1;
					GPLocations[2].row -= 1;
					GPLocations[2].col += 1;
					GPLocations[3].row -= 2 ;

					BlockState=B;
				}
				else if(BlockState==B && canOrient)
				{
					//		*	0	*					*	2	3
					//		*	1	2		becomes		0	1	*
					//		*	*	3					*	*	*
					
					if((GPLocations[0].col == 0) || shiftRight) {
						GPLocations[0].row -= 1;
						GPLocations[1].col += 1;
						GPLocations[2].row += 1;
						GPLocations[3].row += 2;
						GPLocations[3].col += 1;
					}
					else {
						GPLocations[0].row -= 1;
						GPLocations[0].col -= 1;
						GPLocations[2].row += 1;
						GPLocations[2].col -= 1;
						GPLocations[3].row += 2;
					}
					
					BlockState=A;
				}
			}
			
			else if(BlockHolder == 7) { // Z
				checkOrientation();
				if(BlockState==A && canOrient)
				{
					//		0	1	*					*	*	0
					//		*	2	3		becomes		*	2	1
					//		*	*	*					*	3	*
					
					GPLocations[0].col += 2;
					GPLocations[1].row -= 1;
					GPLocations[1].col += 1;						
					GPLocations[3].row -= 1;
					GPLocations[3].col -= 1;
					
					BlockState=B;
				}
				else if(BlockState==B && canOrient)
				{
					//		*	*	0					0	1	*
					//		*	2	1		becomes		*	2	3
					//		*	3	*					*	*	*
						
					if((GPLocations[2].col == 0) || shiftRight) {
						GPLocations[0].col -= 1;
						GPLocations[1].row += 1;
						GPLocations[2].col += 1;
						GPLocations[3].row += 1;
						GPLocations[3].col += 2;
					}
					else {
						GPLocations[0].col -= 2;
						GPLocations[1].row += 1;
						GPLocations[1].col -= 1;
						GPLocations[3].row += 1;
						GPLocations[3].col += 1;
					}
					
					BlockState=A;
				}
			}
			if(GPLocations[0].row < TetrisRow) {SET_MATRIX_BIT(GPLocations[0].row, GPLocations[0].col, 1);}
			if(GPLocations[1].row < TetrisRow) {SET_MATRIX_BIT(GPLocations[1].row, GPLocations[1].col, 1);}
			if(GPLocations[2].row < TetrisRow) {SET_MATRIX_BIT(GPLocations[2].row, GPLocations[2].col, 1);}
			if(GPLocations[3].row < TetrisRow) {SET_MATRIX_BIT(GPLocations[3].row, GPLocations[3].col, 1);}
			shiftLeft = 0;
			shiftRight = 0;
			canOrient = 0;
			++alreadyReOr;
		}
		break;
		
		
		default:
		break;
	}
	return state;
}

/* When to call: This function is called in the Tetris SM and creates a new block after a block can no longer fall.
   
   Purpose: Generates a new block.
   
   How to use: Call in Tetris SM and creates a new block and puts it on the screen. 
   
   Notes:
*/
void GenerateBlock(unsigned char block)
{
	BlockHolder=block;
	if(block==1)
	{
		//T
		BlockState=A;
		
		GPLocations[0].row = TetrisRow;
		GPLocations[0].col = 3;
		GPLocations[1].row = TetrisRow + 1;
		GPLocations[1].col = 4;
		GPLocations[2].row = TetrisRow;
		GPLocations[2].col = 4;
		GPLocations[3].row = TetrisRow;
		GPLocations[3].col = 5;
	}
	
	if(block==2)
	{
		//square
		// O
		BlockState=A;
		GPLocations[0].row = TetrisRow + 1;
		GPLocations[0].col = 3;
		GPLocations[1].row = TetrisRow + 1;
		GPLocations[1].col = 4;
		GPLocations[2].row = TetrisRow;
		GPLocations[2].col = 3;
		GPLocations[3].row = TetrisRow;
		GPLocations[3].col = 4;
	}
	if(block==3)
	{
		// I
		BlockState=A;
		GPLocations[0].row = TetrisRow + 3;
		GPLocations[0].col = 3;
		GPLocations[1].row = TetrisRow + 2;
		GPLocations[1].col = 3;
		GPLocations[2].row = TetrisRow + 1;
		GPLocations[2].col = 3;
		GPLocations[3].row = TetrisRow;
		GPLocations[3].col = 3;
	}
	if(block==4)
	{
		// J
		BlockState=A;
		GPLocations[0].row = TetrisRow + 2;
		GPLocations[0].col = 4;
		GPLocations[1].row = TetrisRow + 1;
		GPLocations[1].col = 4;
		GPLocations[2].row = TetrisRow;
		GPLocations[2].col = 4;
		GPLocations[3].row = TetrisRow;
		GPLocations[3].col = 3;
	}
	if(block==5)
	{
		// L
		BlockState=A;
		GPLocations[0].row = TetrisRow + 2;
		GPLocations[0].col = 3;
		GPLocations[1].row = TetrisRow + 1;
		GPLocations[1].col = 3;
		GPLocations[2].row = TetrisRow;
		GPLocations[2].col = 3;
		GPLocations[3].row = TetrisRow;
		GPLocations[3].col = 4;
	}
	if(block==6)
	{
		// S
		BlockState=A;
		GPLocations[0].row = TetrisRow;
		GPLocations[0].col = 3;
		GPLocations[1].row = TetrisRow;
		GPLocations[1].col = 4;
		GPLocations[2].row = TetrisRow + 1;
		GPLocations[2].col = 4;
		GPLocations[3].row = TetrisRow + 1;
		GPLocations[3].col = 5;
	}
	if(block==7)
	{
		// Z
		BlockState=A;
		GPLocations[0].row = TetrisRow + 1;
		GPLocations[0].col = 3;
		GPLocations[1].row = TetrisRow + 1;
		GPLocations[1].col = 4;
		GPLocations[2].row = TetrisRow;
		GPLocations[2].col = 4;
		GPLocations[3].row = TetrisRow;
		GPLocations[3].col = 5;
	}
}
