/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * symbolTable.c:
 *	Code to maintain a dynamic symbol table.
 *	Each symbol is a number in the source
 *
 * e.g.	a = 35.2
 *
 *	we store the string "35.2" and it's binary value.
 *
 * e.g.	counter = 7
 *
 *	we store the string "counter" and its type
 *
 * e.g.	name$ = "Gordon"
 *
 *	This stored 2 symbols - one for the variable name, one for the
 *	static value.
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
//#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <string.h>

#include "rtb.h"
#include "bomb.h"
#include "bool.h"

//#include "screenKeyboard.h"

#include "lines.h"
#include "keywords.h"
#include "symbolTable.h"


// Globals

int numSymbols ;
int symbolTableStackPtr ;
struct symbolTableStruct *symbolTable ;

// Locals

static int symbolTableStackMax ;
static struct symbolTableStruct *symbolTableStack ;

#ifdef	DEBUG_SYMBOL_TABLE

void dumpSymbolTable (char *prefix)
{
  struct symbolTableStruct *st ;

  int type, did ;
  uint16_t i, j, sym, x ;

  printf ("%s: dumpSymbolTable: %d symbols, symbolTableStackMax/Ptr: %d/%d\n", prefix, numSymbols,
	symbolTableStackMax, symbolTableStackPtr) ;

  for (type = 0 ; type < 16 ; ++type)
  {
    did = FALSE ;
    sym = type << 12 ;
    for (i = 0 ; i < numSymbols ; ++i)
    {
      st = &symbolTable [i] ;
      if (st->type == sym)
      {
	if (!did)
	{
	  printf ("  Type: %s:\n", symbolNames [type]) ;
	  did = TRUE ;
	}

	x = sym | i ;

	switch (sym)
	{
	  case TK_SYM_CONST_NUM:
	    printf ("%6d: %04X: %12s V: %lf\n",       	i, x, st->name, st->value.realVal) ;
	    break ;

	  case TK_SYM_CONST_STR:
	    printf ("%6d: %04X: %12s\n",			i, x, st->name) ;
	    break ;

	  case TK_SYM_VAR_NUM:
	    printf ("%6d: %04X: %12s W:%5d, V: %lf\n",
			i, x, st->name, st->writeCount, st->value.realVal) ;
	    break ;

	  case TK_SYM_VAR_STR:
	    printf ("%6d: %04X: %12s W:%5d, V: \"%s\"\n",
			i, x, st->name, st->writeCount, st->value.stringVal) ;
	    break ;

	  case TK_SYM_VAR_STR_ARR:
	  case TK_SYM_VAR_NUM_ARR:
	  case TK_SYM_VAR_MAP:
	    printf ("%6d: %04X: %12s D: %d (",	i, x, st->name, st->arrayDims) ;
	    for (j = 0 ; j < st->arrayDims ; ++j)
	    {
	      printf ("%d", st->dimensions [j]) ;
	      if (j != (st->arrayDims - 1))
		printf (",") ;
	    }
	    printf (")\n") ;
	    break ;

	  case TK_SYM_GOTO:
	    printf ("%6d: \"GOTO\" Line: %d\n",	i, st->value.lineNumber) ;			break ;
	  case TK_SYM_GOSUB:
	    printf ("%6d: \"GOSUB\" Line: %d\n",	i, st->value.lineNumber) ;			break ;
	  case TK_SYM_RESTORE:
	    printf ("%6d: \"RESTORE\" Line: %d\n",	i, st->value.lineNumber) ;		break ;

	  case TK_SYM_PROC:
	  case TK_SYM_FUNC:
	    printf ("%6d: %04X: %12s: %d",		i, x, st->name, st->value.linePtr) ;
	    if (st->value.linePtr != -1)
	      printf (" -> %d", programLines [st->value.lineNumber].lineNumber) ;
	    printf (", args: %d", st->args) ;
	    printf ("\n") ;
	    break ;

	  case TK_SYM_REM1:
	    printf ("%6d: %04X: \"%s\"\n",		i, x, st->name) ;			break ;
	  case TK_SYM_REM2:
	    printf ("%6d: %04X: \"%s\"\n",		i, x, st->name) ;			break ;
	}
      }
    }
  }
}
#endif


