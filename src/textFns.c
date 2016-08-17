/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 *********************************************************************************
 * textFns.c:
 *	Functions to manipulate text on the screen
 *********************************************************************************
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
 *********************************************************************************
 */

#include <SDL/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "bomb.h"

#include "bool.h"
#include "keywords.h"
#include "symbolTable.h"

#include "screenKeyboard.h"
#include "readline.h"

#include "shuntingYard.h"
#include "rpnEval.h"

#include "rtb.h"
#include "textFns.h"

static int myFgTextColour = 15 ;
static int myBgTextColour =  0 ;


/*
 * doCls:
 *	Clear the screen (to the current text background colour)
 *********************************************************************************
 */

int doCls (void *ptr)
{
  screenClear () ;
  return TRUE ;
}


/*
 * doTwidth: doTheight:
 *	Pseudo variables to represent the size of the TEXT screen
 *********************************************************************************
 */

int doTwidth (void *ptr)
{
  if (ptr != NULL)
    return FALSE ;

  pushN ((double)tWidth) ;
  return TRUE ;
}

int doTheight (void *ptr)
{
  if (ptr != NULL)
    return FALSE ;

  pushN ((double)tHeight) ;
  return TRUE ;
}


/*
 * doHtab: doVtab:
 *	(Pseudo variables)
 *	Change text cursor positions
 *********************************************************************************
 */

int doHtab (void *ptr)
{
  if (ptr == NULL)			// Read
    pushN ((double)cursorX) ;
  else					// Write
    cursorX = (int)rint (*(double *)ptr) ;

  return TRUE ;
}

int doVtab (void *ptr)
{
  if (ptr == NULL)			// Read
    pushN ((double)cursorY) ;
  else					// Write
    cursorY = (int)rint (*(double *)ptr) ;

  return TRUE ;
}


/*
 * doHVtab:
 *	Set both X&Y text positions.
 *********************************************************************************
 */

int doHVtab (void *ptr)
{
  int x, y ;
  
  if (!twoNumbers ())
    return syntaxError ("HVTAB: Expected two numbers") ;
  
  y = (int)floor (popN ()) ;
  x = (int)floor (popN ()) ;

  setTextCursor (x, y) ;

  return TRUE ;
}



/*
 * doTcolour: doBcolour:
 *	(Pseudo variables)
 *	Change text foreground and background colours
 *********************************************************************************
 */

int doTcolour (void *ptr)
{
  if (ptr == NULL)			// Read
    pushN ((double)myFgTextColour) ;
  else					// Write
  {
    myFgTextColour = (int)floor (*(double *)ptr) ;
    setTextFgColour (myFgTextColour) ;
  }
  return TRUE ;
}

int doBcolour (void *ptr)
{
  if (ptr == NULL)			// Read
    pushN ((double)myBgTextColour) ;
  else					// Write
  {
    myBgTextColour = (int)floor (*(double *)ptr) ;
    setTextBgColour (myBgTextColour) ;
  }
  return TRUE ;
}


/*
 * doGet
 * doGetD
 *	Get a single character as a number or string
 *********************************************************************************
 */

int doGet (void *ptr)
{
  updateDisplay () ;
  pushN ((double)readChar ()) ;
  return TRUE ;
}

int doGetD (void *ptr)
{
  char    buf [2] ;
  uint8_t key ;

  updateDisplay () ;

  key = readChar () ;

  buf [0] = key ;
  buf [1] = 0 ;
  pushS (buf) ;

  return TRUE ;
}

/*
 * doInkey:
 *	Check to see if any keys have been pressed and return the key value,
 *	or -1 if not keys pressed
 *********************************************************************************
 */

int doInkey (void *ptr)
{
  if (keyPressed ())
    pushN ((double)keyboardGetchar ()) ;
  else
    pushN (-1.0) ;
  return TRUE ;
}



/*
 * doInput:
 *	Input a line of text, optionally preceeded with a prompt
 *	INPUT "Prompt>", variable
 *	INPUT variable
 *********************************************************************************
 */

int doInput (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  uint16_t  symbol, type, index ;
  char     *line ;
  double    realNum ;

  symbol = *p++ ;
  type   = symbol &  TK_SYM_MASK ;
  index  = symbol & ~TK_SYM_MASK ;

  if (type == TK_SYM_CONST_STR)		// Output the prompt
  {
    screenPuts (symbolTable [index].name) ;
    if (*p++ != TK_COMMA)
      return syntaxError ("INPUT: Comma expected after prompt") ;

    symbol = *p++ ;
    type   = symbol &  TK_SYM_MASK ;
    index  = symbol & ~TK_SYM_MASK ;
  }
  else
    screenPuts ("? ") ;

  if (! ((type == TK_SYM_VAR_NUM) || (type == TK_SYM_VAR_STR)))
    return syntaxError ("INPUT: Scalar variable expected") ;

  if (!endOfLine (p))
    return syntaxError ("INPUT: Extra data at end of line") ;

  line = readLine (NULL) ;

  ++linePtr ;

  if (type == TK_SYM_VAR_STR)			// String
  {
    if (!storeStringVar (index, line))
      return syntaxError ("INPUT: Out of memory") ;
  }
  else						// Number
  {
    if (isdigit (*line)							||	// Simple
      ( (*line == '.') &&  isdigit (*(line+1)))				||	// Start with a dot
      ( (*line == '-') && (isdigit (*(line+1)) || (*(line+1) == '.')))	||	// Negative
      ( (*line == '+') && (isdigit (*(line+1)) || (*(line+1) == '.'))))		// Positive
      sscanf (line, "%lf", &realNum) ;
    else
      realNum = 0.0 ;
    storeRealVar (index, realNum) ;
  }

  return TRUE ;
}
