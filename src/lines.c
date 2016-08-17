/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * lines.c:
 *	Handle lines and line numbers.
 *	We maintain the list of line numbers as an array rather than
 *	a linked line. Disadvantages is static allocation at compile-
 *	time, but the advantage is that we can binary search it when
 *	looking for the target of a GOTO or GOSUB.
 *
 *	There may be other advantages or disadvantages though...
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
#include <errno.h>
#include <ctype.h>

#include "bool.h"
#include "lines.h"

//#include "bomb.h"

#include "screenKeyboard.h"

int numLines = 0 ;
struct lineNumberStruct programLines [MAX_LINES] ;


/*
 * sortLineNumbers:
 *	Sort the array of line numbers into ascending order.
 *	(This is a Shell sort)
 ***********************************************************************
 */

void sortLineNumbers (void)
{
  register int i, j, k, m, n ;

  uint16_t *tmpD ;
  int       tmpN, tmpL ;

  n = numLines ;

  for (m = n / 2 ; m > 0 ; m /= 2 )
  {
    for (j = m ; j < n ; ++j)
    {
      for (i = j - m ; i >= 0 ; i -= m)
      {
	k = i + m ;
        if (programLines [k].lineNumber >= programLines [i].lineNumber)
          break ;
        else // Swap
        {
          tmpD = programLines [i].data ;
          tmpN = programLines [i].lineNumber ;
          tmpL = programLines [i].length ;
	  programLines [i].data       = programLines [k].data   ;
	  programLines [i].lineNumber = programLines [k].lineNumber ;
	  programLines [i].length     = programLines [k].length   ;
	  programLines [k].data       = tmpD ;
	  programLines [k].lineNumber = tmpN ;
	  programLines [k].length     = tmpL ;
        }
      }
    }
  }
}

/*
 * setupProgram:
 *	Initialise the program data array
 *********************************************************************************
 */

void setupProgram (void)
{
  int i ;
  struct lineNumberStruct *pl ;

  for (i = 0 ; i < MAX_LINES ; ++i)
  {
    pl = &programLines [i] ;
    pl->lineNumber = 0 ;
    pl->length     = 0 ;
    pl->data       = NULL ;
  }
  numLines = 0 ;
}

/*
 * deleteProgram:
 *	Delete all program lines.
 *********************************************************************************
 */

void deleteProgram (void)
{
  int i ;
  struct lineNumberStruct *pl ;

  for (i = 0 ; i < numLines ; ++i)
  {
    pl = &programLines [i] ;

    if (pl->data != NULL)
      free (pl->data) ;
    pl->data       = NULL ;
    pl->length     = 0 ;
    pl->lineNumber = 0 ;
  }
  numLines = 0 ;
}


int newLineNumber (int line)
{
  int n = numLines ;

  programLines [numLines++].lineNumber = line ;

  return n ;
}


/*
 * findLine:
 *	Return the index into our line numbers array of the given
 *	line-number.
 *
 *	TODO: Replace the SLOW linear search with FAST binary search
 *********************************************************************************
 */

int findLine (int lineNumber)
{
  int i ;

  for (i = 0 ; i < numLines ; ++i)
    if (programLines [i].lineNumber == lineNumber)
      return i ;

  return -1 ;
}

/*
 * storeLine:
 *	Store a new program line in memory.
 *	Or overwrite an existing one.
 *********************************************************************************
 */

void storeLine (int lineNumber, uint16_t *data, int length)
{
  int n ;

// Check to see if we're overwriting an existing line

  n = findLine (lineNumber) ;

  if (n == -1)	// No line, so add a new one in
  {
    programLines [numLines].lineNumber = lineNumber ;
    programLines [numLines].length     = length ;
    programLines [numLines].data       = malloc (length) ;
    memcpy (programLines [numLines].data, data, length) ;

    ++numLines ;

    sortLineNumbers () ;
  }
  else		// Overwrite an existing line
  {
    programLines [n].length = length ;
    programLines [n].data   = realloc (programLines [n].data, length) ;
    memcpy (programLines [n].data, data, length) ;
  }
}


/*
 * deleteLine:
 *	We're deleting a line from the program store.
 *	This might be considered cheating by freeing the
 *	data, renumbering that line to $bignum, sorting
 *	the nunber list and --count.
 *	But it's effective.
 ***********************************************************************
 */

void deleteLine (int lineNumber)
{
  int n ;

  n = findLine (lineNumber) ;
  if (n == -1)	// Not found (why?)
    return ;

  programLines [n].lineNumber = MAX_LINENUM + 1 ;
  sortLineNumbers () ;

  free (programLines [numLines].data) ;
  programLines [numLines].lineNumber = 0 ;
  programLines [numLines].length     = 0 ;
  programLines [numLines].data       = NULL ;
  --numLines ;

}