/*
 * setupSymbolTable:
 *	Initialise a few bits and bobs to enable our dynamically sized
 *	symbol table.
 *********************************************************************************
 */

void setupSymbolTable (void)
{
  numSymbols            = 0 ;
  symbolTable           = NULL ;

  symbolTableStackPtr = 0 ;
  symbolTableStackMax = 0 ;
  symbolTableStack    = NULL ;
}


/*
 * deleteSymbols:
 *	Delete all symbols from the symbol table - used on the NEW command
 *********************************************************************************
 */

void deleteSymbols (void)
{
  struct symbolTableStruct *st ;
  int i, j, x ;

  for (i = 0 ; i < numSymbols ; ++i)
  {
    st = &symbolTable [i] ;

    if (st->name != NULL)
      free (st->name) ;

    if ((st->type == TK_SYM_CONST_STR) || (st->type == TK_SYM_VAR_STR))
      if (st->value.stringVal != NULL)
	free (st->value.stringVal) ;

    if (st->type == TK_SYM_VAR_NUM_ARR)
    {
      if (st->dimensions != NULL)
	free (st->dimensions) ;
      if (st->value.realArray != NULL)
	free (st->value.realArray) ;
    }

    if (st->type == TK_SYM_VAR_STR_ARR)
    {
      if (st->dimensions != NULL)
      {
	x = st->dimensions [0] ;
	for (j = 1 ; j < st->arrayDims ; ++j)
	  x *= st->dimensions [j] ;

	for (j = 0 ; j < x ; ++j)
	  if (st->value.stringArray [j] != NULL)
	    free (st->value.stringArray [j]) ;

	if (st->value.stringArray != NULL)
	  free (st->value.stringArray) ;

	free (st->dimensions) ;
      }
    }
  }

  if (symbolTable != NULL)
    free (symbolTable) ;

  if (symbolTableStack != NULL)
    free (symbolTableStack) ;

  setupSymbolTable () ;
}


/*
 * clearVariables:
 *	Clear or zero all variables in the symbol table -
 *	usually in preparation for RUN or the CLEAR command.
 *********************************************************************************
 */

void clearVariables (void)
{
  struct symbolTableStruct *st ;
  int i, j, x ;

  for (i = 0 ; i < numSymbols ; ++i)
  {
    st = &symbolTable [i] ;
    st->writeCount = 0 ;

    switch (st->type)
    {
      case TK_SYM_VAR_NUM:
	st->value.realVal = 0.0 ;
	break ;

      case TK_SYM_VAR_STR:
	if (st->value.stringVal != NULL)
	{
	  free (st->value.stringVal) ;
	  st->value.stringVal = NULL ;
	}
	break ;

      case TK_SYM_PROC:
      case TK_SYM_FUNC:
	st->value.lineNumber = -1 ;
	break ;

      case TK_SYM_VAR_NUM_ARR:
	if (st->dimensions != NULL)
	{
	  if (st->value.realArray != NULL)
	    free (st->value.realArray) ;

	  free (st->dimensions) ;
	  st->value.realArray = NULL ;
	  st->dimensions      = NULL ;
	  st->arrayDims       = 0 ;
	}
	break ;

      case TK_SYM_VAR_STR_ARR:
	if (st->dimensions != NULL)
	{
	  x = st->dimensions [0] ;
	  for (j = 1 ; j < st->arrayDims ; ++j)
	    x *= st->dimensions [j] ;

	  for (j = 0 ; j < x ; ++j)
	    if (st->value.stringArray [j] != NULL)
	      free (st->value.stringArray [j]) ;

	  if (st->value.stringArray != NULL)
	    free (st->value.stringArray) ;

	  free (st->dimensions) ;
	  st->value.stringArray = NULL ;
	  st->dimensions        = NULL ;
	  st->arrayDims         = 0 ;
	}

      case TK_SYM_VAR_MAP:
	if (st->dimensions != NULL)
	{
	  x = st->dimensions [0] ;
	  for (j = 1 ; j < st->arrayDims ; ++j)
	    x *= st->dimensions [j] ;

	  for (j = 0 ; j < x ; ++j)
	    if (st->value.map [j] != NULL)
	      free (st->value.map [j]) ;

	  if (st->value.map != NULL)
	    free (st->value.map) ;

	  free (st->dimensions) ;
	  st->value.map         = NULL ;
	  st->dimensions        = NULL ;
	  st->arrayDims         = 0 ;
	}
    }
  }
}


