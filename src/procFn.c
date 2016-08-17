/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * procFn.c:
 *	Handle user-defined procedures and functions
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

#include "bool.h"

#include "keywords.h"
#include "lines.h"
#include "symbolTable.h"
#include "shuntingYard.h"
#include "rpnEval.h"

//#include "commands.h"

#include "run.h"
#include "procFn.h"

// Locals

static int activeFunctions = 0 ;
//static int activeProcs     = 0 ;

static uint32_t *procStack    = NULL ;
static int       procStackPtr = 0 ;
static int       procCount    = 0 ;


/*
 * clearProcs:
 *	Reset things back for a clean program exit, or new run
 *********************************************************************************
 */

void clearProcs (void)
{
  procCount    = 0 ;
  procStackPtr = 0 ;

  if (procStack != NULL)
    free (procStack) ;
}


/*
 * initProcs:
 *	Reset our stack pointer at the start of a new run
 *********************************************************************************
 */

int initProcs (void)
{
  clearProcs () ;

// Pre-Allocate the first stack chunk, to hopefully make small programs
//	run a bit smoother

  if ((procStack = calloc (PROC_STACK_CHUNK, sizeof (int))) == NULL)
    return syntaxError ("Out of memory pre-allocating the PROC stack") ;

  return TRUE ;
}


/*
 * pushProc: popProc:
 *	Manage the PROC/ENDPROC line number stack
 *********************************************************************************
 */

static int pushProc (int data)
{
  if (procStackPtr == procCount)
  {
    procCount += PROC_STACK_CHUNK ;
    if ((procStack = realloc (procStack, procCount * sizeof (int))) == NULL)
      return syntaxError ("Out of memory reallocating procStack space") ;
  }

  procStack [procStackPtr++] = data ;
  return TRUE ;
}

static int popProc (int *data)
{
  if (procStackPtr == 0)
    return FALSE ;

  *data = procStack [--procStackPtr] ;
  return TRUE ;
}


#ifdef	NOT_NEEDED
/*
 * pushFunc: popFunc:
 *	Manage the FN/= stack
 *	This stack is used to keep track of the symbolTableStack pointer
 *	so we know how many symbols to pop off that stack at the end of the
 *	function - we need this because we don't know how many LOCALs we'll
 *	allocate
 *********************************************************************************
 */

static uint32_t *funcStack    = NULL ;
static int       funcStackPtr = 0 ;
static int       funcCount    = 0 ;

static int pushFunc (int data)
{
  if (funcStackPtr == funcCount)
  {
    funcCount += FUNC_STACK_CHUNK ;
    if ((funcStack = realloc (funcStack, funcCount * sizeof (int))) == NULL)
      return syntaxError ("Out of memory reallocating funcStack space") ;
  }

  funcStack [funcStackPtr++] = data ;
  return TRUE ;
}

static int popFunc (int *data)
{
  if (funcStackPtr == 0)
    return FALSE ;

  *data = funcStack [--funcStackPtr] ;
  return TRUE ;
}
#endif


/*
 * doLocal:
 *	Handle the LOCAL instruction
 *********************************************************************************
 */

int doLocal (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  uint16_t  symbol, index ;

  if ((procCount + activeFunctions) == 0)
    return syntaxError ("LOCAL: Not in a PROC/FN") ;

  for (;;)
  {
    symbol = *p &  TK_SYM_MASK ;
    index  = *p & ~TK_SYM_MASK ;

    switch (symbol)
    {
      case TK_SYM_VAR_NUM:
      case TK_SYM_VAR_STR:
	pushSymbol (index) ;
	++p ;
	break ;

      case TK_SYM_VAR_NUM_ARR:
      case TK_SYM_VAR_STR_ARR:
	pushSymbol (index) ;
	if (! ((*(p + 1) == TK_BRA) && (*(p + 2) == TK_KET)))
	  return syntaxError ("LOCAL: Array must be empty") ;
	p += 3 ;
	break ;

      default:
	return syntaxError ("LOCAL: Unsupported variable type") ;
    }

    if (*p == TK_COMMA)
    {
      ++p ;
      continue ;
    }

    if (endOfLine (p))
      break ;
    else
      return syntaxError ("LOCAL: Variable expected") ;
  }
  ++linePtr ;
  return TRUE ;
}


/*
 * doDef:
 *	Just a trap in-case anyone executes a PROC or FN
 *********************************************************************************
 */

int doDef (void *ptr)
{
  return syntaxError ("DEF: Should not be executed") ;
}


/*
 * doProc:
 *	Handle a procedure call
 *********************************************************************************
 */

