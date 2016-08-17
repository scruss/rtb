/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 *********************************************************************************
 * fileFns.c:
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
#include <errno.h>
#include <string.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <unistd.h>

//#include "bomb.h"

#include "bool.h"
#include "keywords.h"
#include "symbolTable.h"

#include "screenKeyboard.h"
#include "readline.h"

#include "shuntingYard.h"
#include "rpnEval.h"

#include "rtb.h"
#include "fileFns.h"


static FILE *fileFdMap [MAX_OPEN_FILES] ;

/*
 * getFd:
 *	Return our internal mapping of handle to file descriptor
 *********************************************************************************
 */

static FILE *getFd (int handle)
{
  if ((handle < 0) || (handle >= MAX_OPEN_FILES))
    return NULL ;
  else
    return fileFdMap [handle] ;
}


/*
 * doOpenIn
 *	Open a file for input only.
 *********************************************************************************
 */

int doOpenIn (void *ptr)
{
  FILE *fd ;
  char *s ;
  int   i, handle = -1 ;

  if (!oneString ())
    return syntaxError ("OPENIN: String expected") ;

  s = popS () ;

  revokeRoot () ;
  fd = fopen (s, "r") ;
  regainRoot () ;
  if (fd == NULL)
  {
    pushN (-1.0) ;
    return TRUE ;
  }

  rewind (fd) ;

// Find an empty slot

  for (i = 0 ; i < MAX_OPEN_FILES ; ++i)
    if (fileFdMap [i] == NULL)
    {
      fileFdMap [i] = fd ;
      handle = i ;
      break ;
    }

  if (handle == -1)	// Too many serial ports open! (Realy? Wow...)
  {
    fclose (fd) ;
    return syntaxError ("OPENIN: Too many files") ;
    return FALSE ;
  }

  fileFdMap [handle] = fd ;

  free  (s) ;
  pushN ((double)handle) ;

  return TRUE ;
}


/*
 * doOpenUp
 *	Open a file for update.
 *********************************************************************************
 */

int doOpenUp (void *ptr)
{
  FILE *fd ;
  char *s ;
  int   i, handle = -1 ;

  if (!oneString ())
    return syntaxError ("OPENUP: String expected") ;

  s = popS () ;

  revokeRoot () ;
  fd = fopen (s, "r+") ;
  regainRoot () ;

  if (fd == NULL)
  {
    if (errno == ENOENT)	// We'll try to create the file if it's not there
    {
      revokeRoot () ;
      fd = fopen (s, "w+") ;
      regainRoot () ;
    }

    if (fd == NULL)
    {
      pushN (-1.0) ;
      return TRUE ;
    }
  }

  rewind (fd) ;

// Find an empty slot

  for (i = 0 ; i < MAX_OPEN_FILES ; ++i)
    if (fileFdMap [i] == NULL)
    {
      fileFdMap [i] = fd ;
      handle = i ;
      break ;
    }

  if (handle == -1)	// Too many serial ports open! (Realy? Wow...)
  {
    fclose (fd) ;
    return syntaxError ("OPENUP: Too many files") ;
    return FALSE ;
  }

  fileFdMap [handle] = fd ;

  free  (s) ;
  pushN (handle) ;

  return TRUE ;
}


/*
 * doOpenOut
 *	Open a file for writing.
 *********************************************************************************
 */

int doOpenOut (void *ptr)
{
  FILE *fd ;
  char *s ;
  int   i, handle = -1 ;

  if (!oneString ())
    return syntaxError ("OPENOUT: String expected") ;

  s = popS () ;

  revokeRoot () ;
  fd = fopen (s, "w") ;
  regainRoot () ;

  if (fd == NULL)
  {
    if (fd == NULL)
    {
      pushN (-1.0) ;
      return TRUE ;
    }
  }

  rewind (fd) ;

// Find an empty slot

  for (i = 0 ; i < MAX_OPEN_FILES ; ++i)
    if (fileFdMap [i] == NULL)
    {
      fileFdMap [i] = fd ;
      handle = i ;
      break ;
    }

  if (handle == -1)	// Too many serial ports open! (Realy? Wow...)
  {
    fclose (fd) ;
    return syntaxError ("OPENOUT: Too many files") ;
    return FALSE ;
  }

  fileFdMap [handle] = fd ;

  free  (s) ;
  pushN (handle) ;

  return TRUE ;
}


/*
 * doClose:
 *	Close a file already opened
 *********************************************************************************
 */