/*
 * pushSymbol: popSymbol:
 *	Save a copy of a symbol onto the symbolTableStack and restore
 *	it back again.
 *	This is used in procedures/functions to save the arguments
 *	and to create local variables.
 *********************************************************************************
 */

int pushSymbol (uint16_t index)
{
  struct symbolTableStruct *orig, *copy ;

  if (symbolTableStackPtr == symbolTableStackMax)
  {
    symbolTableStackMax += MAX_SYMBOLS ;
    if ((symbolTableStack = realloc (symbolTableStack, sizeof (struct symbolTableStruct) * symbolTableStackMax)) == NULL)
      bomb ("Out of memory reallocating symbolStack space", TRUE) ;
  }

  orig = &symbolTable [index] ;
  copy = &symbolTableStack [symbolTableStackPtr] ;

  memcpy (copy, orig, sizeof (struct symbolTableStruct)) ;

  orig->writeCount = 0 ;

  switch (orig->type)
  {
    case TK_SYM_VAR_NUM:	orig->value.realVal   =  0.0 ; break ;
    case TK_SYM_VAR_STR:	orig->value.stringVal = NULL ; break ;

    case TK_SYM_VAR_NUM_ARR:
      orig->value.realArray  = NULL ;
      orig->dimensions       = NULL ;
      orig->arrayDims        = 0 ;
      break ;

    case TK_SYM_VAR_STR_ARR:
      orig->value.stringArray = NULL ;
      orig->dimensions        = NULL ;
      orig->arrayDims         = 0 ;
      break ;

  }

  ++symbolTableStackPtr ;
  return TRUE ;
}

int popSymbol (void)
{
  struct symbolTableStruct *copy ;
  struct symbolTableStruct *orig ;

  if (symbolTableStackPtr == 0)
    bomb ("popSymbol: Stack underflow", FALSE) ;

  copy = &symbolTableStack [--symbolTableStackPtr] ;
  orig = &symbolTable      [copy->index] ;

  if (orig->type == TK_SYM_VAR_STR)
    if (orig->value.stringVal != NULL)
      free (orig->value.stringVal) ;

  memcpy (orig, copy, sizeof (struct symbolTableStruct)) ;

  return TRUE ;
}


/*
 * newSymbol:
 *	Create a new symbol in the symbol table. (Checking that it doesn't
 *	already exist first)
 *********************************************************************************
 */

static struct symbolTableStruct *_newSymbol (char *name)
{
  struct symbolTableStruct *st ;

  if (numSymbols == MAX_SYMBOLS)
    bomb ("Maximum symbol table size exceeded", FALSE) ;

  symbolTable = realloc (symbolTable, sizeof (struct symbolTableStruct) * (numSymbols + 1)) ;

  if (symbolTable == NULL)
    bomb ("Out of memory error (Allocating a new symbol)", FALSE) ;

  st = &symbolTable [numSymbols] ;

  if (name == NULL)
    st->name = NULL ;
  else
  {
    if ((st->name = malloc (strlen (name) + 1)) == NULL)
      bomb ("Out of memory error (Allocating new symbol name)", FALSE) ;
    strcpy (st->name, name) ;
  }
  st->index = numSymbols++ ;
  return st ;
}

