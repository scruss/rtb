/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * commands.c:
 *	General commands to type
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

#define	_GNU_SOURCE

#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <sys/time.h>
#include <dirent.h>
#include <fnmatch.h>

#include "version.h"
#include "rtb.h"

#include "screenKeyboard.h"
#include "serial.h"

#include "bool.h"
#include "keywords.h"
#include "lines.h"
#include "symbolTable.h"
#include "shuntingYard.h"
#include "rpnEval.h"

#include "commands.h"
#include "procedures.h"
#include "assign.h"
#include "cycle.h"
#include "goto.h"
#include "procFn.h"
#include "fileFns.h"
#include "spriteFns.h"
#include "run.h"

// Globals

/*
 * findProcs:
 *	Called at program RUN time - scan the entire program, looking for
 *	procedures (and functions) and entering their linePtrs into the
 *	symbol table and checking their arguments.
 *	Also look for the first instance of DATA
 *********************************************************************************
 */

static int findProcs (void)
{
  uint16_t *p ;
  uint16_t  symbol, index ;
  int       pIndex ;
  int       args, expectComma ;

  dataPtr   = firstDataPtr   = NULL ;
  dataIndex = firstDataIndex = -1 ;

  for (pIndex = 0 ; pIndex < numLines ; ++pIndex)
  {
    p = programLines [pIndex].data ;

    if ((*p == TK_DATA) && (dataPtr == NULL))
    {
      dataPtr   = firstDataPtr   = p + 1 ;
      dataIndex = firstDataIndex = pIndex ;
      continue ;
    }

    if (*p++ != TK_DEF)
      continue ;

    symbol = *p &  TK_SYM_MASK ;
    index  = *p & ~TK_SYM_MASK ;

    ++p ;

    if (! ((symbol == TK_SYM_PROC) || (symbol == TK_SYM_FUNC)))
      continue ;

// We have one, store it ...

    symbolTable [index].value.linePtr = pIndex ;

// ... and work out how many parameters it has

    if (endOfLine (p))				// EOL - none?
    {
      symbolTable [index].args = 0 ;
      continue ;
    }

    if (*p != TK_BRA)
      return syntaxError ("PROC/FN \"%s\" at line %d: Bad parameter list",
		symbolTable [index].name, programLines [pIndex].lineNumber) ;

    args        = 0 ;
    expectComma = FALSE ;
    while (*++p != TK_KET)
    {
      if (expectComma && (*p == TK_COMMA))
      {
	expectComma = FALSE ;
	++p ;
      }

      symbol  = *p & TK_SYM_MASK ;

      if (endOfLine (p))	// Oops
	return syntaxError ("PROC/FN \"%s\" at line %d: Bad parameter list (1)",
		symbolTable [index].name, programLines [pIndex].lineNumber) ;

      if ((symbol == TK_SYM_VAR_NUM_ARR) || (symbol == TK_SYM_VAR_STR_ARR))
      {
	if (*++p != TK_BRA)
	  return syntaxError ("PROC/FN \"%s\" at line %d: Expected '(' after array",
		symbolTable [index].name, programLines [pIndex].lineNumber) ;

	if (*++p != TK_KET)
	  return syntaxError ("PROC/FN \"%s\" at line %d: Expected ')' after array",
		symbolTable [index].name, programLines [pIndex].lineNumber) ;

 	++args ;
	expectComma = TRUE ;
	continue ;
      }


      if ((symbol == TK_SYM_VAR_NUM) || (symbol == TK_SYM_VAR_STR))
      {
 	++args ;
	expectComma = TRUE ;
	continue ;
      }

      return syntaxError ("PROC/FN \"%s\" at line %d: Bad parameter list (2)",
		symbolTable [index].name, programLines [pIndex].lineNumber) ;

    }

    ++p ;

    if (!endOfLine (p))
      return syntaxError ("PROC/FN \"%s\" at line %d: Bad parameter list (3)",
	      symbolTable [index].name, programLines [pIndex].lineNumber) ;

    symbolTable [index].args = args ;
  }
  return TRUE ;
}


