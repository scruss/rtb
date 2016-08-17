/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * cycle.c:
 *	Handle the CYCLE...REPEAT stuff
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <unistd.h>

#include "rtb.h"
#include "bomb.h"
#include "bool.h"

#include "keywords.h"
#include "lines.h"
#include "symbolTable.h"
#include "shuntingYard.h"
#include "rpnEval.h"

//#include "commands.h"

#include "run.h"
#include "cycle.h"

// Locals

static int *cycleStack    = NULL ;
static int  cycleStackPtr = 0 ;
static int  cycleCount    = 0 ;

static uint16_t *forStackVar  = NULL ;
static double   *forStackEnd  = NULL ;
static double   *forStackStep = NULL ;
static int       forStackPtr  = 0 ;
static int       forCount     = 0 ;

/*
 * clearCycle:
 *	Reset things back for a clean program exit, or new run
 *********************************************************************************
 */

void clearCycle (void)
{
  forCount      = 0 ;
  cycleCount    = 0 ;
  forStackPtr   = 0 ;
  cycleStackPtr = 0 ;

  if (cycleStack != NULL)
  {
    free (cycleStack) ;   cycleStack   = NULL ;
    free (forStackVar) ;  forStackVar  = NULL ;
    free (forStackEnd) ;  forStackEnd  = NULL ;
    free (forStackStep) ; forStackStep = NULL ;
  }
}

/*
 * initCycle:
 *	Reset our stack pointer at the start of a new run
 *********************************************************************************
 */

int initCycle (void)
{
  clearCycle () ;

// Pre-Allocate the first stack chunk, to hopefully make small programs
//	run a bit smoother

    if ((cycleStack   = calloc (CYCLE_STACK_CHUNK, sizeof (int))) == NULL)
      return syntaxError ("Out of memory pre-allocating the CYCLE stack") ;

    if ((forStackVar  = calloc (FOR_STACK_CHUNK, sizeof (uint16_t))) == NULL)
      return syntaxError ("Out of memory pre-allocating the FOR(v) stack") ;

    if ((forStackEnd  = calloc (FOR_STACK_CHUNK, sizeof (double))) == NULL)
      return syntaxError ("Out of memory pre-allocating the FOR(e) stack") ;

    if ((forStackStep = calloc (FOR_STACK_CHUNK, sizeof (double))) == NULL)
      return syntaxError ("Out of memory pre-allocating the FOR(s) stack") ;

  cycleCount = CYCLE_STACK_CHUNK ;
  forCount   = FOR_STACK_CHUNK ;

  return TRUE ;
}


/*
 * pushCycleStack: popCycleStack:
 *	Manage the CYCLE...REPEAT line number stack
 *********************************************************************************
 */

static int pushCycleStack (int line)
{
  if (cycleStackPtr == cycleCount)
  {
    cycleCount += CYCLE_STACK_CHUNK ;
    if ((cycleStack = realloc (cycleStack, cycleCount * sizeof (int))) == NULL)
      bomb ("Out of memory reallocating cycleStack space", TRUE) ;
  }

  cycleStack [cycleStackPtr++] = line ;
  return TRUE ;
}

static int popCycleStack (void)
{
  if (cycleStackPtr == 0)
    return -1 ;
  else
    return cycleStack [--cycleStackPtr] ;
}


static int pushForStack (int index, double toValue, double stepValue)
{
  if (forStackPtr == forCount)
  {
    forCount += FOR_STACK_CHUNK ;
    if ((forStackVar  = realloc (forStackVar,  forCount * sizeof (uint16_t))) == NULL) return FALSE ;
    if ((forStackEnd  = realloc (forStackEnd,  forCount * sizeof (double)))   == NULL) return FALSE ;
    if ((forStackStep = realloc (forStackStep, forCount * sizeof (double)))   == NULL) return FALSE ;
  }

  forStackVar  [forStackPtr] = index ;	// We know the type
  forStackEnd  [forStackPtr] = toValue ;
  forStackStep [forStackPtr] = stepValue ;

  ++forStackPtr ;
  return TRUE ;
}


