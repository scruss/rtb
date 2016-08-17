/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * readline.c:
 *	Read in a line of a maximum length.
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

// Todo:
//	Write a proper terminal emulator

#include <SDL/SDL.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

#include "screenKeyboard.h"
#include "bomb.h"
#include "bool.h"

#include "readline.h"

static char *history [MAX_HISTORY] ;
static int hPtr = 0 ;

static char *iBuf,  *iPtr ;
static int iBufLen ;


/*
 * cursorOn: cursorOff:
 *	Manage the screen cursor by making the currently pointed
 *	character highlighted (inverse)
 *********************************************************************************
 */

static void cursorOn (void)
{
  uint32_t tmp ;
  uint8_t c = *iPtr ;

  if (c == 0)
    c = ' ' ;

  tmp          = fgTextColour ;
  fgTextColour = bgTextColour ;
  bgTextColour = tmp ;

  screenPutchar (c, TRUE) ;
  screenPutchar (8, FALSE) ;

  tmp          = fgTextColour ;
  fgTextColour = bgTextColour ;
  bgTextColour = tmp ;
}

static void cursorOff (void)
{
  uint8_t c = *iPtr ;

  if (c == 0)
    c = ' ' ;

  screenPutchar (c, TRUE) ;
  screenPutchar (8, FALSE) ;
}


/*
 * rewindCursor:
 *	Move the cursor location to the start of the line.
 *********************************************************************************
 */

static void rewindCursor (void)
{
  int backSpaces = iPtr - iBuf ;

  while (backSpaces--)
    screenPutchar (8, FALSE) ;
}


/*
 * redrawLine:
 *	Redraw the input line - assumes the cursor is at the start already.
 *	Needlessly complicated as it has to print the line, rewind, then print
 *	up to the position of the iPtr location.
 *********************************************************************************
 */

static void redrawLine (void)
{
  char *p, *e ;
  register int c = 0 ;

  p = iBuf ;
  e = iBuf + iBufLen ;

  while (p != e)
  {
    ++c ;
    screenPutchar (*p++, TRUE) ;
  }

// Add an extra space in-case we've just deleted a character

  screenPutchar (' ', TRUE) ;
  ++c ;

  while (c-- != 0)
    screenPutchar (8, FALSE) ;

  p = iBuf ;

  while (p != iPtr)
    screenPutchar (*p++, TRUE) ;
}


/*
 * insertChar:
 *	Insert a character at iPtr location, moving all others down.
 *********************************************************************************
 */

static void insertChar (register int c)
{
  register char *d = iBuf + iBufLen + 1 ;	// Start at the trailing 0
  register char *s = d - 1 ;

  rewindCursor () ;

  while (d != iPtr)
    *d-- = *s-- ;

  *iPtr++ = c ;
  ++iBufLen ;

  redrawLine () ;
}


/*
 * deleteChar:
 *	Delete the character at iPtr location, moving all others up.
 *********************************************************************************
 */

static void deleteChar (void)
{
  register char *d = iPtr ;
  register char *s = d + 1 ;
  register char *e = iBuf + iBufLen + 1 ;

  while (s != e)
    *d++ = *s++ ;

  --iBufLen ;
}


/*
 * deleteLine:
 *	Delete the line, updating the display
 *********************************************************************************
 */

static void deleteLine (void)
{
  register int x ;

  rewindCursor () ;

  for (x = 0 ; x < iBufLen ; ++x)
    screenPutchar (' ', TRUE) ;

  for (x = 0 ; x < iBufLen ; ++x)
    screenPutchar (8, FALSE) ;
}


/*
 * findNext:
 *	Bump iPtr to the next occourance of the supplied character.
 *********************************************************************************
 */

static void findNext (register int c)
{
  register char *p ;

  rewindCursor () ;

// Search from next location to the end of the line

  p = iPtr + 1 ;
  while (p < (iBuf + iBufLen))
    if (*p == c)
    {
      iPtr = p ;
      redrawLine () ;
      return ;
    }
    else
      ++p ;

// Search from the start to the current location

  p = iBuf ;
  while (p < iPtr)
    if (*p == c)
    {
      iPtr = p ;
      redrawLine () ;
      return ;
    }
    else
      ++p ;
}

/*
 * saveHistory:
 *	Save the current line in the history buffer
 * 
 */

static void saveHistory (void)
{
  register int len, lastH = hPtr - 1 ;

// Don't store blank lines in the history

  if ((len = strlen (iBuf)) == 0)
    return ;

// Don't store duplicate lines

  if (lastH < 0)
    lastH = MAX_HISTORY - 1 ;

  if (history [lastH] != NULL)
    if (strcmp (history [lastH], iBuf) == 0) /* Identical */
      return ;

  if (history [hPtr] != NULL)
    free (history [hPtr]) ;

  if ((history [hPtr] = malloc (len+1)) == NULL)
    bomb ("Out of memory (history save)", FALSE) ;

  strcpy (history [hPtr], iBuf) ;

  if (++hPtr == MAX_HISTORY)
    hPtr = 0 ;
}


/*
 * readLine:
 *	This is called by the dispatcher in the IO section whenever data
 *	is detected from the keyboard.
 *
 */

