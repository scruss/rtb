/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * array.c:
 *	Calculate array indicies
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "rtb.h"
#include "bool.h"

#include "symbolTable.h"
#include "keywords.h"

#include "shuntingYard.h"
#include "rpnEval.h"
#include "hash.h"

#include "array.h"


/*
 * getArrayIndex:
 *	Evaluate the index of an array
 *********************************************************************************
 */

int getArrayIndex (uint16_t *p, int *len, int *index)
{
  struct    symbolTableStruct *st ;
  uint16_t *q  ;
  uint16_t  saved ;
  int       dims, braCount ;
  int       idx, thisLen, start ;

  st = &symbolTable [*p & ~TK_SYM_MASK] ;

  if (st->arrayDims == 0)
    return syntaxError ("Undimensioned array") ;

// We need to trick the shunter to stop at the last TK_KET, so we'll
//	scan for it and replace the token after it with an EOL

  q = ++p ;
  if (*q++ != TK_BRA)
    return syntaxError ("Array: ( expected") ;

  braCount = 1 ;
  for (;;++q)
  {
    if (endOfLine (q))
      return syntaxError ("Array: Mismatched ()'s") ;

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
//	p still points to the (

  if (!shuntingYard (p, &thisLen))
    return FALSE ;

  *q = saved ;	// Undo our little hack

  start = stackOrderPtr ;

  if (!rpnEval ())
    return FALSE ;

  dims = stackOrderPtr - start ;	// Dims inside the ()'s

  if (st->arrayDims != dims)
    return syntaxError ("Array: Incorrect number of dimensions: Got %d, expected %d", dims, st->arrayDims) ;

  if (!popArrayIndex (st, &idx))
    return FALSE ;

  *index = idx ;
  *len   = thisLen + 1 ;
  return TRUE ;
}


/*
 * popArrayIndex:
 *	Pop all the indicies off the stack and compute the true index into the array.
 *	Uses the standard array index calculation:
 *		index = n1 + (n2 * d1) + (n3 * d2 * d1) + (n4 * d3 * d2 * d1) ...
 *	However here, the 'n' value can be either a number or the result of the
 *	map hash function to get the index.
 *********************************************************************************
 */

int popArrayIndex (struct symbolTableStruct *st, int *index)
{
  int idx,i, j, x ;
  int dims = st->arrayDims ;
  char *s ;

  if (st->arrayDims == 0)
    return syntaxError ("Undimensioned array") ;

  if (stackOrderPtr < 1)
    return syntaxError ("Array: Bad dimension") ;

  /**/ if (stackOrder [stackOrderPtr - 1] == EVAL_STACK_NUM)
    idx = (int)rint (popN ()) ;
  else if (stackOrder [stackOrderPtr - 1] == EVAL_STACK_STR)
  {
    s   = popS () ;
    idx = hash (s, st->dimensions [0]) ;
    free (s) ;
  }
  else
    return syntaxError ("Invalid array index type") ;

  if ((idx >= st->dimensions [0]) || (idx < 0))
    return syntaxError ("Out of bounds") ;

  for (i = 0 ; i < (dims - 1) ; ++i)
  {
    if (stackOrderPtr < 1)
      return syntaxError ("Array: Bad dimension") ;

    /**/ if (stackOrder [stackOrderPtr - 1] == EVAL_STACK_NUM)
      x = (int)rint (popN ()) ;
    else if (stackOrder [stackOrderPtr - 1] == EVAL_STACK_STR)
    {
      s = popS () ;
      x = hash (s, st->dimensions [i + 1]) ;
      free (s) ;
    }
    else
      return syntaxError ("Invalid array index type") ;

    if ((x >= st->dimensions [i + 1]) || (x < 0))
      return syntaxError ("Out of bounds") ;

    for (j = 0 ; j <= i ; ++j)
      x *= st->dimensions [j] ;

    idx += x ;
  }

  *index = idx ;
  return TRUE ;
}