int doClose (void *ptr)
{
  FILE *fd ;
  int   handle ;
  
  if (!oneNumber ())
    return syntaxError ("CLOSE: Expected one number") ;
  
  handle = (int)rint (popN ()) ;

  if ((fd = getFd (handle)) == NULL)
    return syntaxError ("CLOSE: Invalid handle") ;

  if (fclose (fd) == 0)
  {
    fileFdMap [handle] = NULL ;
    return TRUE ;
  }

  return syntaxError ("CLOSE: File close failed: %s", strerror (errno)) ;
}


/*
 * doRewind:
 *	Reset the pointer to the start of the file
 *********************************************************************************
 */

int doRewind (void *ptr)
{
  FILE *fd ;
  int   handle ;
  
  if (!oneNumber ())
    return syntaxError ("REWIND: Expected one number") ;
  
  handle = (int)rint (popN ()) ;

  if ((fd = getFd (handle)) == NULL)
    return syntaxError ("REWIND: Invalid handle") ;

  rewind (fd) ;
  return TRUE ;
}


/*
 * doFfwd:
 *	Fast-forward the file pointer to the end
 *********************************************************************************
 */

int doFfwd (void *ptr)
{
  FILE *fd ;
  int   handle ;
  
  handle = (int)rint (popN ()) ;

  if ((fd = getFd (handle)) == NULL)
    return syntaxError ("FFWD: Invalid handle") ;

  fseek (fd, 0L, SEEK_END) ;
  return TRUE ;
}


/*
 * doSeek:
 *	Seek to a particular location
 *********************************************************************************
 */

int doSeek (void *ptr)
{
  FILE *fd ;
  int   handle ;
  long  offset ;
  
  if (!twoNumbers ())
    return syntaxError ("SEEK: Expected two numbers") ;
  
  offset = (long)rint (popN ()) ;
  handle =  (int)rint (popN ()) ;

  if ((fd = getFd (handle)) == NULL)
    return syntaxError ("SEEK: Invalid handle") ;

  if (fseek (fd, offset, SEEK_SET) != 0)
    return syntaxError ("SEEK: Fail: %s", strerror (errno)) ;

  return TRUE ;
}


/*
 * doEOF:
 *	Return a files EOF status.
 *
 * Note: Unix has a funny notion of EOF - well, not funny, but often
 *	misunderstood... EOF is not set until an actual EOF condition
 *	has arisen, so reading to the end of the file, but no more
 *	does not set any EOF status - because technically the end of
 *	file hasn't been reached!
 *	However, in BASIC, we really don't want to read past the end
 *	of the file, so we're going to stat the file every time we
 *	check and test the length against the current position. This
 *	is innefficient and will also fail if we're reading records from a
 *	sparse file, but there you go.
 *********************************************************************************
 */

int doEOF (void *ptr)
{
  FILE *fd ;
  int   handle ;
  struct stat sbuf ;

  if (!oneNumber ())
    return FALSE ;

  handle = (int)rint (popN ()) ;

  if ((fd = getFd (handle)) == NULL)
    return syntaxError ("EOF: Invalid handle") ;

  if (fstat (fileno (fd), &sbuf) != 0)
    return syntaxError ("EOF: File IO error") ;

//printf ("EOF: %ld - %ld\n", sbuf.st_size, ftell (fd)) ;

  pushN (sbuf.st_size == ftell (fd) ? 1.0 : 0.0) ;
  return TRUE ;
}


/*
 * doInput#:
 *	Input some data from a file...
 *********************************************************************************
 */

