/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * procedures.c:
 *	Execute built-in procedures
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
#include <ctype.h>
#include <math.h>
#include <time.h>

#include <unistd.h>

#include "rtb.h"
#include "bomb.h"
#include "bool.h"

#include "screenKeyboard.h"

#include "keywords.h"
#include "lines.h"
#include "symbolTable.h"
//#include "tokenise1.h"
#include "shuntingYard.h"
#include "rpnEval.h"
#include "array.h"

#include "commands.h"
#include "goto.h"
#include "procFn.h"
#include "run.h"
#include "assign.h"



/*
 * Simple procedures and operations with no arguments
 *********************************************************************************
 */

/*
 * doEnd: doStop:
 *	End of the program!
 *********************************************************************************
 */

int doEnd (void *ptr)
{
  return FALSE ;
}

int doStop (void *ptr)
{
  screenPrintf ("*** Stopped at line %d\n", programLines [linePtr].lineNumber) ;
  continuePtr = ++linePtr ;
  return FALSE ;
}


/*
 * doDeg: doRad: doClock:
 *	Set the global angle units
 *********************************************************************************
 */

int doDeg (void *ptr)
{
  angleConversion = M_PI / 180.0 ;
  return TRUE ;
}

int doRad (void *ptr)
{
  angleConversion = 1.0 ;
  return TRUE ;
}

int doClock (void *ptr)
{
  angleConversion = M_PI / 30.0 ;
  return TRUE ;
}


/*
 * Procedures with one argument
 *
 *********************************************************************************
 */

/*
 * doWait:
 *	Wait for some time...
 *********************************************************************************
 */

int doWait (void *ptr)
{
  int seconds ;
  double result ;
  struct timespec sleeper ;

  if (!oneNumber ())
    return syntaxError ("WAIT: Number expected") ;

  result =  popN () ;

  if (result == 0.0)	// Old, but still valid
    updateDisplay () ;
  else
  {
    seconds = (int)floor (result) ;
    while (seconds > 0)
    {
      sleep (1) ;
      if (escapePressed)
	return TRUE ;
      --seconds ;
    }

    sleeper.tv_sec  = 0 ;
    sleeper.tv_nsec = (long)((result - floor (result)) * 1.0e9) ;

    nanosleep (&sleeper, NULL) ;
  }

  return TRUE ;
}

/*
 * Procedures with two arguments
 *
 *********************************************************************************
 */


/*
 * doSwap:
 *	Swap 2 numbers or strings
 *********************************************************************************
 */

int doSwap (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  uint16_t  sym1, sym2, t1, t2, i1, i2 ;
  int       here, then, len ;
  double    tmpN ;
  char     *tmpS ;

  then = oStackPtr ;
  if (!shuntingYard (p, &len))
    return FALSE ;
  here = oStackPtr ;

  if ((here - then) != 3)
    return syntaxError ("SWAP: No data") ;

  p += len ;
  if (!endOfLine (p))
    return syntaxError ("SWAP: Extra data after parameters") ;

  sym1 = oStack [here - 1] ;
  t1   = sym1 &  TK_SYM_MASK ;
  i1   = sym1 & ~TK_SYM_MASK ;
  sym2 = oStack [here - 2] ;
  t2   = sym2 &  TK_SYM_MASK ;
  i2   = sym2 & ~TK_SYM_MASK ;

  if ((t1 == TK_SYM_VAR_NUM) && (t2 == TK_SYM_VAR_NUM))
  {
    tmpN                           = symbolTable [i1].value.realVal ;
    symbolTable [i1].value.realVal = symbolTable [i2].value.realVal ;
    symbolTable [i2].value.realVal = tmpN ;
    ++linePtr ;
    return TRUE ;
  }
  else if ((t1 == TK_SYM_VAR_STR) && (t2 == TK_SYM_VAR_STR))
  {
    tmpS                             = symbolTable [i1].value.stringVal ;
    symbolTable [i1].value.stringVal = symbolTable [i2].value.stringVal ;
    symbolTable [i2].value.stringVal = tmpS ;
    ++linePtr ;
    return TRUE ;
  }
  else
    return syntaxError ("SWAP: Invalid types") ;
}

