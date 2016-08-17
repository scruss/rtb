/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * run.c:
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

#undef	DEBUG_RUN

#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

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

#include "commands.h"
#include "procedures.h"
#include "assign.h"
#include "goto.h"
#include "procFn.h"
#include "cycle.h"
#include "run.h"

// Globals

int tron     = FALSE ;
int breakOut = FALSE ;		// Used by function return in IF statement


int runKeyword (struct   keywordStruct *kw, uint16_t *p)
{
  int ok, len ;

// Instruction handling its own arguments (& linePtr)

  if ((kw->flags == KYF_0) && (kw->function != NULL))
    return kw->function (p) ;

// Standard built-in procedure

  if (((kw->flags & KYF_PROC) != 0) && (kw->function != NULL) )
  {
    if (kw->args == 0)		// No Args
    {
      if (!endOfLine (p))
	return syntaxError ("%s: Extra data", kw->keyword) ;
      ok = kw->function (p) ;
    }
    else			// One or more Args
    {
      if (!shuntingYard (p, &len))
	return FALSE ;

      p += len ;
      if (!endOfLine (p))
	return syntaxError ("%s: Extra data", kw->keyword) ;

      if (!rpnEval ())
	return FALSE ;

      ok = kw->function (NULL) ;
    }
    ++linePtr ;
    return ok ;
  }

// Pseudo variable
//	(We know it's an assignment, so can check for read only)

  if ((kw->flags & KYF_PV) != 0)
  {
    if ((kw->flags & KYF_RO) != 0)
      return syntaxError ("\"%s\" is read-only", kw->keyword) ;

    ok = doAssignment (p - 1) ;
    ++linePtr ;
    return ok ;
  }

  return syntaxError ("Unexpected keyword: \"%s\" [0x%02X]", kw->keyword, kw->flags) ;
}


/*
 * runLines:
 *	Execute the program from the given linePtr
 *	May be called recursively to cope with user-defined functions.
 *********************************************************************************
 */

int runLines (void)
{
  struct   lineNumberStruct *progLine ;
  int      running ;
  uint16_t token, symbol ;
  uint16_t *p ;

  running  = TRUE ;
  breakOut = FALSE ;

  while (running && !breakOut)
  {
    if (linePtr >= numLines)
    {
      --linePtr ;
      return syntaxError ("Unexpected end of program. (No END or STOP)") ;
    }

    progLine = &programLines [linePtr] ;
    p        = progLine->data ;

    if (escapePressed)
    {
      screenPrintf ("\nEscape at line %d\n", progLine->lineNumber) ;
      continuePtr = linePtr ;
      return FALSE ;
    }

// We're calling keyPressed here (and throwing away the result) for no
//	reason other than the test to see if the escape key has
//	been presses. Sadly, the call to SDL_PollEvent () used 
//	in keyPresses is relatively slow, so we're only going to
//	check every now and then...

    if (checkForEscape)
    {
      (void)keyPressed () ;
      checkForEscape = FALSE ;
    }

    if (tron)
    {
//      screenPrintf ("[#%d]\n", progLine->lineNumber) ;
      printf       ("[#%d]\n", progLine->lineNumber) ;
//      usleep (1000) ;
    }

// Note:
//	Compare code here with code in procedures.c: doIf()
//      If changes made here, then we may have to change that too.

    token  = *p++ ;
    symbol = token &  TK_SYM_MASK ;

// Token or Symbol?

    if (symbol == 0)			// Built-in Keyword/token
    {

// If we get an EQUALS at the start, then we have to return via
//	a call to doEndFunc as in all likliehood we're ending a
//	user-defined function/procedure and have been called recursively

      if (token == TK_EQUALS)	// Special
	return doEndFunc (p) ;

      running = runKeyword (&keywords [token], p) ;

      if (breakOut)
      {
	breakOut = FALSE ;
	return running ;
      }
    }
    else				// Symbol
    {
      switch (symbol)
      {
	case TK_SYM_REM1:
	case TK_SYM_REM2:
	  ++linePtr ;
	  break ;

	case TK_SYM_RESTORE:	running = doRestore (p) ;	break ;
	case TK_SYM_GOTO:	running = doGoto    (p) ;	break ; 
	case TK_SYM_GOSUB:	running = doGosub   (p) ;	break ;
	case TK_SYM_PROC:	running = doProc    (p) ;	break ;

	case TK_SYM_VAR_NUM:
	case TK_SYM_VAR_STR:
	  running = doAssignment (p - 1) ;
	  ++linePtr ;
	  break ;

	case TK_SYM_VAR_NUM_ARR:
	case TK_SYM_VAR_STR_ARR:
	  running = doArrayAssignment (p - 1) ;
	  ++linePtr ;
	  break ;

	default:
	  (void)syntaxError ("RUN: Unexpected symbol: 0x%04X", symbol) ;
	  running = FALSE ;
	  break ;
      }
    }
  }

  breakOut = FALSE ;
  return running ;
}


/*
 * runFrom:
 *	Run a program from a given line number
 *********************************************************************************
 */

int runFrom (int lineNumber)
{
  if (lineNumber == 0)
    linePtr = 0 ;
  else if ((linePtr = findLine (lineNumber)) == -1)
    return syntaxError ("Line %d doesn't exist", lineNumber) ;

  return runLines () ;
}