static int _doProc (int procLinePtr, int args)
{
  uint16_t *p ;
  uint16_t  symbol, sIndex ;
  int       braCount, type, i ;

  pushProc (linePtr + 1) ;
  pushProc (symbolTableStackPtr) ;
 
// Arguments are on the rpnStacks, right to left, but arguments in the
//	definition are left to right.
//
//	What we're doing here is taking the 'variables' off the defintion and
//	pushing them into the symbol stack and replacing them with the arguments
//	this effectively treats them as local variables for this procedure

// We need to get 'p' to the last )

  p = programLines [procLinePtr].data + 2 ;		// +2 - Skip over 'DEF' and 'PROCname'

  for (braCount = 0 ;; ++p)
  {
    if (endOfLine (p))
      return syntaxError ("PROC: Mismatched ()'s") ;

    if (*p == TK_BRA)
    {
      ++braCount ;
      continue ;
    }

    if (*p == TK_KET)
      if (--braCount == 0)
        break ;
  }

  for (i = 0 ; i < args ; ++i)
  {
    while ((*p & TK_SYM_MASK) == 0)
      --p ;

    type   = stackOrder [stackOrderPtr - 1] ;
    symbol = *p &  TK_SYM_MASK ;
    sIndex = *p & ~TK_SYM_MASK ;

    pushSymbol (sIndex) ;	// Preserve the original

    /**/ if (symbol == TK_SYM_VAR_NUM)
    {
      if (type == EVAL_STACK_NUM)
	storeRealVar     (sIndex, popN ()) ;
      else
	return syntaxError ("Wrong parameter type (number expected)") ;
    }
    else if (symbol == TK_SYM_VAR_STR)
    {
      if (type == EVAL_STACK_STR)
	storeStringVarNA (sIndex, popS ()) ;
      else
	return syntaxError ("Wrong parameter type (string expected)") ;
    }
    else if (symbol == TK_SYM_VAR_NUM_ARR)
    {
      if (type == EVAL_STACK_NUM_ARR)
	storeArray       (sIndex, popAn ()) ;
      else
	return syntaxError ("Wrong parameter type (numeric array expected)") ;
    }
    else if (symbol == TK_SYM_VAR_STR_ARR)
    {
      if (type == EVAL_STACK_STR_ARR)
	storeArray       (sIndex, popAs ()) ;
      else
	return syntaxError ("Wrong parameter type (string array expected)") ;
    }
    else
      return syntaxError ("Unsupported parameter type (help?)") ;

    --p ;
  }

  linePtr = procLinePtr + 1 ;	// Line after the DEF
  return TRUE ;
}

/*********************************************************************************/

int doProc (void *ptr)
{
  struct    symbolTableStruct *procSym ;
  uint16_t *p = (uint16_t *)ptr ;
  uint16_t *q ;
  uint16_t  c, symbol, stIndex ;
  int       ok, len, argsGot ;
  int       stack1, braCount ;

// Find the symbol and see if we have already found the line number

  symbol  = *(p-1) ;
  stIndex = symbol & ~TK_SYM_MASK ;

  procSym = &symbolTable [stIndex] ;

  if (procSym->value.linePtr == -1)
    return syntaxError ("PROC Not found") ;

// Check parameters

  if (endOfLine (p))
  {
    if (procSym->args == 0)
    {
      pushProc (linePtr + 1) ;			// Where we come back to
      pushProc (symbolTableStackPtr) ;
      linePtr = procSym->value.linePtr + 1 ;	// Line after the DEF
      return TRUE ;
    }
    else
      return syntaxError ("PROC: Parameter(s) required") ;
  }

  if (*p != TK_BRA)
      return syntaxError ("PROC: ( expected") ;

// OK... What we used to do was just call the shunter at this point then rpnEval it
//	leaving the args on the stack to push into the procedure and that was most
//	excellent... Until I decided to give it the ability to pass arrays into
//	procedures/functions.
//	Now it's a bother and we'll shunt/eval each individual argument

  argsGot = stackOrderPtr ;
  ++p ;
  while (!endOfLine (p))
  {

// Find comma or finishing )

    braCount = 0 ;
    for (q = p ; !endOfLine (q) ; ++q)
    {
      if ( ((*q == TK_COMMA) || (*q == TK_KET)) && (braCount == 0))
	break ;

      if (*q == TK_BRA)
      {
	++braCount ;
	continue ;
      }

      if (*q == TK_KET)
      {
	--braCount ;
	continue ;
      }
    }

    stack1 = oStackPtr ;

    c = *q ; *q = TK_SYM_EOL ;			// Temp. hack
    ok = shuntingYard (p, &len) ;
    *q = c ;					// Undo the hack
    if (!ok)
      return FALSE ;
    p = ++q ;

    if ((oStackPtr - stack1) == 2)	// One item
    {
      symbol = oStack [oStackPtr - 1] & TK_SYM_MASK ;
      if (symbol == TK_SYM_VAR_NUM_ARR)
      {
	pushAn (oStack [oStackPtr - 1]) ;
	oStackPtr -= 2 ;
	continue ;
      }
      if (symbol == TK_SYM_VAR_STR_ARR)
      {
	pushAs (oStack [oStackPtr - 1]) ;
	oStackPtr -= 2 ;
	continue ;
      }
    }

    if (!rpnEval ())
      return FALSE ;
  }

  argsGot = stackOrderPtr - argsGot ;

// Check the RPN evaluation stack for parameters

  if (argsGot != procSym->args)
  {
    /**/ if (procSym->args == 0)
      return syntaxError ("PROC: No parameters expected (got %d)", stackOrderPtr) ;
    else if (procSym->args == 1)
      return syntaxError ("PROC: Expected one parameter") ;
    else
      return syntaxError ("PROC: Expected %d parameters", procSym->args) ;
  }

 return _doProc (procSym->value.linePtr, procSym->args) ;
}