char *readLine (char *preload)
{
  register int c ;
  static int finding  = FALSE ;
  static int findChar =    -1 ;
  static int tHist    =    -1 ;

  updateDisplay () ;

  iPtr    = iBuf ;
  if (preload == NULL)
  {
    *iBuf   = 0 ;
    iBufLen = 0 ;
  }
  else
  {
    strcpy (iPtr, preload) ;
    iBufLen = strlen (preload) ;
    redrawLine () ;
  }

  for (;;)
  {
    cursorOn  () ;
    c = keyboardGetchar () ;
    cursorOff () ;

    if (finding)
    {
      finding = FALSE ;
      if (c == K_FIND)
	findNext (findChar) ;
      else
	findNext (findChar = c) ;    
      continue ;
    }

/** Printable character? Insert it.	**/

    if (isprint (c))
    {
      if (iBufLen < MAX_INPUT_SIZE)
	insertChar (c) ;
      continue ;
    }


/** Switch for all other characters	**/

    switch (c)
    {

// Cursor movement

      case RTB_KEY_HOME:
      case K_START_OF_LINE:
	rewindCursor () ;
	iPtr = iBuf ;
	break ;

      case RTB_KEY_END:
      case K_END_OF_LINE:
	rewindCursor () ;
	iPtr = iBuf + iBufLen ;
	redrawLine   () ;
	break ;

      case RTB_KEY_LEFT:
	rewindCursor () ;
	if (iPtr != iBuf)
	  --iPtr ;
	redrawLine   () ;
	break ;

      case RTB_KEY_RIGHT:
	rewindCursor () ;
	if (iPtr != (iBuf + iBufLen))
	  ++iPtr ;
	redrawLine   () ;
	break ;

// Enable find mode?

      case K_FIND:
	finding = TRUE ;
	break ;

// Swap char + next?

      case K_SWAP:
	if (iPtr < (iBuf + iBufLen - 1))
	{
	  rewindCursor () ;
	  iPtr [1] ^= iPtr [0] ;
	  iPtr [0] ^= iPtr [1] ;
	  iPtr [1] ^= iPtr [0] ;
	  redrawLine   () ;
	}
	break ;

// Delete things 

      case K_DEL_C_UNDER:
	if (iPtr != (iBuf + iBufLen))
	{
	  rewindCursor () ;
	  deleteChar   () ;
	  redrawLine   () ;
	}
	break ;

      case K_DEL_C_LEFT:
      case ctrl ('H'):
	rewindCursor () ;
	if (iPtr != iBuf)
	{
	  --iPtr ;
	  deleteChar () ;
	}
	redrawLine   () ;
	break ;

      case K_DEL_LINE:
	deleteLine () ;
	iBufLen = 0 ;
	iPtr    = iBuf ;
	*iBuf   = 0 ;
	tHist   = -1 ;	// Reset history pointer too
	break ;

// Wind back through the history?

      case RTB_KEY_UP:
	if (history [0] != NULL)		// No history
	{
	  if (tHist == -1)			// No windings
	    tHist = hPtr ;

	  if (--tHist < 0)			// Wind back
	    if ( history [tHist = MAX_HISTORY - 1] == NULL)
	      tHist = hPtr - 1;

	  deleteLine () ;
	  strcpy (iBuf, history [tHist]) ;
	  iBufLen = strlen (iBuf) ;
	  iPtr    = iBuf + iBufLen ;
	  redrawLine () ;
	}
	break ;

// Wind forwards through the history?

      case RTB_KEY_DOWN:
	if (history [0] != NULL)		// No history
	{
	  if (tHist == -1)			// No windings
	    tHist = hPtr - 1 ;

	  if (++tHist == MAX_HISTORY)		// Wind forward
	    tHist = 0 ;

	  if (history [tHist] == NULL)		// No forward. history
	    tHist = 0 ;

	  deleteLine () ;
	  strcpy (iBuf, history [tHist]) ;
	  iBufLen = strlen (iBuf) ;
	  iPtr    = iBuf + iBufLen ;
	  redrawLine () ;
	}
	break ;

// CR?  We're done...

      case CR:
      {

// Nul ternimate it so we can display it & store it

	*(iBuf + iBufLen) = 0 ;
	screenPutchar ('\n', TRUE) ;
	saveHistory   () ;
	tHist = -1 ;

	return iBuf ;
      }

      default:
	break ;
    }
  }
}


/*
 * readChar:
 *	Read in a single character
 *********************************************************************************
 */

uint8_t readChar (void)
{
  uint8_t c ;

  cursorOn  () ;
  c = keyboardGetchar () ;
  cursorOff () ;

  return c ;
}


/*
 * setupReadline:
 *	Initialise our line reading stuff
 *********************************************************************************
 */

void setupReadline (void)
{
  register int i ;

  if ((iBuf  = malloc (MAX_INPUT_SIZE + 1)) == NULL)
    bomb ("Insufficient memory for the line input buffer", FALSE) ;

  *iBuf =  0 ;
  iPtr    = iBuf ;
  iBufLen = 0 ;

/** History initialisations **/

  for (i = 0 ; i < MAX_HISTORY ; ++i)
    history [i] = NULL ;
}
