/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * list.c:
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

#undef	DEBUG_LIST

#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdint.h>

#include "bool.h"
#include "rtb.h"
#include "parseInput.h"
#include "readline.h"

#include "bomb.h"
#include "screenKeyboard.h"

#include "keywords.h"
#include "lines.h"
#include "symbolTable.h"

#include "commands.h"

// Globals

int  listToBuffer = FALSE ;
char listBuffer [1024] ;

static int lineStart = TRUE ;
static int indent    = 0 ;
static int forLine   = FALSE ;

static FILE *listFd = NULL ;
static int   doLineNumbers = TRUE ;


/*
 * listPrintf:
 *	Channel all list output through this, so we can capture it,
 *	send it to file or screen, etc.
 *********************************************************************************
 */

void listPrintf (char *message, ...)
{
  va_list argp ;
  char buffer [1024] ;

  va_start (argp, message) ;
    vsnprintf (buffer, 1023, message, argp) ;
  va_end (argp) ;

  if (listFd != NULL)
    fputs (buffer, listFd) ;
  else if (listToBuffer)
    strcat (listBuffer, buffer) ;
  else
    screenPuts (buffer) ;
}

static void spaces (void)
{
  int x = indent * 2 ;
  while (x-- > 0)
    listPrintf (" ") ;
}

static void doListSymbol (uint16_t token)
{
  int symbol = token &  TK_SYM_MASK ;
  int index  = token & ~TK_SYM_MASK ;

  switch (symbol)
  {
    case TK_SYM_CONST_NUM:
      listPrintf ("%s", symbolTable [index].name) ;
      return ;

    case TK_SYM_CONST_STR:
      listPrintf ("\"%s\"", symbolTable [index].name) ;
      return ;

    case TK_SYM_VAR_NUM:
    case TK_SYM_VAR_STR:
    case TK_SYM_VAR_NUM_ARR:
    case TK_SYM_VAR_STR_ARR:
    case TK_SYM_VAR_MAP:
      listPrintf ("%s", symbolTable [index].name) ;
      return ;

    case TK_SYM_REM1:
      if (!lineStart)
	listPrintf (" ") ;
      listPrintf ("REM%s", symbolTable [index].name) ;
      return ;

    case TK_SYM_REM2:
      if (!lineStart)
	listPrintf (" ") ;
      listPrintf ("//%s", symbolTable [index].name) ;
      return ;

    case TK_SYM_RESTORE:
      if (symbolTable [index].value.lineNumber == 0)
	listPrintf ("RESTORE") ;
      else
	listPrintf ("RESTORE %d", symbolTable [index].value.lineNumber) ;
      return ;

    case TK_SYM_GOTO:
      listPrintf ("GOTO %d", symbolTable [index].value.lineNumber) ;
      return ;

    case TK_SYM_GOSUB:
      listPrintf ("GOSUB %d", symbolTable [index].value.lineNumber) ;
      return ;

    case TK_SYM_PROC:
      listPrintf ("PROC %s", symbolTable [index].name) ;
      return ;

    case TK_SYM_FUNC:
      listPrintf ("FN %s", symbolTable [index].name) ;
      return ;

  }
  listPrintf ("[%04X:%04X:%04X]", token, symbol, index) ;
}


static void doListKeyword (int keyword)
{
  struct keywordStruct *kw = &keywords [keyword] ;
  uint16_t flags = kw->flags ;

  if ((flags & KYF_ARITH) != 0)	listPrintf (" ") ;
  if ((flags & KYF_REL)   != 0)	listPrintf (" ") ;

  if (keyword == TK_THEN)  listPrintf (" ") ;
  if (keyword == TK_TO)    listPrintf (" ") ;
  if (keyword == TK_STEP)  listPrintf (" ") ;

  if ((keyword == TK_CYCLE) && !lineStart)
    listPrintf (" ") ;

  listPrintf ((char *)kw->keyword) ;

  if ((flags & KYF_PROC)  != 0)	listPrintf (" ") ;
  if ((flags & KYF_FUNC)  != 0)	listPrintf (" ") ;
  if ((flags & KYF_ARITH) != 0)	listPrintf (" ") ;
  if ((flags & KYF_REL)   != 0)	listPrintf (" ") ;
  if ((flags == KYF_0)        )	listPrintf (" ") ;

  if (keyword == TK_COMMA) listPrintf (" ") ;

  if (keyword == TK_SEMICO) listPrintf (" ") ;

  if (keyword == TK_FOR) 
  {
    ++indent ;
    forLine = TRUE ;
  }

  if ((keyword == TK_CYCLE) && !forLine)
    ++indent ;

  if (keyword == TK_DO)
    ++indent ;

  if (keyword == TK_IF)
    ++indent ;

  if ((keyword == TK_SWITCH) || (keyword == TK_CASE) || (keyword == TK_DEFAULT))
    ++indent ;
}

/*
 * doList:
 *	List a segment (or all) of the program
 *********************************************************************************
 */