/*
 * doEndProc:
 *	Called by the interpreter (runLines and IF) when the
 *	ENDPROC instruction is encoutered
 *********************************************************************************
 */

int doEndProc (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  int sPtr ;

  if (!endOfLine (p))
    return syntaxError ("ENDPROC: Extra data") ;

  if (!popProc (&sPtr))
    return syntaxError ("ENDPROC without PROC") ;

  if (!popProc (&linePtr))
    return syntaxError ("ENDPROC without PROC") ;

  while (symbolTableStackPtr != sPtr)
    popSymbol () ;

  return TRUE ;
}


/*
 * doFunc:
 *	Handle a function call
 *
 * At a first glance it might look like it's the same as procedures, however
 *	functions are called from the rpnEvaluator and all the parameters have
 *	already been evaluated and setup on the stack, so it's not quite the
 *	same )-:
 *	This means that for now, arrays can only be passed into a function as
 *	the first argument as any arguments before the array will be treated
 *	as an array index and it will then dereference the array element...
 *********************************************************************************
 */

int doFunc (uint16_t token)
{
  struct    symbolTableStruct *s = &symbolTable [token & ~TK_SYM_MASK] ;
  register uint16_t *p ;
  uint16_t  symbol, sIndex ;
  int       braCount, type, i ;
  int       ok ;
  int       myLinePtr ;
  int       funcLinePtr, sPtr ;

  sPtr = symbolTableStackPtr ;

//  pushFunc (symbolTableStackPtr) ;

  funcLinePtr = s->value.linePtr ;

  if (funcLinePtr == -1)
    return syntaxError ("FN Not found") ;

  if (s->args != 0)
  {
    p = programLines [funcLinePtr].data + 2 ;		// +2 - Skip over 'DEF' and 'FNname'

    for (braCount = 0 ;; ++p)
    {
      if (endOfLine (p))
	return syntaxError ("FN: Mismatched ()'s") ;

      if (*p == TK_BRA)
      {
	++braCount ;
	continue ;
      }

      if (*p == TK_KET)
	if (--braCount == 0)
	  break ;
    }

    for (i = 0 ; i < s->args ; ++i)
    {
      while ((*p & TK_SYM_MASK) == 0)
	--p ;

      type   = stackOrder [stackOrderPtr - 1] ;
      symbol = *p &  TK_SYM_MASK ;
      sIndex = *p & ~TK_SYM_MASK ;

      pushSymbol (sIndex) ;

      switch (symbol)
      {
	case TK_SYM_VAR_NUM:
	  if (type == EVAL_STACK_NUM)
	    storeRealVar     (sIndex, popN ()) ;
	  else
	    return syntaxError ("Wrong parameter type (number expected)") ;
	  break ;

	case TK_SYM_VAR_STR:
	  if (type == EVAL_STACK_STR)
	    storeStringVarNA (sIndex, popS ()) ;
	  else
	    return syntaxError ("Wrong parameter type (string expected)") ;
	  break ;

	case TK_SYM_VAR_NUM_ARR:
	  if (type == EVAL_STACK_NUM_ARR)
	    storeArray       (sIndex, popAn ()) ;
	  else
	    return syntaxError ("Wrong parameter type (numeric array expected)") ;
	  break ;

	case TK_SYM_VAR_STR_ARR:
	  if (type == EVAL_STACK_STR_ARR)
	    storeArray       (sIndex, popAs ()) ;
	  else
	    return syntaxError ("Wrong parameter type (string array expected)") ;
	  break ;

	default:
	  return syntaxError ("Unsupported parameter type (help?)") ;
      }

      --p ;
    }
  }

  ++activeFunctions ;

  myLinePtr = linePtr ;
    linePtr = funcLinePtr + 1 ;

  ok = runLines () ;

  linePtr = myLinePtr ;
  --activeFunctions ;

  while (symbolTableStackPtr != sPtr)
    popSymbol () ;

  return ok ;
}


/*
 * doEndFunc:
 *	Called by the interpreter (runLines and IF) when a statement
 *	starting with = is encountered
 *********************************************************************************
 */

int doEndFunc (void *ptr)
{
  register uint16_t *p = (uint16_t *)ptr ;
  int len ;

  if (activeFunctions == 0)
    return syntaxError ("END-FN without FN") ;

  if (endOfLine (p))
    return syntaxError ("Function end: No Result") ;

  if (!shuntingYard (p, &len))
    return FALSE ;

  p += len ;
  if (!endOfLine (p))
    return syntaxError ("Function end: Extra Data") ;

  return rpnEval () ;
}
