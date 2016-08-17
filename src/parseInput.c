/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * parseInput.c:
 *	Parse the line typed by the user. If it starts with a number
 *	then it has to be stored in program memory, else it has to be
 *	evaluated as an immediate command.
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
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdint.h>

#include "rtb.h"

#include "bomb.h"

#include "screenKeyboard.h"

#include "bool.h"
#include "lines.h"
#include "keywords.h"
#include "symbolTable.h"
#include "tokenize.h"
#include "shuntingYard.h"
#include "rpnEval.h"
#include "run.h"

#include "commands.h"

#include "parseInput.h"

// Globals

uint16_t tokenizedLine [256] ;


/*
 * enargv:
 *	Split up a line into a standard set of argc + *argv[] units.
 *	Args are separated by spaces or commas.
 *	It only recognises double quotes for verbatim mode.
 *********************************************************************************
 */

static int separator (register char test)
{
  return ((test == ' ') || (test == ',')) ;
}

static int enargv (char *argv [], char *line)
{
  char     *p = line ;
  int    argc = 0 ;
  int quoting = FALSE ;

  while (*p && (argc < 32))
  {
    while (separator (*p))
      ++p ;

    if (*p == '"')
    {
      quoting = TRUE ;
      ++p ;
    }

    argv [argc++] = p ;

    for  (; *p ; ++p)
    {
      if ((*p == '"') && quoting)
      {
	quoting = FALSE ;
	*p++ = 0 ;
	break ;
      }

      if (separator (*p) && quoting)
	continue ;

      if (separator (*p))
      {
	*p++ = 0 ;
	break ;
      }
    }

  }
  return argc ;
}


/*
 * doCommandLine:
 *	Parse a command line. Line is expected to have no leading or
 *	trailing spaces. We alter line by writing back into it.
 *********************************************************************************
 */

static void doCommandLine (char *line)
{
  struct  commandStruct *c ;
  int     length ;
  char   *copy ;
  int     argc ;
  char   *argv [32] ;

  escapePressed  = FALSE ;
  programRunning = FALSE ;
  gotSyntaxError = FALSE ;

// Copy the command line for later...

  if ((copy = malloc (strlen (line) + 1)) == NULL)
    return (void)syntaxError ("*** Out of memory") ;

  strcpy (copy, line) ;

  argc = enargv (argv, line) ;

  for (c = commands ; c->command != NULL ; ++c)
  {
    if (strcasecmp (c->command, argv [0]) == 0)
    {
      if (c->function == NULL)
        screenPrintf ("*** Command: %s with no function\n", c->command) ;
      else
	c->function (argc, argv) ;
      free (copy) ;
      return ;
    }
  }

/// This is a gross hack which reaches 11 on the scale of 0-10 on the gross 
//	hackiness scale. Rather than do it properly (whatever that is), I'm
//	going to tokenize and insert the line into the program memory then
//	run that line of code.
//	It'll probably upset everything else, but what the heck...

  if (tokenize (copy, tokenizedLine, &length))
  {
    storeLine  (60000, tokenizedLine, length) ;
    (void)tokenize ("END", tokenizedLine, &length) ;
    storeLine  (60001, tokenizedLine, length) ;
    runFrom    (60000) ;
    deleteLine (60001) ;
    deleteLine (60000) ;
  }

  free (copy) ;
}


/*
 * parseInput:
 *	Take an input line and attempt to parse it. We basically check
 *	for the presence of a line number or not.
 *********************************************************************************
 */

void parseInput (char *line)
{
  register char *ptr = line ;
  register char *tail ;
  unsigned int lineNum = 0 ;
  int length ;

// Ignore leading spaces

  while (isspace (*ptr))
    ++ptr ;

// Blank line?

  if (*ptr == '\0')
    return ;

// Chomp trailing spaces

  tail = ptr + strlen (ptr) - 1 ;
  while (isspace (*tail))
    *tail-- = '\0' ;

// If no number, then it's an immediate command

  if (!isdigit (*ptr))
  {
    doCommandLine (ptr) ;
    return ;
  }

// Read the number

  while (isdigit (*ptr))
  {
    if (lineNum > (MAX_LINENUM / 10))
    {
      screenPrintf ("*** Line number too big. Maximum is: %d.\n", MAX_LINENUM) ;
      return ;
    }
    else
      lineNum = lineNum * 10 + (*ptr - '0') ;
    ++ptr ;
  }

// Ignore spaces after the number

  while (isspace (*ptr))
    ++ptr ;

// Empty line?

  if (strlen (ptr) == 0)
    deleteLine (lineNum) ;
  else
  {
    if (tokenize (ptr, tokenizedLine, &length))
    {
      storeLine (lineNum, tokenizedLine, length) ;
      programChanged = TRUE ;
    }
  }
}
