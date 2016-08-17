/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * lines.h:
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

#define	MAX_LINENUM	59999
#define	MAX_LINES	 4096

extern int numLines ;

struct lineNumberStruct
{
  int       lineNumber ;
  int       length ;
  uint16_t *data ;
} ;

extern struct lineNumberStruct programLines [MAX_LINES] ;

extern void setupProgram    (void) ;
extern void sortLineNumbers (void) ;
extern void deleteProgram   (void) ;
extern int  newLineNumber   (int line) ;
extern int  findLine        (int lineNumber) ;
extern void storeLine       (int lineNumber, uint16_t *data, int length) ;
extern void deleteLine      (int lineNumber) ;