/*
 * doCycle:
 *	Remember our current place to come back to when we REPEAT
 *********************************************************************************
 */

int doCycle (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;

  if (!endOfLine (p))
    return syntaxError ("CYCLE: Extra data") ;

  if (!pushCycleStack (linePtr))
    return syntaxError ("Too many LOOPs") ;

  ++linePtr ;
  return TRUE ;
}


/*
 * forNextCheck:
 *	Check to see if we're inside a FOR loop
 *	If we are, then we evaluate the loop test to work out if we're doing
 *	the loop again, or if it's time to exit the loop.
 *********************************************************************************
 */

static int forNextCheck (int *newLinePtr, int indexCheck)
{
  uint16_t  index ;
  double    result, toValue, stepValue ;
  int       x ;

  if (*programLines [*newLinePtr].data != TK_FOR)
    return TRUE ;

  if (forStackPtr == 0)
    return syntaxError ("REPEAT/NEXT without FOR") ;

  x = forStackPtr - 1 ;
  
  index     = forStackVar  [x] ;
  toValue   = forStackEnd  [x] ;
  stepValue = forStackStep [x] ;

  if (indexCheck != -1)
    if (indexCheck != index)
      return syntaxError ("NEXT: index variable mismatch") ;

  if (!getRealVar (index, &result))	// Out of the symbol table
    return FALSE ;

  result += stepValue ;

  if (stepValue > 0.0)
  {
    if (result > toValue)
    {
      --forStackPtr ;
      *newLinePtr = linePtr + 1 ;	// Jump to next line
      return TRUE ;
    }
  }
  else
  {
    if (result < toValue)
    {
      --forStackPtr ;
      *newLinePtr = linePtr + 1 ;	// Jump to next line
      return TRUE ;
    }
  }

// Still here... update the numbers and return to the line *after* the FOR

  if (!pushCycleStack (*newLinePtr))
    return syntaxError ("Too many FORs") ;

  storeRealVar (index, result) ;	// Into the symbol table
  *newLinePtr = *newLinePtr + 1 ;
  return TRUE ;
}


/*
 * doRepeat:
 *	Loop back to a CYCLE
 *	repeat [[while | until] (condition)]
 *********************************************************************************
 */

int doRepeat (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  uint16_t  doing ;
  int       newLinePtr ;
  int       len, test, check ;
  double    wuTrue ;

  newLinePtr = popCycleStack () ;
  if (newLinePtr == -1)
    return syntaxError ("REPEAT without CYCLE/DO/FOR") ;

  if ((*p == TK_UNTIL) || (*p == TK_WHILE))
  {
    doing = *p++ ;
    if (!shuntingYard (p, &len))
      return FALSE ;
    p += len ;

    if (!endOfLine (p))
      return syntaxError ("REPEAT: Extra data after UNTIL/WHILE") ;

    if (!rpnEvalNum (&wuTrue))
      return FALSE ;

    test  = (doing == TK_UNTIL) ? 0 : 1 ;
    check = ((int)wuTrue == 0)  ? 0 : 1 ;

    if (check == test)		// 0 is FALSE, anything else considered TRUE
    {
      if (!forNextCheck (&newLinePtr, -1))
	return FALSE ;
      linePtr = newLinePtr ;
      return TRUE ;
    }
    ++linePtr ;
    return TRUE ;
  }

  if (!endOfLine (p))
    return syntaxError ("REPEAT: Extra data") ;

  if (!forNextCheck (&newLinePtr, -1))
    return FALSE ;
  linePtr = newLinePtr ;
  return TRUE ;
}


/*
 * doNext:
 *	Special version of repeat for FOR loops
 *	NEXT [variable]
 *********************************************************************************
 */