static void doList (int startLine, int endLine)
{
  struct lineNumberStruct *lp ;
  uint16_t *p ;
  uint16_t  token ;
  int       lineNumber, ptr, key ;
  char     *separator ;

  indent = 0 ;

  for (ptr = 0 ; ptr < numLines ; ++ptr)
  {
    if (keyPressed () && (listFd == NULL))
    {
      key = keyboardGetchar () ;
      if (key == K_ESCAPE)
      {
	listPrintf ("Escape\n") ;
	return ;
      }

      if (key == ' ')
      {
	listPrintf ("-- Pause -- %c", 4) ;
	(void)keyboardGetchar () ;
	listPrintf ("\r           \r") ;
      }
    }

    lp         = &programLines [ptr] ;
    p          = lp->data ;
    lineNumber = lp->lineNumber ;
    
    if ((lineNumber < startLine) || (lineNumber > endLine))
      continue ;
    
    lineStart = TRUE ;
    forLine   = FALSE ;
 
// Decrease indent?
    
    p = programLines [ptr].data ;
    if ((*p == TK_REPEAT)  || (*p == TK_NEXT)  ||
	(*p == TK_ELSE)    || (*p == TK_ENDIF) ||
	(*p == TK_ENDCASE) || (*p == TK_ENDSWITCH) )
      --indent ;

    separator = " " ;
    if (indent < 0)
    {
      separator = "#" ;
      indent    = 0 ;
    }

    if ((indent != 0) && (*p == TK_DEF))
    {
      separator = "#" ;
      indent    = 0 ;
    }

#ifdef	DEBUG_LIST
    listPrintf ("%3d: %5d", programLines [ptr].length, lineNumber) ;
#else
    if (doLineNumbers)
    {
      if (listToBuffer)
	listPrintf (  "%d ", lineNumber) ;
      else
	listPrintf ("%5d%s", lineNumber, separator) ;
    }
#endif

    spaces () ;

    if (*p == TK_ELSE)
      ++indent ;

    while ((token = *p++) != TK_SYM_EOL)
    {
      if ((token & TK_SYM_MASK) != 0)	// Symbol
	doListSymbol (token) ;
      else
	doListKeyword (token & ~TK_SYM_MASK) ;
      if ((token == TK_THEN) && !endOfLine (p))
	--indent ;
      lineStart = FALSE ;
    }
    listPrintf ("\n") ;
  }
  return ;
}


void doListCommand (int argc, char *argv [])
{
  int l1, l2 ;

  /**/ if (argc == 1)
    doList (0,MAX_LINENUM) ;
  else if (argc == 2)
  {
    l1 = atoi (argv [1]) ;
    doList (l1, l1) ;
  }
  else
  {
    l1 = atoi (argv [1]) ;
    l2 = atoi (argv [2]) ;
    doList (l1, l2) ;
  }
}


/*
 * doEditCommand:
 *	Handle the 'ED' command to allow us to edit a line
 *********************************************************************************
 */

void doEditCommand (int argc, char *argv [])
{
  int lineNum ;

  if (argc != 2)
    return (void)syntaxError ("ED: Need a line number") ;

  lineNum = atoi (argv [1]) ;

  listBuffer [0] = 0 ;
  listToBuffer = TRUE ;
    doList (lineNum, lineNum) ;
  listToBuffer = FALSE ;

  listBuffer [strlen (listBuffer) - 1] = 0 ;	// Chomp trailing NL

  screenPutchar ('>', TRUE) ;
  parseInput (readLine (listBuffer)) ;
}


/*
 * doSaveCommand:
 *	SAVE our program.
 *	Included here as it's going to use the LIST code...
 *********************************************************************************
 */

void doSaveCommand (int argc, char *argv [])
{
  char *f1, *fx ;

  if (numLines == 0)
    return (void)syntaxError ("SAVE: No program to save") ;

  /**/ if ((argc == 1) && (savedFilename != NULL))
    f1 = savedFilename ;
  else if (argc == 2)
    f1 = argv [1] ;
  else
    return (void)syntaxError ("SAVE <filename> [start [end]]") ;

  if (argc == 2)
  {
    if (savedFilename != NULL)
      free (savedFilename) ;

    if ((savedFilename = malloc (strlen (argv [1]) + 1)) == NULL)
      return (void)syntaxError ("SAVE: Out of memory") ;
    strcpy (savedFilename, argv [1]) ;
  }

  if ((fx = malloc (strlen (savedFilename) + 6)) == NULL)
    return (void)syntaxError ("SAVE: Out of memory") ;

  sprintf (fx, "%s.rtb", savedFilename) ;

  revokeRoot () ;
  if ((listFd = fopen (fx, "w")) == NULL)
  {
    screenPrintf ("SAVE: Unable to open \"%s\": %s\n", savedFilename, strerror (errno)) ;
    free (fx) ;
    free (savedFilename) ;
    listFd        = NULL ;
    savedFilename = NULL ;
  }
  else
  {
    screenPrintf ("*** Saving to: \"%s\"\n", savedFilename) ;
    doList (0, MAX_LINENUM) ;
    fclose (listFd) ;
    free (fx) ;
    listFd = NULL ;
  }
  regainRoot () ;
} 

void doSaveNNCommand (int argc, char *argv [])
{
  int l ;
  uint16_t *p ;
  uint16_t  symbol ;

// Scan program first:

  for (l = 0 ; l < numLines ; ++l)
    for (p = programLines [l].data ; *p != TK_SYM_EOL ; ++p)
    {
      symbol = *p & TK_SYM_MASK ;
      if ((symbol == TK_RESTORE) && ((*p & ~TK_SYM_MASK) == 0))
	continue ;
      if ((symbol == TK_SYM_GOTO) || (symbol == TK_SYM_GOSUB) || (symbol== TK_RESTORE) )
	return (void)syntaxError ("SAVENN: Not suitable for saving without line-numbers") ;
    }
  doLineNumbers = FALSE ;
    doSaveCommand (argc, argv) ;
  doLineNumbers = TRUE ;
}
