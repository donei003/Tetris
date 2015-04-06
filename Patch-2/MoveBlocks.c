/* When to call: Should only be called in Tetris SM.
   
   Purpose: A helper function which sets a control bit high if a block can fall 
   and low if the block has either reached the bottom or another block is preventing
   it to fall.
      
   How to use: Call checkBlockFall() and then check if canFall is 0 or 1. 
   
   Notes: If 1 then the block can fall. If 0 it cannot.
*/
void checkBlockFall() {
	unsigned char piecesFree = 0x00;
	for(unsigned char i = 0; i < 4; ++i) {
		if(GPLocations[i].row == 0x00) {// This is our current way of stopping a block from moving down
			// This should also set a bit to enable the block generator
			continue;
		}
		if(GET_MATRIX_BIT(GPLocations[i].row - 1, GPLocations[i].col)) {
			for(unsigned char j = 0; j < 4; ++j) {
				if(j == i) {
					continue;
				}
				if(((GPLocations[i].row - 1) == GPLocations[j].row) && (GPLocations[i].col == GPLocations[j].col)) {
					++piecesFree;
					break;
				}
			}
		}
		else {
			++piecesFree;
		}
	}
	if(piecesFree == 0x04) {
		canFall = 0x01;
	}
	else {
		canFall = 0x00;
	}
}

/* When to call: Is called in the JoyStick SM but can be used elsewhere.
   
   Purpose: A helper function which sets a control bit high if a block can move left 
   and low if the block has either reached column zero or another block is preventing
   it from moving.
      
   How to use: Call checkMoveLeft() and then check if canMoveLeft is 0 or 1. 
   
   Notes:
*/
void checkMoveLeft() {
	unsigned char piecesFree = 0x00;
	for(unsigned char i = 0; i < 4; ++i) {
		//GPLocations[i].row == 0x00 || 
		if(GPLocations[i].col == 0x00) {// This is our current way of stopping a block from moving down
			// This should also set a bit to enable the block generator
			break;
		}
		if(GPLocations[i].row >= TetrisRow && GPLocations[i].col > 0) {
			++piecesFree;
		}
		else if(GET_MATRIX_BIT(GPLocations[i].row, GPLocations[i].col - 1)) {
			for(unsigned char j = 0; j < 4; ++j) {
				if(j == i) {
					continue;
				}
				if(((GPLocations[i].row) == GPLocations[j].row) && ((GPLocations[i].col - 1) == GPLocations[j].col)) {
					++piecesFree;
					break;
				}
			}
		}
		else {
			++piecesFree;
		}
	}
	if(piecesFree == 0x04) {
		canMoveLeft = 0x01;
	}
	else {
		canMoveLeft = 0x00;
	}
}

/* When to call: Is called in the JoyStick SM but can be used elsewhere.
   
   Purpose: A helper function which sets a control bit high if a block can move right 
   and low if the block has either reached column seven or another block is preventing
   it from moving.
      
   How to use: Call checkMoveRight() and then check if canMoveRight is 0 or 1. 
   
   Notes:
*/
void checkMoveRight() {
	unsigned char piecesFree = 0x00;
	for(unsigned char i = 0; i < 4; ++i) {
		//GPLocations[i].row == 0x00 ||
		if( GPLocations[i].col == TetrisCol - 1) {// This is our current way of stopping a block from moving down
			// This should also set a bit to enable the block generator
			break;
		}
		if(GPLocations[i].row >= TetrisRow && GPLocations[i].col < TetrisCol - 1) {
			++piecesFree;
		}
		else if(GET_MATRIX_BIT(GPLocations[i].row, GPLocations[i].col + 1)) {
			for(unsigned char j = 0; j < 4; ++j) {
				if(j == i) {
					continue;
				}
				if(((GPLocations[i].row) == GPLocations[j].row) && ((GPLocations[i].col + 1) == GPLocations[j].col)) {
					++piecesFree;
					break;
				}
			}
		}
		else {
			++piecesFree;
		}
	}
	if(piecesFree == 0x04) {
		canMoveRight = 0x01;
	}
	else {
		canMoveRight = 0x00;
	}
}

/* When to call: Is called in the JoyStick SM but can be used elsewhere.
   
   Purpose: A helper function which sets a control bit high if a block can move down 
   and low if the block has either reached row zero or another block is preventing
   it from moving.
      
   How to use: Call checkMoveDown() and then check if canMoveDown is 0 or 1. 
   
   Notes:
*/
void checkMoveDown() {
	unsigned char piecesFree = 0x00;
	for(unsigned char i = 0; i < 4; ++i) {
		if(GPLocations[i].row == 0x00) {// This is our current way of stopping a block from moving down
			// This should also set a bit to enable the block generator
			break;
		}
		if(GPLocations[i].row > TetrisRow ) {
			++piecesFree;
		}
		else if(GET_MATRIX_BIT(GPLocations[i].row - 1, GPLocations[i].col)) {
			for(unsigned char j = 0; j < 4; ++j) {
				if(j == i) {
					continue;
				}
				if(((GPLocations[i].row - 1) == GPLocations[j].row) && (GPLocations[i].col == GPLocations[j].col)) {
					++piecesFree;
					break;
				}
			}
		}
		else {
			++piecesFree;
		}
	}
	if(piecesFree == 0x04) {
		canMoveDown = 0x01;
	}
	else {
		canMoveDown = 0x00;
	}
}

void shiftBlockLeft() {
	for(unsigned char i = 0; i < 4; ++i) {
		GPLocations[i].col -= 1;
	}
}

void shiftBlockRight() {
	for(unsigned char i = 0; i < 4; ++i) {
		GPLocations[i].col += 1;
	}
}

void shiftBlockDown() {
	for(unsigned char i = 0; i < 4; ++i) {
		GPLocations[i].row -= 1;
	}
}