int doNext (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  int       newLinePtr ;
  uint16_t  variable, type, index ;

  newLinePtr = popCycleStack () ;
  if (newLinePtr == -1)
    return syntaxError ("NEXT without FOR") ;

  if (endOfLine (p))			// Simple case
  {
    if (!forNextCheck (&newLinePtr, -1))
      return FALSE ;

    linePtr = newLinePtr ;
    return TRUE ;
  }

  variable = *p++ ;
  type     = variable &  TK_SYM_MASK ;
  index    = variable & ~TK_SYM_MASK ;

  if (type != TK_SYM_VAR_NUM)
    return syntaxError ("NEXT: Numeric variable expected") ;

  if (!forNextCheck (&newLinePtr, index))
    return FALSE ;

  linePtr = newLinePtr ;
  return TRUE ;
}




/*
 * doContinue:
 *	Loop back to a CYCLE before hitting the REPEAT
 *********************************************************************************
 */

int doContinue (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  int       newLinePtr ;

  if (!endOfLine (p))
    return syntaxError ("CONTINUE: Extra data") ;

  newLinePtr = popCycleStack () ;
  if (newLinePtr == -1)
    return syntaxError ("CONTINUE without CYCLE/DO/FOR") ;

  if (!forNextCheck (&newLinePtr, -1))
    return FALSE ;

  linePtr = newLinePtr ;
  return TRUE ;
}

/*
 * doBreak:
 *	Break out of a CYCLE loop before hitting the REPEAT
 *********************************************************************************
 */

int doBreak (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  uint16_t *x ;
  int       newLinePtr ;
  int       loopCount = 0 ;

  if (!endOfLine (p))
    return syntaxError ("BREAK: Extra data") ;

// We're discarding the newLinePtr, but we need to check it first...

  newLinePtr = popCycleStack () ;
  if (newLinePtr == -1)
    return syntaxError ("BREAK without CYCLE/DO/FOR") ;

// need to see if we were inside a FOR loop though:

  if (*programLines [newLinePtr].data == TK_FOR)
  {
    if (forStackPtr == 0)
      return syntaxError ("BREAK: FOR Loop count mismatch") ;
    else
      --forStackPtr ;
  }

// Now scan from the current line to the first line with a REPEAT token in it
//	However, as we go, we need to scan for more CYCLE...REPEAT loops...

  for (newLinePtr = linePtr + 1 ; newLinePtr < numLines ; ++newLinePtr)
  {
    x = programLines [newLinePtr].data ;
    if (*x == TK_REPEAT)
    {
      if (loopCount == 0)
      {
	linePtr = newLinePtr + 1 ;
	return TRUE ;
      }
      else
	--loopCount ;
      continue ;
    }
    while (!endOfLine (x))
    {
      if ((*x == TK_CYCLE) || (*x == TK_DO))
	++loopCount ;
      ++x ;
    }

  }
  return syntaxError ("BREAK: No REPEAT") ;
}


/*
 * doDo:
 *	Slightly different syntax to the CYCLE construct:
 *	do [[while | until] (condition)]
 *********************************************************************************
 */

int doDo (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  int       len ;
  double    testVal ;
  int       iTestVal, testResult ;

// On its own:

  if (endOfLine (p))
  {
    if (!pushCycleStack (linePtr))
      return syntaxError ("Too many LOOPs") ;

    ++linePtr ;
    return TRUE ;
  }

  if ((*p != TK_WHILE) && (*p != TK_UNTIL))
    return syntaxError ("DO: WHILE or UNTIL expected") ;

  if (*p == TK_WHILE)
    testResult = 1 ;
  else
    testResult = 0 ;

  ++p ;
  if (!shuntingYard (p, &len))
    return FALSE ;

  p += len ;

  if (!endOfLine (p))
    return syntaxError ("DO: Extra data after WHILE or UNTIL") ;

  if (!rpnEvalNum (&testVal))
    return FALSE ;

  iTestVal = (int)testVal ;
  if (iTestVal != 0)
    iTestVal = 1 ;

  if (iTestVal == testResult)	// 0 is FALSE, anything else considered TRUE
    if (pushCycleStack (linePtr))
    {
      ++linePtr ;
      return TRUE ;
    }
    else
      return syntaxError ("Too many LOOPs") ;
  else
  {
    if (pushCycleStack (linePtr))	// Big Cheat here - pretend to do another CYCLE, then BREAK...
      return doBreak (p) ;
    else
      return syntaxError ("Too many LOOPs") ;
  }

//  return syntaxError ("DO: Extra data") ;
}