static int newSymbol (char *name, int type)
{
  register int i ;
  struct symbolTableStruct *st ;

// See if it already exists 

  for (i = 0 ; i < numSymbols ; ++i)
  {
    st = &symbolTable [i] ;
    if (st->type == type)
      if (strcmp (st->name, name) == 0)
	return i ;
  }

  st = _newSymbol (name) ;
  st->type = type ;

// Initialise variables

  switch (type)
  {
    case TK_SYM_VAR_NUM:
      st->value.realVal = 0.0 ;
      break ;

    case TK_SYM_VAR_NUM_ARR:
      st->value.realArray = NULL ;
      st->dimensions      = NULL ;
      st->arrayDims       = 0 ;
      break ;

    case TK_SYM_VAR_STR:
      st->value.stringVal = NULL ;
      break ;

    case TK_SYM_VAR_STR_ARR:
      st->value.stringArray = NULL ;
      st->dimensions        = NULL ;
      st->arrayDims         = 0 ;
      break ;

    case TK_SYM_VAR_MAP:
      st->value.map  = NULL ;
      st->dimensions = NULL ;
      st->arrayDims  = 0 ;	// Also flags un-dimensioned array
      break ;
  }

  st->writeCount = 0 ;

  return numSymbols - 1 ;
}


/*
 * newSymbolVariableXXX:
 *	Create a new variable in the symbol table
 *********************************************************************************
 */

uint16_t newSymbolVariableReal (char *name)
  { return TK_SYM_VAR_NUM     | newSymbol (name, TK_SYM_VAR_NUM) ; }

uint16_t newSymbolVariableRealArray (char *name)
  { return TK_SYM_VAR_NUM_ARR | newSymbol (name, TK_SYM_VAR_NUM_ARR) ; }

uint16_t newSymbolVariableString (char *name)
  { return TK_SYM_VAR_STR      | newSymbol (name, TK_SYM_VAR_STR) ; }

uint16_t newSymbolVariableStringArray (char *name)
  { return TK_SYM_VAR_STR_ARR  | newSymbol (name, TK_SYM_VAR_STR_ARR) ; }

uint16_t newSymbolVariableMap (char *name)
  { return TK_SYM_VAR_MAP  | newSymbol (name, TK_SYM_VAR_MAP) ; }


/*
 * newSymbolConstantReal:
 *	Create an store a new constant real number the symbol table
 *********************************************************************************
 */

uint16_t newSymbolConstantReal (char *original, double value)
{
  int symbol ;

  if (strlen (original) == 0)
    return -1 ;

  symbol = newSymbol (original, TK_SYM_CONST_NUM) ;
  symbolTable [symbol].value.realVal = value ;

  return TK_SYM_CONST_NUM | symbol ;
}

/*
 * newSymbolConstantString:
 *	Create an store a new constant string the symbol table
 *********************************************************************************
 */

uint16_t newSymbolConstantString (char *data)
{
  int symbol ;

  symbol = newSymbol (data, TK_SYM_CONST_STR) ;
  symbolTable [symbol].value.stringVal = NULL ;

  return TK_SYM_CONST_STR | symbol ;
}

/*
 * newSymbolRem1: newSymbolRem2:
 *	Create and store a new remark in the symbol table
 *********************************************************************************
 */

uint16_t newSymbolRem1 (char *data)
{
  int symbol ;
  symbol = newSymbol (data, TK_SYM_REM1) ;
  return TK_SYM_REM1 | symbol ;
}

uint16_t newSymbolRem2 (char *data)
{
  int symbol ;
  symbol = newSymbol (data, TK_SYM_REM2) ;
  return TK_SYM_REM2 | symbol ;
}


/*
 * newSymbolGotoSubRest:
 *	Create and store a new linenumber for a GOTO, GOSUB or RESTORE
 *********************************************************************************
 */