/*
 * doRunCommand:
 *	RUN a program!
 *********************************************************************************
 */

void doRunCommand (int argc, char *argv [])
{
  char *p ;
  int   startLine = 0 ;

  gotSyntaxError = FALSE ;
  continuePtr    = -1 ;

  if (numLines == 0)
    return (void)syntaxError ("No program to RUN") ;

  if (argc == 2)
  {
    p = argv [1] ;
    while (isdigit (*p))
    {
      if (startLine > (MAX_LINENUM / 10))
	return (void)syntaxError ("Line number too big. Maximum is: %d.\n", MAX_LINENUM) ;
      else
	startLine = startLine * 10 + (*p - '0') ;
      ++p ;
    }
    if (*p != '\0')
      return (void)syntaxError ("Invalid line number") ;
  }

  fStackPtr     = oStackPtr   = 0 ;
  stackOrderPtr = numStackPtr = strStackPtr = 0 ;
  symbolTableStackPtr = 0 ;
  strcpy (fmtString, DEFAULT_NUM_FORMAT) ;

  if (!initSubs  ())	return ;
  if (!initProcs ())	return ;
  if (!initCycle ())	return ;

  serialCloseAll () ;
  fileCloseAll   () ;

  clearVariables () ;
  deleteSprites  () ;

  if (!findProcs ())
    return ;

  if (gettimeofday (&startTime, NULL) != 0)
    return (void)syntaxError ("Setting initial run-time failed") ;

  angleConversion = M_PI / 180.0 ;	//Degrees

  programRunning = TRUE ;
  escapePressed  = FALSE ;
  gotSyntaxError = FALSE ;
  (void)runFrom (startLine) ;
  escapePressed  = FALSE ;
  programRunning = FALSE ;

  return ;
}


/*
 * doContCommand:
 *	Continue a program (if possible)
 *********************************************************************************
 */

void doContCommand (int argc, char *argv [])
{
  if (numLines == 0)
    return (void)syntaxError ("No program to RUN") ;

  if (continuePtr == -1)
    return (void)syntaxError ("No program active") ;

  linePtr     = continuePtr ;
  continuePtr = -1 ;

  programRunning = TRUE ;
  escapePressed  = FALSE ;
  gotSyntaxError = FALSE ;
    runLines () ;
  programRunning = FALSE ;
  escapePressed  = FALSE ;

  return ;
}


/*
 * doTronCommand: doTroffCommand:
 *	Turn trace on or off.
 *********************************************************************************
 */

void doTronCommand (int argc, char *argv [])
{
  tron = TRUE ;
  screenPrintf ("*** Trace ON\n") ;
  return ;
}

void doTroffCommand (int argc, char *argv [])
{
  tron = FALSE ;
  screenPrintf ("*** Trace OFF\n") ;
  return ;
}


/*
 * doClearCommand:
 *	Clear all variables
 *********************************************************************************
 */

void doClearCommand (int argc, char *argv [])
{
  deleteSprites  () ;
  clearVariables () ;
  continuePtr = -1 ;
  return ;
}


/*
 * doNewCommand:
 *	New - delete, wipe out, +++REDO FROM START, etc.
 *********************************************************************************
 */

void doNewCommand (int argc, char *argv [])
{
  if (argc != 1)
    return (void)syntaxError ("New what?\n") ;

  deleteSprites  () ;
  clearVariables () ;
  deleteProgram  () ;
  deleteSymbols  () ;
  programChanged = FALSE ;
  screenPuts ("Ready\n\n") ;
  return ;
}


/*
 * doDebugCommand:
 *	Do various stuff with the >>> command...
 *********************************************************************************
 */

