/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * load.c:
 *	Load a program from disk/net/etc into memory
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
//#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdint.h>

#include "rtb.h"
#include "bomb.h"
#include "bool.h"

#include "symbolTable.h"
#include "screenKeyboard.h"

#include "lines.h"
#include "tokenize.h"
#include "parseInput.h"

#include "load.h"

/*
 * doLoadCommand:
 *	Load a program from file
 *********************************************************************************
 */

void _doLoad (char *file)
{
  FILE *fd ;
  char  line [256] ;
  char *p, *q, *fx ;
  int   length ;

  int   fileLineNum    = 0 ;
  int   progLineNum    = 0 ;
  int   firstLine      = TRUE ;
  int   loadError      = FALSE ;
  int   gotLineNumbers = FALSE ;

  int   num ; 


  if (programChanged)
    return (void)syntaxError ("LOAD: Current program has been altered. Use NEW first") ;

  deleteProgram () ;
  deleteSymbols () ;

  if ((fx = malloc (strlen (file) + 6)) == NULL)
    return (void)syntaxError ("LOAD: Out of memory") ;

  sprintf (fx, "%s.rtb", file) ;

  revokeRoot () ;
  if ((fd = fopen (fx, "r")) == NULL)
    if ((fd = fopen (file, "r")) == NULL)
    {
      regainRoot () ;
      free (fx) ;
      return (void)syntaxError ("LOAD: Unable to open \"%s\": %s", file, strerror (errno)) ;
    }

  while (fgets (line, 256, fd) != NULL)
  {

    ++fileLineNum ;

// Chomp leading and trailing spaces

    p = line ;
    while (isspace (*p))
      ++p ;
    q = p + strlen (p) - 1 ;
    while (isspace (*q))
      *q-- = 0 ;

    if (strlen (p) == 0)		// Ignore blank lines
      continue ;

    if (*p == '#')			// Ignore # comments (ie. for #!/... lines)
      continue ;

// Check first line for a line number

    if (firstLine)
    {
      if (isdigit (*p))
	gotLineNumbers = TRUE ;
      else
	if (initialFilename == NULL)
	  screenPrintf ("Adding line numbers\n") ;
      firstLine = FALSE ;
    }

    if (gotLineNumbers)
    {
      num = 0 ;
      if (!isdigit (*p))
      {
	screenPrintf ("*** Missing line number\n") ;
	num = 0 ;
      }
      else
      {
	while (isdigit (*p))
	{
	  if (num > (MAX_LINENUM / 10))
	  {
	    screenPrintf ("*** Line number too big. (Max. %d)\n", MAX_LINENUM) ;
	    num = 0 ;
	    break ;
	  }
	  else
	    num = num * 10 + (*p - '0') ;
	  ++p ;
	}
      }

      if (num == 0)
      {
	loadError = TRUE ;
	break ;
      }

      progLineNum = num ;

      while (isspace (*p))
	++p ;
    }
    else
      ++progLineNum ;

    if (tokenize (p, tokenizedLine, &length))
    {
      storeLine (progLineNum, tokenizedLine, length) ;
      continue ;
    }

    loadError = TRUE ;
    break ;
  }

  if (loadError)
  {
    screenPrintf ("Program load aborted at line %d\n", gotLineNumbers ? progLineNum : fileLineNum) ;
    deleteProgram () ;
    deleteSymbols () ;
  }

  fclose     (fd) ;
  free       (fx) ;
  regainRoot () ;

  if (initialFilename == NULL)
    screenPrintf ("Ready.\n\n") ;

  if (savedFilename != NULL)
  {
    free (savedFilename) ;
    savedFilename = NULL ;
  }
}


void doLoadCommand (int argc, char *argv [])
{
  if (argc != 2)
    return (void)syntaxError ("LOAD: What filename?") ;

  _doLoad (argv [1]) ;
}


