/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * rtb.h:
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

// A handy macro...

#define	endOfLine(X)	((*X == TK_SYM_EOL) || ((*X & TK_SYM_MASK) == TK_SYM_REM2))

#define	DEFAULT_NUM_FORMAT "%-.10g"

// Globals

extern int             gotSyntaxError ;
extern int             exitRTB ;

extern int             escapePressed ;
extern int             checkForEscape ;

extern struct timeval  startTime ;
extern char           *savedFilename ;
extern int             rootRevoked ;

// Other globals needed

extern double angleConversion ;
extern int    programChanged ;
extern int    programRunning ;
extern int    linePtr ;
extern int    continuePtr ;

extern uint16_t *dataPtr,  *firstDataPtr ;
extern int       dataIndex, firstDataIndex ;

extern char fmtString [32] ;

extern char *initialFilename ;


// Functions

extern int syntaxError (char *message, ...) ;
extern void revokeRoot (void) ;
extern void regainRoot (void) ;