int doInputX (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  uint16_t *q ;
  uint16_t  symbol, type, index ;
  FILE     *fd ;
  double    result ;
  char      buffer [1024], *c ;
  int       handle, ok, len ;

// We need to evaluate the expression at *p, however we need to stop
//	the evaluator at the first comma, so we'll search for it and
//	temporarilly change it..

  for (q = p + 1 ; *q != TK_COMMA ; ++q)
    if (endOfLine (q))
      return syntaxError ("INPUT#: Comma expected") ;

  *q = TK_SYM_EOL ;
    ok = shuntingYard (p, &len) ;
  *q = TK_COMMA ;

  if (!ok)
    return FALSE ;

  if (!rpnEvalNum (&result))
    return FALSE ;

  handle = (int)rint (result) ;

  if ((fd = getFd (handle)) == NULL)
    return syntaxError ("INPUT#: Invalid handle") ;

  fseek (fd, 0L, SEEK_CUR) ;	// ANSI Requirement

  p = p + len  + 1 ;	// Skip over the comma

  symbol = *p++ ;
  type   = symbol &  TK_SYM_MASK ;
  index  = symbol & ~TK_SYM_MASK ;

  if (! ((type == TK_SYM_VAR_NUM) || (type == TK_SYM_VAR_STR)))
    return syntaxError ("INPUT#: Scalar variable expected") ;

  if (!endOfLine (p))
    return syntaxError ("INPUT#: Extra data at end of line") ;

  if (fgets (buffer, 1024, fd) == NULL)
    return syntaxError ("INPUT#: File error: %s", strerror (errno)) ;

// Remove any trailing new line
//	Trying to cope with any combination of CR & LF...

  for (c = buffer + strlen (buffer) - 1 ; (c >= buffer) && isspace (*c) ; --c)
    *c = 0 ;

  if (type == TK_SYM_VAR_STR)			// String
  {
    if (!storeStringVar (index, buffer))
      return syntaxError ("INPUT: Out of memory") ;
  }
  else						// Number
  {
    if (isdigit (*buffer)							||	// Simple
      ( (*buffer == '.') &&  isdigit (*(buffer+1)))				||	// Start with a dot
      ( (*buffer == '-') && (isdigit (*(buffer+1)) || (*(buffer+1) == '.')))	||	// Negative
      ( (*buffer == '+') && (isdigit (*(buffer+1)) || (*(buffer+1) == '.'))))		// Positive
      sscanf (buffer, "%lf", &result) ;
    else
      result = 0.0 ;
    storeRealVar (index, result) ;
  }

  ++linePtr ;
  return TRUE ;
}

int doPrintX (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  uint16_t *q ;
  FILE     *fd ;
  double    resultN ;
  char     *resultS ;
  int       handle, ok, len ;

// We need to evaluate the expression at *p, however we need to stop
//	the evaluator at the first comma, so we'll search for it and
//	temporarilly change it..

  for (q = p + 1 ; *q != TK_COMMA ; ++q)
    if (endOfLine (q))
      return syntaxError ("PRINT#: Comma expected") ;

  *q = TK_SYM_EOL ;
    ok = shuntingYard (p, &len) ;
  *q = TK_COMMA ;

  if (!ok)
    return FALSE ;

  if (!rpnEvalNum (&resultN))
    return FALSE ;

  handle = (int)rint (resultN) ;

  if ((fd = getFd (handle)) == NULL)
    return syntaxError ("PRINT#: Invalid handle") ;

  fseek (fd, 0L, SEEK_CUR) ;	// ANSI Requirement

  p = p + len  + 1 ;	// Skip over the comma

  while (!endOfLine (p))
  {
    if (*p == TK_SEMICO)	// Skip over semicolons
      { ++p ; continue ; }

    if (!shuntingYard (p, &len))
      return FALSE ;

    if (len == 0)
      return syntaxError ("PRINT#: Invalid items") ;

    if (!rpnEval ())
      return FALSE ;

// Check the top of the stack and use that as the base type to evaluate

    /**/ if (stackOrder [stackOrderPtr - 1] == EVAL_STACK_NUM)
    {
      resultN = popN () ;
      fprintf (fd, fmtString, resultN) ;
    }
    else if (stackOrder [stackOrderPtr - 1] == EVAL_STACK_STR)
    {
      resultS = popS () ;
      fprintf (fd, "%s", resultS) ;
      free (resultS) ;
    }
    else if (stackOrder [stackOrderPtr - 1] == EVAL_STACK_NUM_ARR)
    {
      (void)popAn () ;
      screenPuts ("(num-array)") ;
    }
    else if (stackOrder [stackOrderPtr - 1] == EVAL_STACK_STR_ARR)
    {
      (void)popAs () ;
      screenPuts ("(str-array)") ;
    }
    else
      return syntaxError ("PRINT#: Don't know how to print that") ;

    p += len ;
  }

  if (*--p != TK_SEMICO)
    fprintf (fd, "\n") ;

  ++linePtr ;
  return TRUE ;
}


/*
 * fileCloseAll:
 *	Close all serial ports - used at end of program run
 *********************************************************************************
 */

void fileCloseAll (void)
{
  int i ;

  for (i = 0 ; i < MAX_OPEN_FILES ; ++i)
    if (fileFdMap [i] != NULL)
    {
      fclose (fileFdMap [i]) ;
      fileFdMap [i] = NULL ;
    }
}


/*
 * setupFiles:
 *	Initialise our mapping table
 *********************************************************************************
 */

void setupFiles (void)
{
  int i ;

  for (i = 0 ; i < MAX_OPEN_FILES ; ++i)
    fileFdMap [i] = NULL ;
}
