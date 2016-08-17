/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * assign.c:
 *	Handle a variable assignment
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
//#include "bomb.h"
#include "bool.h"

#include "keywords.h"
#include "lines.h"
#include "symbolTable.h"
#include "shuntingYard.h"
#include "rpnEval.h"
#include "array.h"

#include "commands.h"
#include "goto.h"
#include "run.h"
#include "assign.h"

/*
 * doAssignment:
 *	Handle a variable assignment - scanning the line from the given location.
 *	On entry, p should be pointing to the symbol representing the variable
 *********************************************************************************
 */

int doAssignment (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  struct    symbolTableStruct *st ;
  double    resultNum ;
  char     *resultStr ;
  uint16_t  variable, type, index ;
  int       len ;

  resultStr = NULL ;
  resultNum = 0.0 ;

  variable = *p++ ;
  type     = variable &  TK_SYM_MASK ;
  index    = variable & ~TK_SYM_MASK ;

  if (*p++ != TK_EQUALS)
    return syntaxError ("Equals expected") ;

  if (!shuntingYard (p, &len))
    return FALSE ;

  p += len ;
  if (!endOfLine (p))
    return syntaxError ("Assign: Extra data at end of line") ;

// Work out the type of assignment

  if (type == 0)		// Pseudo Variable
  {
    if (!rpnEvalNum (&resultNum))
      return FALSE ;
    keywords [index].function (&resultNum) ;
    return TRUE ;
  }

  st = &symbolTable [index] ;

  if (st->type == TK_SYM_VAR_NUM)
  {
    if (!rpnEvalNum (&resultNum))
      return FALSE ;
    storeRealVar (index, resultNum) ;
  }
  else
  {
    if (!rpnEvalStr (&resultStr))
      return FALSE ;
    if (!storeStringVar (index, resultStr))
    {
      free (resultStr) ;
      return syntaxError ("Assign: Out of memory") ;
    }
    free (resultStr) ;
  }

  return TRUE ;
}

int doArrayAssignment (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  struct    symbolTableStruct *st ;
  double    resultNum ;
  char     *resultStr ;
  int       arrayIndex, len ;

  st = &symbolTable [*p & ~TK_SYM_MASK] ;

  if (!getArrayIndex (p, &len, &arrayIndex))
    return FALSE ;
  
  p += len ;
  if (*p++ != TK_EQUALS)
    return syntaxError ("Equals expected") ;

  if (!shuntingYard (p, &len))
    return FALSE ;

  p += len ;
  if (!endOfLine (p))
    return syntaxError ("Array Assign: Extra data at end of line") ;

  if (st->type == TK_SYM_VAR_NUM_ARR)		// Number
  {
    if (!rpnEvalNum (&resultNum))
      return FALSE ;
    st->value.realArray [arrayIndex] = resultNum ;
  }
  else						// String
  {
    if (!rpnEvalStr (&resultStr))
      return FALSE ;
    if (st->value.stringArray [arrayIndex] != NULL)
      free (st->value.stringArray [arrayIndex]) ;
    st->value.stringArray [arrayIndex] = resultStr ;
  }

  return TRUE ;
}


/*
 * doLet:
 *	Handle the LET keyword
 *********************************************************************************
 */

int doLet (void *ptr)
{
  uint16_t *p = (uint16_t *)ptr ;
  int ok ;

  switch (*p & TK_SYM_MASK)
  {
    case TK_SYM_VAR_NUM:
    case TK_SYM_VAR_STR:
      ok = doAssignment (p) ;
      break ;

    case TK_SYM_VAR_NUM_ARR:
    case TK_SYM_VAR_STR_ARR:
      ok = doArrayAssignment (p) ;
      break ;

    default:
      return syntaxError ("LET: Invalid assignment") ;
  }

  ++linePtr ;
  return ok ;
}