/*
 * doWhile:
 *	Similar to IF, but controlling a loop
 *********************************************************************************
 */

int doWhile (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  int       len ;
  double    whileTrue ;

  if (!shuntingYard (p, &len))
    return FALSE ;

  p += len ;
  if (*p != TK_CYCLE)
    return syntaxError ("WHILE: Missing CYCLE") ;

  ++p ;

  if (!endOfLine (p))
    return syntaxError ("WHILE: Extra data after CYCLE") ;

  if (!rpnEvalNum (&whileTrue))
    return FALSE ;

  if ((int)whileTrue != 0)	// 0 is FALSE, anything else considered TRUE
    return doCycle (p) ;
  else
  {
    if (pushCycleStack (linePtr))	// Big Cheat here - pretend to do another CYCLE, then BREAK...
      return doBreak (p) ;
    else
      return syntaxError ("Too many CYCLEs") ;
  }
}


/*
 * doUntil:
 *	Same as WHILE, but the opposite check
 *	I ought to merge the code somehow...
 *********************************************************************************
 */

int doUntil (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  int       len ;
  double    whileTrue ;

  if (!shuntingYard (p, &len))
    return FALSE ;

  p += len ;
  if (*p != TK_CYCLE)
    return syntaxError ("UNTIL: Missing CYCLE") ;

  ++p ;

  if (!endOfLine (p))
    return syntaxError ("UNTIL: Extra data after CYCLE") ;

  if (!rpnEvalNum (&whileTrue))
    return FALSE ;

  if ((int)whileTrue == 0)	// 0 is FALSE, anything else considered TRUE
    return doCycle (p) ;
  else
  {
    if (pushCycleStack (linePtr))	// Big Cheat here - pretend to do another CYCLE, then BREAK...
      return doBreak (p) ;
    else
      return syntaxError ("Too many CYCLEs") ;
  }
}


/*
 * doFor:
 *	The FOR loop
 *	FOR variable = start TO end STEP step CYCLE
 *	  start, end and step can be evaluated via the shunter & rpnEvaluator
 *	CYCLE is now optional.
 *********************************************************************************
 */

int doFor (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  uint16_t  variable, type, index ;
  double    result, toValue, stepValue ;
  int       len ;
  
// Start with the assignment section

  variable = *p++ ;
  type     = variable &  TK_SYM_MASK ;
  index    = variable & ~TK_SYM_MASK ;

  if (type != TK_SYM_VAR_NUM)
    return syntaxError ("FOR: Numeric variable expected") ;

  if (*p++ != TK_EQUALS)
    return syntaxError ("FOR: Equals expected") ;

  if (!shuntingYard (p, &len))
    return FALSE ;
  p += len ;

  if (!rpnEvalNum (&result))
    return FALSE ;
  storeRealVar (index, result) ;		// Assign

// Check and evaluate the TO value

  if (*p != TK_TO)
    return syntaxError ("FOR: TO expected") ;

  ++p ;
  if (!shuntingYard (p, &len))
    return FALSE ;
  p += len ;

  if (!rpnEvalNum (&toValue))
    return FALSE ;

// See if there is a step

  if (*p == TK_STEP)
  {
    ++p ;
    if (!shuntingYard (p, &len))
      return FALSE ;
    p += len ;

    if (!rpnEvalNum (&stepValue))
      return FALSE ;
  }
  else
    stepValue = 1.0 ;

// Push the information into the FOR stack

  if (!pushForStack (index, toValue, stepValue))
    return syntaxError ("Too many LOOPs") ;

// Next may be CYCLE or end of line

  if ((*p == TK_CYCLE) || (*p == TK_DO))
    ++p ;

  if (!endOfLine (p))
    return syntaxError ("FOR: Extra data at end of line") ;

  if (!pushCycleStack (linePtr))
    return syntaxError ("Too many LOOPs") ;
  else
  {
    ++linePtr ;
    return TRUE ;
  }
}
