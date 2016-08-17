/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * readline.h:
 ***********************************************************************
 * This file is part of RTB:
 *	Return To Basic
 *	http://projects.drogon.net/return-to-basic
 *
 *    RTB is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    RTB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RTB.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */


// Defines

#define MAX_HISTORY	50
#define MAX_INPUT_SIZE	250

// Some characters

#define CR	('\r')

#define ctrl(X) (X&037)

// Movement in a line

#define K_LEFT		KEY_LEFT
#define K_RIGHT		KEY_RIGHT

#define K_START_OF_LINE	(ctrl('A'))
#define K_END_OF_LINE	(ctrl('E'))

#define K_FIND		(ctrl('F'))
#define K_SWAP		(ctrl('S'))
#define K_DEL_LINE	(ctrl('U'))
#define K_DEL_C_UNDER	(ctrl('D'))
#define K_DEL_C_LEFT	((char)127)
#define K_ESCAPE	(ctrl('['))

// Function prototypes

extern void  setupReadline (void) ;
extern uint8_t  readChar   (void) ;
extern char    *readLine   (char *preload) ;