uint16_t newSymbolGotoSubRest (int lineNumber, uint16_t token)
{
  struct symbolTableStruct *st ;
  int    i ;

// See if we already have a reference to this line number

  for (i = 0 ; i < numSymbols ; ++i)
  {
    st = &symbolTable [i] ;
    if (st->type == token) 
      if (st->value.lineNumber == lineNumber)
	return token | i ;
  }

  st = _newSymbol (NULL) ;
  st->type             = token ;
  st->value.lineNumber = lineNumber ;
  return token | (numSymbols - 1) ;
}

/*
 * newSymbolProcFn:
 *	Create and store a new symbol for a user-defined procedure or function
 *********************************************************************************
 */

uint16_t newSymbolProcFn (char *name, uint16_t token)
{
  struct symbolTableStruct *st ;
  int    i ;

// See if we already have a reference to this name

  for (i = 0 ; i < numSymbols ; ++i)
  {
    st = &symbolTable [i] ;
    if (st->type == token) 
      if (strcmp (st->name, name) == 0)
	return token | i ;
  }

  st = _newSymbol (name) ;
  st->type             = token ;
  st->value.lineNumber = -1 ;
  return token | (numSymbols - 1) ;
}


/*
 * storeStringVar:
 *	Store a string variable
 *********************************************************************************
 */

int storeStringVar (uint16_t sIndex, char *string)
{
  struct symbolTableStruct *st = &symbolTable [sIndex] ;

  if (st->value.stringVal != NULL)
    free (st->value.stringVal) ;

  if ((st->value.stringVal = malloc (strlen (string) + 1)) == NULL)
    return FALSE ;

  strcpy (st->value.stringVal, string) ;
  ++st->writeCount ;

  return TRUE ;
}


/*
 * storeStringVarNA:
 *	Store a string variable - No Allocation of memory
 *********************************************************************************
 */

int storeStringVarNA (uint16_t sIndex, char *string)
{
  struct symbolTableStruct *st = &symbolTable [sIndex] ;

  if (st->value.stringVal != NULL)
    free (st->value.stringVal) ;

  st->value.stringVal = string ;
  ++st->writeCount ;

  return TRUE ;
}


/*
 * storeRealVar:
 *	Store a real number
 *********************************************************************************
 */

void storeRealVar (uint16_t sIndex, double realNum)
{
  struct symbolTableStruct *st = &symbolTable [sIndex] ;

  st->value.realVal = realNum ;
  ++st->writeCount ;
}

/*
 * getRealVar:
 *	Extract a real number out of the symbol table, checking to make sure
 *	it's been assigned...
 *********************************************************************************
 */

int getRealVar (uint16_t sIndex, double *realNum)
{
  struct symbolTableStruct *st = &symbolTable [sIndex] ;

  if (st->writeCount == 0)
    return syntaxError ("Unassigned variable") ;

  *realNum = st->value.realVal ;
  return TRUE ;
}


/*
 * storeArray:
 *	Store (copy) an array
 *********************************************************************************
 */

void storeArray (uint16_t sIndex, uint16_t oSymbol)
{
  struct symbolTableStruct *to   = &symbolTable [sIndex] ;
  struct symbolTableStruct *from = &symbolTable [oSymbol & ~TK_SYM_MASK] ;

  to->value      = from->value ;
  to->type       = from->type  ;
  to->arrayDims  = from->arrayDims  ;
  to->dimensions = from->dimensions  ;
  to->writeCount = from->writeCount ;
}

/*
 * getRealVar:
 *	Extract a real number out of the symbol table, checking to make sure
 *	it's been assigned...
 *********************************************************************************
 */

int getArray (uint16_t sIndex, uint16_t nSymbol)
{
  struct symbolTableStruct *to   = &symbolTable [nSymbol & ~TK_SYM_MASK] ;
  struct symbolTableStruct *from = &symbolTable [sIndex] ;

  to->value      = from->value ;
  to->type       = from->type  ;
  to->arrayDims  = from->arrayDims  ;
  to->dimensions = from->dimensions  ;
  to->writeCount = from->writeCount ;

  return TRUE ;
}