/*
 * doDebug:
 *	Whatever
 *********************************************************************************
 */

int doDebug (void *ptr)
{
  int x ;

  if (!oneNumber ())
    return syntaxError ("DEBUG: Number expected") ;

  x =  (int)rint (popN ()) ;
  if ((x & 0x01) != 0)	dumpShuntingYard ("dbg") ;
  if ((x & 0x02) != 0)	dumpRpnStack     ("dbg") ;
  if ((x & 0x04) != 0)	dumpSymbolTable  ("dbg") ;

  return TRUE ;
}



/*
 * Complex procedures and operations
 ********************************************************************************* 
 */

/*
 * doNumFormat:
 *	Set the printing number format.
 *********************************************************************************
 */

int doNumFormat (void *ptr)
{
  int width, places ;
  
  if (!twoNumbers ())
    return syntaxError ("NUMFORMAT: Expected two numbers") ;
  
  places = (int)floor (popN ()) ;
  width  = (int)floor (popN ()) ;

  if (width == 0)
    strcpy (fmtString, DEFAULT_NUM_FORMAT) ;
  else
    sprintf (fmtString, "%%%d.%df", width, places) ;

  return TRUE ;
}


/*
 * doPrint
 *	PRINT ...
 ********************************************************************************* 
 */

int doPrint (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  int       len = 0 ;
  double    resultN ;
  char     *resultS ;

  while (!endOfLine (p))
  {
    if (*p == TK_SEMICO)	// Skip over semicolons
      { ++p ; continue ; }

    if (!shuntingYard (p, &len))
      return FALSE ;

    if (len == 0)
      return syntaxError ("PRINT: Invalid items") ;

    if (!rpnEval ())
      return FALSE ;

// Check the top of the stack and use that as the base type to evaluate

    /**/ if (stackOrder [stackOrderPtr - 1] == EVAL_STACK_NUM)
    {
      resultN = popN () ;
      screenPrintf (fmtString, resultN) ;
    }
    else if (stackOrder [stackOrderPtr - 1] == EVAL_STACK_STR)
    {
      resultS = popS () ;
      screenPuts (resultS) ;
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
      return syntaxError ("PRINT: Don't know how to print that") ;

    p += len ;
  }

  if (*--p != TK_SEMICO)
    screenPutchar ('\n', TRUE) ;

  ++linePtr ;
  return TRUE ;
}


/*
 * doRead:
 *	Read data into variables
 *********************************************************************************
 */

static void nextData (void)
{
  int x ;
  uint16_t *p ;

// Next symbol

  ++dataPtr ;

// Skip over a comma

  if (*dataPtr == TK_COMMA)
    ++dataPtr ;

// If not EOL, then we just return - we don't care at this stage what it is, just that
//	it's not EOL (Read will sort that out)

  if (!endOfLine (dataPtr))
    return ;

// EOL - Find the next DATA line

  ++dataIndex ;
  for (x = dataIndex ; x < numLines ; ++x)
  {
    p = programLines [x].data ;
    if (*p == TK_DATA)
    {
      dataPtr   = p + 1 ;
      dataIndex = x ;
      return ;
    }
  }

  dataPtr   = NULL ;
  dataIndex = -1 ;
}

int doRead (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  struct    symbolTableStruct *st ;
  uint16_t  symbol, index ;
  uint16_t  dSymbol, dIndex ;
  int       len, arrayIndex ;


  for (;;)
  {
    if (dataPtr == NULL)
      return syntaxError ("READ: Out of data") ;

    symbol = *p &  TK_SYM_MASK ;
    index  = *p & ~TK_SYM_MASK ;

    dSymbol = *dataPtr  &  TK_SYM_MASK ;
    dIndex  = *dataPtr  & ~TK_SYM_MASK ;
    st      = &symbolTable [index] ;

    switch (symbol)
    {
	case TK_SYM_VAR_NUM:
	if (dSymbol != TK_SYM_CONST_NUM)
	  return syntaxError ("READ: Invalid data type (string when number expected)") ;

	storeRealVar (index, symbolTable [dIndex].value.realVal) ;
	++p ;
	break ;

      case TK_SYM_VAR_STR:
	if (dSymbol != TK_SYM_CONST_STR)
	  return syntaxError ("READ: Invalid data type (number when string expected)") ;

/*
	if (st->value.stringVal != NULL)
	  free (symbolTable [index].value.stringVal) ;
*/

	storeStringVar (index, symbolTable [dIndex].name) ;
	++p ;
	break ;

      case TK_SYM_VAR_NUM_ARR:
	if (dSymbol != TK_SYM_CONST_NUM)
	  return syntaxError ("READ: Invalid data type (string when number expected)") ;

	if (!getArrayIndex (p, &len, &arrayIndex))
	  return FALSE ;

	st->value.realArray [arrayIndex] = symbolTable [dIndex].value.realVal ;
	p += len ;
	break ;

      case TK_SYM_VAR_STR_ARR:
	if (dSymbol != TK_SYM_CONST_STR)
	  return syntaxError ("READ: Invalid data type (number when string expected)") ;

	if (!getArrayIndex (p, &len, &arrayIndex))
	  return FALSE ;

	if (st->value.stringArray [arrayIndex] != NULL)
	  free (st->value.stringArray [arrayIndex]) ;
	st->value.stringArray [arrayIndex] = malloc (strlen (symbolTable [dIndex].name) + 1) ;
	strcpy (st->value.stringArray [arrayIndex], symbolTable [dIndex].name) ;
	p += len ;
	break ;

      default:
	return syntaxError ("READ: Bad variable") ;
    }

// Bump data pointer

    nextData () ;

// Bump read pointer (over a comma, if present)

    if (*p == TK_COMMA)
      ++p ;

    if (endOfLine (p))
    {
      ++linePtr ;
      return TRUE ;
    }
  }
}


/*
 * doData:
 *	Skip over DATA instructions
 *********************************************************************************
 */

int doData (void *ptr)
{
  ++linePtr ;
  return TRUE ;
}


/*
 * doRestore:
 *	Restore the DATA pointer
 *********************************************************************************
 */

int doRestore (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  struct    symbolTableStruct *s ;
  int       newLinePtr, newLineNumber ;

  if (!endOfLine (p))
    return syntaxError ("RESTORE: Bad number") ;

  s = &symbolTable [*(p-1) & ~TK_SYM_MASK] ;

// Full restore?

  if (s->value.lineNumber == 0)
  {
    if (firstDataPtr == NULL)
      return syntaxError ("RESTORE: No Data") ;

    dataPtr   = firstDataPtr ;
    dataIndex = firstDataIndex ;
    ++linePtr ;
    return TRUE ;
  }

// Find the line

  newLineNumber = s->value.lineNumber ;
  if ((newLinePtr = findLine (newLineNumber)) == -1)
    return syntaxError ("RESTORE: Line %d does not exist", newLineNumber) ;

  p = programLines [newLinePtr].data ;
  if (*p == TK_DATA)
  {
    dataPtr   = p + 1 ;
    dataIndex = newLinePtr ;
    ++linePtr ;
    return TRUE ;
  }

  return syntaxError ("RESTORE: No data on line %d", newLineNumber) ;

}


/*
 * doDim:
 *	Create a new array
 *********************************************************************************
 */

int doDim (void *ptr)
{
  struct    symbolTableStruct *st ;
  uint16_t *p = (uint16_t *)ptr ;
  uint16_t *q ;
  uint16_t  saved, variable, index, symbol ;
  int       i, size, start, len, dim, dims, braCount ;

  for (;;)
  {
    variable = *p++ ;
    symbol   = variable &  TK_SYM_MASK ;
    index    = variable & ~TK_SYM_MASK ;

    st = &symbolTable [index] ;

    if (! ((symbol == TK_SYM_VAR_NUM_ARR) || (symbol == TK_SYM_VAR_STR_ARR) || (symbol == TK_SYM_VAR_MAP)))
      return syntaxError ("DIM: Invalid variable type") ;

    if (st->arrayDims != 0)
      return syntaxError ("DIM: Can't re-dimension") ;

// We need to trick the shunter to stop at the last TK_KET, so we'll
//	scan for it and replace the token after it with an EOL

    q = p ;
    if (*q++ != TK_BRA)
      return syntaxError ("DIM: ( expected") ;

    braCount = 1 ;
    for (;;++q)
    {
      if (endOfLine (q))
	return syntaxError ("DIM: Mismatched ()'s") ;

      if (*q == TK_BRA)
      {
	++braCount ;
	continue ;
      }

      if (*q == TK_KET)
	if (--braCount == 0)
	  break ;
    }
    ++q ;
    saved = *q ;
    *q = TK_SYM_EOL ;

// Call the shunter to work out the array indicies

    if (!shuntingYard (p, &len))
      return FALSE ;

    *q = saved ;	// un-do our little hack

    start = stackOrderPtr ;

    if (!rpnEval ())
      return FALSE ;

    dims = stackOrderPtr - start ;

    st->arrayDims  = dims ;
    st->dimensions = calloc (sizeof (int), dims) ;

    size = 1 ;
    for (i = 0 ; i < dims ; ++i)
    {
      if (!oneNumber ())
	return syntaxError ("DIM: Bad dimension") ;

      dim = (int)rint (popN ()) + 1 ;	// BASIC arrays go from 0 to /top/ inclusive...
      if (dim < 2)
	return syntaxError ("DIM: Array dimension must be at least 1") ;

      size *= dim ;
      symbolTable [index].dimensions [i] = dim ;
    }

    /**/ if (symbol == TK_SYM_VAR_STR_ARR)
    {
      if ((symbolTable [index].value.stringArray = calloc (sizeof (char *), size + 1)) == NULL)
	return syntaxError ("DIM: Out of memory") ;
    }
    else if (symbol == TK_SYM_VAR_NUM_ARR)
    {
      if ((symbolTable [index].value.realArray   = calloc (sizeof (double), size + 1)) == NULL)
	return syntaxError ("DIM: Out of memory") ;
    }
    else	// Map
    {
      if ((symbolTable [index].value.map         = calloc (sizeof (char *), size + 1)) == NULL)
	return syntaxError ("DIM: Out of memory") ;
    }

// Any more?

    p = q ;
    if (*p == TK_COMMA)
      ++p ;
    else
      break ;
  }

  if (!endOfLine (p))
    return syntaxError ("DIM: Extra data") ;

  ++linePtr ;
  return TRUE ;
}


/*
 * doEndif:
 *	Ignored - we just jump over it
 *********************************************************************************
 */

int doEndif (void *ptr)
{
  ++linePtr ;
  return TRUE ;
}


/*
 * doElse:
 *	We've hit an else statement.. Need to scan forward for an ENDIF
 *********************************************************************************
 */

int doElse (void *ptr)
{
  uint16_t *p ;
  int l ;
  int count = 0 ;

  for (l = linePtr + 1 ; l < numLines ; ++l)
  {
    p = programLines [l].data ;

// Some things indicate a runnaway IF...

    if (*p == TK_DEF)
      return syntaxError ("Runnaway IF (no ENDIF)") ;

// Another IF ?

    if (*p == TK_IF)
    {
      while (*p != TK_THEN)
      {
	if (endOfLine (p))
	{
	  linePtr = l ;
	  return syntaxError ("ELSE: Found IF without THEN") ;
	}
	++p ;
      }

      ++p ;

      if (endOfLine (p))	// Multi-Line IF
	++count ;

      continue ;
    }

    if (*programLines [l].data == TK_ENDIF)
    {
      if (count == 0)
      {
	linePtr = l + 1 ;
	return TRUE ;
      }
      --count ;
      continue ;
    }

  }

// No matching endif lines

  return syntaxError ("ELSE: Missing ENDIF") ;
}


/*
 * findElse:
 *	Scan program lines to find the next ELSE statement, however we can also
 *	return OK if we find a matching ENDIF statement.
 *********************************************************************************
 */

static int findElse (void)
{
  register uint16_t *p ;
  int l ;
  int count = 0 ;

  for (l = linePtr + 1 ; l < numLines ; ++l)
  {
    p = programLines [l].data ;

// Some things indicate a runnaway IF...

    if (*p == TK_DEF)
      return syntaxError ("Runnaway IF (no ELSE/ENDIF)") ;

// Another IF ?

    if (*p == TK_IF)
    {
      while (*p != TK_THEN)
      {
	if (endOfLine (p))
	{
	  linePtr = l ;
	  return syntaxError ("IF: Found IF without THEN") ;
	}
	++p ;
      }

      ++p ;

      if (endOfLine (p))	// Multi-Line IF?
	++count ;
      continue ;
    }

// ENDIF?

    if (*programLines [l].data == TK_ENDIF)
    {
      if (count == 0)
      {
	linePtr = l + 1 ;
	return TRUE ;
      }
      --count ;
     continue ;
    }

// ELSE

    if (*programLines [l].data == TK_ELSE)
    {
      if (count == 0)
      {
	linePtr = l + 1 ;
	return TRUE ;
      }
    }
  }

// No else lines

  return syntaxError ("IF: Missing ELSE/ENDIF") ;
}


/*
 * doIf:
 *	Start of IF instruction processing
 *********************************************************************************
 */

int doIf (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  uint16_t  token, symbol ;
  int       len ;
  double    result ;
  int       ok ;
  int      multiLine = FALSE ;

  if (!shuntingYard (p, &len))
    return FALSE ;

  p += len ;
  if (*p != TK_THEN)
    return syntaxError ("IF: Missing THEN") ;

  ++p ;

  if (endOfLine (p))		// End of line indicates multi-line IF
    multiLine = TRUE ;

  if (!rpnEvalNum (&result))
    return FALSE ;

  if (result == 0.0)		// 0 is FALSE, anything else considered TRUE
  {
    if (multiLine)
      return findElse () ;
    else
    {
      ++linePtr ;
      return TRUE ;
    }
  }

// The test is positive - do the action

  if (multiLine)
  {
    ++linePtr ;
    return TRUE ;
  }

// Note:
//	Compare code here with code in run.c
//	If changes made here, then we may have to change that too.

  token  = *p++ ;
  symbol = token &  TK_SYM_MASK ;

// Keyword or Symbol?

  if (symbol == 0)		// Built-in keyword/token
  {

// If we get an EQUALS at the start, then we have to return via
//	a call to doEndFunc as in all likliehood we're ending a
//	user-defined function/procedure and have been called recursively

    if (token == TK_EQUALS)	// Special
    {
      ok = doEndFunc (p) ;
      breakOut = TRUE ;
      return ok ;
    }

    return runKeyword (&keywords [token], p) ;
  }
  else
  {
    switch (symbol)
    {
      case TK_SYM_RESTORE:	ok = doRestore (p) ;	break ;
      case TK_SYM_GOTO:		ok = doGoto    (p) ;	break ;
      case TK_SYM_GOSUB:	ok = doGosub   (p) ;	break ;
      case TK_SYM_PROC:		ok = doProc    (p) ;	break ;

      case TK_SYM_VAR_NUM:
      case TK_SYM_VAR_STR:
	ok = doAssignment (p - 1) ;
	++linePtr ;
	break ;

      case TK_SYM_VAR_NUM_ARR:
      case TK_SYM_VAR_STR_ARR:
	ok = doArrayAssignment (p - 1) ;
	++linePtr ;
	break ;

      default:
	return syntaxError ("IF: Invalid or unsupported instruction after THEN") ;
    }
    return ok ;
  }

}
