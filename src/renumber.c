/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * renumber.c:
 *	Renumber your program!
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

#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/time.h>

#include <unistd.h>

#include "rtb.h"
#include "bomb.h"

#include "screenKeyboard.h"

#include "bool.h"
#include "keywords.h"
#include "lines.h"
#include "symbolTable.h"
#include "shuntingYard.h"
#include "rpnEval.h"

#include "renumber.h"

/*
 * updateTargets:
 *	Scan the symbol table for GOTO, GOSUB and RESTOREs that point to the
 *	current line and updates them as required.
 *********************************************************************************
 */

static void updateTargets (int this, int new)
{
  struct symbolTableStruct *st ;
  int symbol ;

  for (symbol = 0 ; symbol < numSymbols ; ++symbol)
  {
    st = &symbolTable [symbol] ;
    if ((st->type == TK_SYM_GOTO) || (st->type == TK_SYM_GOSUB))
      if (st->value.lineNumber == this)
	st->value.lineNumber = new ;
  }

}


static void renumber (int start, int inc, int startPtr, int endPtr)
{
  struct lineNumberStruct *p ;
  int thisNumber, newNumber ;
  int line ;

  newNumber = start ;
  for (line = startPtr ; line <= endPtr ; ++line)
  {
    p = &programLines [line] ;
    thisNumber = p->lineNumber ;
    updateTargets (thisNumber, newNumber) ;
    p->lineNumber = newNumber ;
    newNumber += inc ;
  }

}


/*
 * doRenumberCommand:
 *	RENUMBER start {,inc} {,lineStart} {,lineEnd}
 *********************************************************************************
 */

void doRenumberCommand (int argc, char *argv [])
{
  int start     = 100 ;
  int inc       =  10 ;
  int startLine =   0 ;
  int endLine   =   0 ;
  int startPtr, endPtr ;

// Check args..

  if (argc > 5)
    return (void)syntaxError ("RENUMBER: Usage start increment line-start line-end") ;

  if (argc > 1) start     = atoi (argv [1]) ;
  if (argc > 2) inc       = atoi (argv [2]) ;
  if (argc > 3) startLine = atoi (argv [3]) ;
  if (argc > 4) endLine   = atoi (argv [4]) ;

  if ((start < 1) || (start > 50000))
    return (void)syntaxError ("RENUMBER: Start must be between 1 and 50000") ;

  if ((inc < 1) || (inc > 100))
    return (void)syntaxError ("RENUMBER: Increment must be between 1 and 50000") ;

  if (startLine == 0)
    startPtr = 0 ;
  else if ((startPtr = findLine (startLine)) == -1)
    return (void)syntaxError ("RENUMBER: Line %d does not exist", startLine) ;

  if (endLine == 0)
    endPtr = numLines ;
  else if ((endPtr = findLine (endLine)) == -1)
    return (void)syntaxError ("RENUMBER: Line %d does not exist", endLine) ;

  renumber (start, inc, startPtr, endPtr) ;

  sortLineNumbers () ;
}