void doDebugCommand (int argc, char *argv [])
{
  if (argc != 2)
    return (void)syntaxError ("Usage: >>> <flag>") ;

  /**/ if (strcmp (argv [1], "sy") == 0)
    dumpShuntingYard (">>>") ;
  else if (strcmp (argv [1], "rpn") == 0)
    dumpRpnStack (">>>") ;
  else  if (strcmp (argv [1], "st") == 0)
    dumpSymbolTable (">>>") ;
  else
    screenPuts ("? Unknown Debug code (sy, rpn, st)\n") ;

  return ;
}


/*
 * doDirCommand:
 *	File directory
 *********************************************************************************
 */

static int filter (const struct dirent *dir)
{
  if (fnmatch ("*.rtb", dir->d_name, FNM_NOESCAPE | FNM_CASEFOLD) == 0)
    return 1 ;
  return 0 ;
}

void doDirCommand (int argc, char *argv [])
{
  struct dirent **namelist ;
  int i, n ;
  char *dir ;

  if (argc == 1)
    dir = "." ;
  else if (argc == 2)
    dir = argv [1] ;
  else
    return (void)syntaxError ("DIR {directory}") ;

  if ((n = scandir (dir, &namelist, filter, alphasort)) < 0)
    return (void)syntaxError ("DIR: Unable to read file directory") ;

  screenPrintf ("Matched %d %s\n", n, (n == 1) ? "file" : "files") ;

  for (i = 0 ; i < n ; )
  {
    screenPrintf ("%30s   ", namelist [i]->d_name) ;
    free (namelist [i]) ;
    if (++i == n)
    {
      screenPrintf ("\n") ;
      break ;
    }
    screenPrintf ("%30s\n", namelist [i]->d_name) ;
    free (namelist [i]) ;
    ++i ;
  }
  free (namelist) ;
}


/*
 * doCdCommand:
 *	Chdir
 *********************************************************************************
 */

void doCdCommand (int argc, char *argv [])
{
  int res ;

  if (argc != 2)
    return (void)syntaxError ("CD {directory}") ;

  revokeRoot () ;
    res = chdir (argv [1]) ;
  regainRoot () ;

  if (res < 0)
    return (void)syntaxError ("CD: %s", strerror (errno)) ;
}


/*
 * doPwdCommand:
 *	Print working directory
 *********************************************************************************
 */

void doPwdCommand (int argc, char *argv [])
{
  char *wd ;

  revokeRoot () ;
    wd = getcwd (NULL, 0) ;
  regainRoot () ;

  if (wd == NULL)
    return (void)syntaxError ("PWD: %s", strerror (errno)) ;

  screenPrintf ("CWD: %s\n", wd) ;
  free (wd) ;
}


/*
 * doVersionCommand:
 *	Output our version, etc.
 *********************************************************************************
 */

void doVersionCommand (int argc, char *argv [])
{
  screenPuts ("\n") ;
  screenPrintf ("This is Return to Basic Version: %s.%s.%s\n", VERSION, MAJ_VERSION, SUB_VERSION) ;
  screenPuts ("\n") ;
  screenPuts ("  RTB is free software: you can redistribute it and/or modify\n") ;
  screenPuts ("  it under the terms of the GNU General Public License as published by\n") ;
  screenPuts ("  the Free Software Foundation, either version 3 of the License, or\n") ;
  screenPuts ("  (at your option) any later version.\n") ;
  screenPuts ("\n") ;
  screenPuts ("  RTB is distributed in the hope that it will be useful,\n") ;
  screenPuts ("  but WITHOUT ANY WARRANTY; without even the implied warranty of\n") ;
  screenPuts ("  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n") ;
  screenPuts ("  GNU General Public License for more details.\n") ;
  screenPuts ("\n") ;
  screenPuts ("  You should have received a copy of the GNU General Public License\n") ;
  screenPuts ("  along with RTB.  If not, see <http://www.gnu.org/licenses/>.\n") ;
  screenPuts ("\n") ;
}


/*
 * doExitCommand:
 *	So long and thanks for all the fish.
 *********************************************************************************
 */

void doExitCommand (int argc, char *argv [])
{
  exitRTB = TRUE ;
}
