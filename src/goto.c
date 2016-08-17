/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * goto.c:
 *	Handle GOTO and GOSUBs
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
//#include "shuntingYard.h"
//#include "rpnEval.h"

//#include "commands.h"

#include "run.h"
#include "goto.h"


// Locals

static int *gosubStack    = NULL ;
static int  gosubStackPtr = 0 ;
static int  gosubCount    = 0 ;


/*
 * clearSubs:
 *	Reset things back for a clean program exit, or new run
 *********************************************************************************
 */

void clearSubs (void)
{
  gosubCount    = 0 ;
  gosubStackPtr = 0 ;

  if (gosubStack != NULL)
    free (gosubStack) ;
}

/*
 * initSubs:
 *	Reset our stack pointer at the start of a new run
 *********************************************************************************
 */

int initSubs (void)
{
  clearSubs () ;

// Pre-Allocate the first stack chunk, to hopefully make small programs
//	run a bit smoother

  if ((gosubStack = calloc (GOSUB_STACK_CHUNK, sizeof (int))) == NULL)
    return syntaxError ("Out of memory pre-allocating the GOSUB stack") ;

  return TRUE ;
}


/*
 * pushGosubStack: popGosubStack:
 *	Manage the GOSUB/RETURN line number stack
 *********************************************************************************
 */

static int pushGosubStack (int line)
{
  if (gosubStackPtr == gosubCount)
  {
    gosubCount += GOSUB_STACK_CHUNK ;
    if ((gosubStack = realloc (gosubStack, gosubCount * sizeof (int))) == NULL)
      bomb ("Out of memory reallocating gosubStack space", TRUE) ;
  }

  gosubStack [gosubStackPtr++] = line ;
  return TRUE ;
}

static int popGosubStack (void)
{
  if (gosubStackPtr == 0)
    return -1 ;
  else
    return gosubStack [--gosubStackPtr] ;
}

/*
 * doReturn:
 *	Return from a GOSUB
 *********************************************************************************
 */

int doReturn (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  int       newLinePtr ;

  if (!endOfLine (p))
    return syntaxError ("Data after RETURN shouldn't be there") ;

  newLinePtr = popGosubStack () ;
  if (newLinePtr == -1)
  {
    syntaxError ("RETURN without GOSUB") ;
    return FALSE ;
  }
  linePtr = newLinePtr ;
  return TRUE ;
}

/*
 * doGosub:
 *	Call a subroutine
 *********************************************************************************
 */

int doGosub (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  struct   symbolTableStruct *s ;
  int      newLinePtr, newLineNumber ;

  if (!endOfLine (p))
    return syntaxError ("GOSUB: Bad number") ;

  s = &symbolTable [*(p-1) & ~TK_SYM_MASK] ;
  newLineNumber = s->value.lineNumber ;
  if ((newLinePtr = findLine (newLineNumber)) == -1)
    return syntaxError ("GOSUB: Line %d does not exist", newLineNumber) ;

  if (!pushGosubStack (linePtr + 1))
    return syntaxError ("Too many GOSUBs") ;

  linePtr = newLinePtr ;
  return TRUE ;
}


/*
 * doGoto:
 *	Handle the dreaded GOTO instruction!
 *********************************************************************************
 */

int doGoto (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  struct   symbolTableStruct *s ;
  int      newLinePtr, newLineNumber ;

  if (!endOfLine (p))
    return syntaxError ("GOTO: Bad number") ;

  s = &symbolTable [*(p-1) & ~TK_SYM_MASK] ;
  newLineNumber = s->value.lineNumber ;
  if ((newLinePtr = findLine (newLineNumber)) == -1)
    return syntaxError ("GOTO: Line %d does not exist", newLineNumber) ;

  linePtr = newLinePtr ;
  return TRUE ;
}
