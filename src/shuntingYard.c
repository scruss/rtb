/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 *********************************************************************************
 * shuntingYard.c:
 *********************************************************************************
 *
 * Dijkstras shunting yard.
 *
 * Rules: (Taken from Wikipedia)
 *
 * While there are tokens to be read:
 *
 * 1.	Read a token.
 *		If the token is a number, then add it to the output queue.
 *
 * 2.		If the token is a function token, then push it onto the stack.
 *
 * 3.		If the token is a function argument separator (e.g., a comma):
 *			Until the token at the top of the stack is a left
 *			parenthesis, pop operators off the stack onto
 *			the output queue. If no left parentheses are
 *			encountered, either the separator was misplaced
 *			or parentheses were mismatched.
 *
 * 4.	If the token is an operator, o1, then:
 *		while there is an operator token, o2, at the top of the stack, and
 *			either o1 is left-associative and its precedence
 *			is less than or equal to that of o2, or o1 is
 *			right-associative and its precedence is less
 *			than that of o2, pop o2 off the stack, onto the output queue;
 *
 *		push o1 onto the stack.
 *
 * 5.	If the token is a left parenthesis, then push it onto the stack.
 *
 * 6.	If the token is a right parenthesis:
 *		Until the token at the top of the stack is a left
 *		parenthesis, pop operators off the stack onto the output queue.
 *		Pop the left parenthesis from the stack and discard
 *		If the token at the top of the stack is a function token,
 *			pop it onto the output queue.
 *		If the stack runs out without finding a left parenthesis,
 *			then there are mismatched parentheses.
 *
 * 7.	When there are no more tokens to read:
 *		While there are still operator tokens in the stack:
 *			If the operator token on the top of the stack
 *			is a left parenthesis, then there are mismatched
 *			parentheses. Pop the operator onto the output queue.
 *
 * Exit.
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

#undef	SY_DEBUG2

#include <SDL/SDL.h>
#include <stdio.h>
//#include <stdlib.h>
#include <stdint.h>
#include <string.h>
//#include <ctype.h>
//#include <errno.h>

//#include <unistd.h>

#include "rtb.h"
#include "bomb.h"
#include "bool.h"


#include "keywords.h"
#include "symbolTable.h"

#include "shuntingYard.h"

#if defined (DEBUG_SHUNTER) || defined (SY_DEBUG1) || defined (SY_DEBUG2) || defined (SY_DEBUG3)
#include "screenKeyboard.h"
#endif

// Globals

uint16_t oStack [O_STACK_SIZE] ;	// Output stack
int      oStackPtr = 0 ;

uint16_t fStack [O_STACK_SIZE] ;	// Function stack
int      fStackPtr = 0 ;

#ifdef	DEBUG_SHUNTER

void dumpShuntingYard (char *prefix)
{
  int i, j ;
  uint16_t symbol, index ;

  if ((oStackPtr + fStackPtr) == 0)
  {
    printf ("%s: dumpShuntingYard: Both Stacks empty\n", prefix) ;
    return ;
  }
  else
    printf ("%s: dumpShuntingYard: oStackPtr: %d, fStackPtr %d\n", prefix, oStackPtr, fStackPtr) ;

  for (i = 0 ; i < oStackPtr ; ++i)
  {
    symbol = oStack [i] &  TK_SYM_MASK;
    index  = oStack [i] & ~TK_SYM_MASK ;

    printf ("%s: %3d: %04X:%4d  ", prefix, i, symbol, index) ; 

    if (symbol == 0)
      printf ("Keyword: %s\n", keywords [index].keyword) ;
    else
      printf ("%4d: \"%s\": ", index, symbolNames [symbol >> 12]) ;

    switch (symbol)
    {
      case 0:
	break ;

      case TK_SYM_CONST_NUM:
      case TK_SYM_VAR_NUM:
	printf ("%f\n", symbolTable [index].value.realVal) ;
	break ;

      case TK_SYM_CONST_STR:
      case TK_SYM_VAR_STR:
	printf ("\"%s\"\n", symbolTable [index].value.stringVal) ;
	break ;

      case TK_SYM_VAR_NUM_ARR:
	printf ("    %d (", symbolTable [index].arrayDims) ;
	for (j = 0 ; j < symbolTable [index].arrayDims ; ++j)
	{
	  printf ("%4d", symbolTable [index].dimensions [j]) ;
	  if (j != (symbolTable [index].arrayDims - 1))
	    printf (",") ;
	}
	printf (")\n") ;
	break ;


      case TK_SYM_PROC:
      case TK_SYM_FUNC:
	printf ("\"%s\"\n", symbolTable [index].name) ;
	break ;

      case TK_SYM_EOL:
      case 0xFF:
	printf ("\n") ;

      default:
	printf ("[%04X:%04X]\n", symbol, index) ;
	break ;
    }
  }
}
#endif


/*
 * queue:
 *	Insert a token into the output queue
 *********************************************************************************
 */

static int queue (uint16_t token)
{
  if (oStackPtr == O_STACK_SIZE)
    return syntaxError ("Expression too complex (queue: stack overflow)") ;

  oStack [oStackPtr++] = token ;
  return TRUE ;
}

/*
 * shunt:
 *	Push a token into the shunting yard (function stack)
 *********************************************************************************
 */

static int shunt (uint16_t token)
{
  if (fStackPtr == O_STACK_SIZE)
    return syntaxError ("Expression too complex (shunt: stack overflow)") ;

  fStack [fStackPtr++] = token ;
  return TRUE ;
}


/*
 * shuntToQueue:
 *	Move the top of the function stack to the output queue
 *********************************************************************************
 */

static int shuntToQueue (void)
{
  int ok ;

  ok =  queue (fStack [--fStackPtr]) ;

  return ok ;
}


/*
 * shuntRule3:
 *	Implment Rule 3 above.
 *********************************************************************************
 */

static int shuntRule3 ()
{
  uint16_t tos ;

  if (fStackPtr == 0)	// Stack underflow
    return syntaxError ("Invalid expression [possibly missing (]") ;

  tos   = fStack [fStackPtr - 1] ;
  while (tos != TK_BRA)
  {
    if (!shuntToQueue ())
      return FALSE ;

    if (fStackPtr == 0)
      return syntaxError ("Invalid expression [possibly missing (]") ;

    tos = fStack [fStackPtr - 1] ;
  }
  return TRUE ;
}


/*
 * shuntRule4:
 *	Implment Rule 4 above.
 *********************************************************************************
 */

static int shuntRule4 (uint16_t o1)
{
  uint16_t o2 ;
  uint16_t o2flags, o1flags ;
  uint8_t  o1prec, o2prec ;

  if (fStackPtr == 0)
    return  shunt (o1) ;

  if ((o1 & TK_SYM_MASK) == TK_SYM_FUNC)		// User-Def FN
  {
    o1flags = 0 ;
    o1prec  = 9 ;
  }
  else							// Built-in
  {
    o1flags = keywords [o1].flags ;
    o1prec  = keywords [o1].precidence ;
  }

  o2 = fStack [fStackPtr - 1] ;

  if ((o2 & TK_SYM_MASK) == TK_SYM_FUNC)		// User-Def FN
  {
    o2flags = 0 ;
    o2prec  = 9 ;
  }
  else							// Built-in
  {
    o2flags = keywords [o2].flags ;
    o2prec  = keywords [o2].precidence ;
  }

  while ((o2flags & (KYF_ARITH | KYF_REL)) != 0)	// TOS is an Operator token
  {
    if ( (((o1flags & KYF_RA) == 0) && (o1prec <= o2prec))	||
	 (((o1flags & KYF_RA) != 0) && (o1prec <  o2prec)) ) 
    {
      if (!shuntToQueue ())
	return FALSE ;

      if (fStackPtr == 0)
	break ;

      o2 = fStack [fStackPtr - 1] ;
      if ((o2 & TK_SYM_MASK) == TK_SYM_FUNC)		// User-Def FN
      {
	o2flags = 0 ;
	o2prec  = 9 ;
      }
      else						// Built-in
      {
	o2flags = keywords [o2].flags ;
	o2prec  = keywords [o2].precidence ;
      }
    }
    else
      break ;
  }

  return shunt (o1) ;
}



/*
 * shuntingYard:
 *	Implment the rules
 *********************************************************************************
 */

int shuntingYard (uint16_t *p, int *len)
{
  uint16_t token, symbol ;
  uint16_t flags, tos ;
  int l = 0 ;
  int fStackPtrStart = fStackPtr ;

#ifdef	SY_DEBUG2
  printf ("shuntingYard: *p is 0x%04X (%d)\n", *p, *p & ~TK_SYM_MASK) ;
#endif

// Mark the output queue

  queue (TK_SYM_EOL) ;

// Shunt until we get a token that's either EOL, or
//	not a function or symbol, or pseudo variable

  for (;; ++p)
  {
    token  = *p ;
    l     += 1 ;
    symbol = token & TK_SYM_MASK ;

#ifdef	SY_DEBUG2
  printf ("shuntingYard: token: 0x%04X", token) ;
  if (symbol == 0)
    printf (": \"%s\"", keywords [token].keyword) ;
  printf ("\n") ;
#endif

// Simple check to make sure this token is shuntable

    if (symbol == 0)			// Built-in token
    {
      flags = keywords [token].flags ;

      if ((flags & KYF_SHUNTER) == 0)	// Can't shunt it -> game over.
	break ;

// Rule 1: If the symbol is a pseudo variable, add it to the output queue

      if ((flags & KYF_PV) != 0)
      {
	if (!queue (token))
	  return FALSE ;
	continue ;
      }
    }

// Rule 1: If the symbol is a constant or variable, add it to the output queue

    else if ((symbol == TK_SYM_CONST_NUM)   || (symbol == TK_SYM_CONST_STR)	||
	     (symbol == TK_SYM_VAR_NUM)     || (symbol == TK_SYM_VAR_STR) )
    {
#ifdef	SY_DEBUG2
  printf ("shuntingYard: Rule 1: Queueing number: 0x%04X\n", token) ;
#endif
      if (!queue (token))
	return FALSE ;
      continue ;
    }

// Rule 1.5: If token is an array or map then add it to the stack
//	Sort of pretending it's a function here

    if ((symbol == TK_SYM_VAR_NUM_ARR) || (symbol == TK_SYM_VAR_STR_ARR) || (symbol == TK_SYM_VAR_MAP) )
    {
#ifdef	SY_DEBUG2
  printf ("shuntingYard: Rule 1.5: Shunting ARRAY: 0x%04X\n", token) ;
#endif
      if (!shunt (token))
	return FALSE ;
      continue ;
    }
    
// Rule 2: If token is a function, add it to the stack

    if (symbol == TK_SYM_FUNC)
    {
#ifdef	SY_DEBUG2
  printf ("shuntingYard: Rule 2: Shunting user-def FN: 0x%04X\n", token) ;
#endif
      if (!shunt (token))
	return FALSE ;
      continue ;
    }

// Any other symbol, then we break out ...

    if (symbol != 0)
      break ;

// Built-in keyword

    flags = keywords [token].flags ;

    if ((flags & KYF_FUNC) != 0)
    {
#ifdef	SY_DEBUG2
  printf ("shuntingYard: Rule 2: Shunting internal FN: %d\n", token) ;
#endif
      if (!shunt (token))
	return FALSE ;
      continue ;
    }

// Rule 3: If token is a function argument separtor (ie. comma)

    if (token == TK_COMMA)
    {
#ifdef	SY_DEBUG2
  printf ("shuntingYard: Rule 3: Comma\n") ;
#endif
      if (!shuntRule3 ())
	return FALSE ;
      continue ;
    }

// Rule 4: If token is an operator, add it to the stack, but check precidence rules
//	(arithmetic or relational)

    if ((flags & (KYF_ARITH | KYF_REL)) != 0)
    {
#ifdef	SY_DEBUG2
  printf ("shuntingYard: Rule 4: Operator\n") ;
#endif
      if (!shuntRule4 (token))
	return FALSE ;
      continue ;
    }

// Rule 5: If token is left bracket, add it onto the stack

    if (token == TK_BRA)
    {
#ifdef	SY_DEBUG2
  printf ("shuntingYard: Rule 5: Shunt (\n") ;
#endif
      if (!shunt (token))
	return FALSE ;
      continue ;
    }

// Rule 6: If token is a right bracket:
//	Pop stack to Queue until TOS is left bracket, then if the new TOS is a function,
//	send that to the output Queue too.

    if (token == TK_KET)
    {
#ifdef	SY_DEBUG2
  printf ("shuntingYard: Rule 6: )\n") ;
#endif
      if (fStackPtr == 0)
	return syntaxError ("missing (") ;

      while ((tos = fStack [fStackPtr - 1]) != TK_BRA)
      {
	if (!shuntToQueue ())
	  return FALSE ;

	if (fStackPtr == 0)
	  return syntaxError ("Unmatched brackets") ;
      }

//	Drop the (

      if (--fStackPtr == 0)
	continue ;

//	If TOS is a function or Array, move it to the Queue

      tos = fStack [fStackPtr - 1] ;
      symbol = tos & TK_SYM_MASK ;
      if ( (symbol == TK_SYM_FUNC) || (symbol == TK_SYM_VAR_NUM_ARR) || (symbol == TK_SYM_VAR_STR_ARR) )
      {
	if (!shuntToQueue ())
	  return FALSE ;
	continue ;
      }
      else if ((keywords [tos].flags & KYF_FUNC) != 0)	// Built-in function
      {
	if (!shuntToQueue ())
	  return FALSE ;
	continue ;
      }

      continue ;
    }

// We exit if it's something we've not covered here

    break ;
  }

// Finally Rule 7: Copy the function stack (yard) to the output Queue

  while (fStackPtr != fStackPtrStart)
  {
    tos = fStack [fStackPtr -1] ;
    if (tos == TK_BRA)
      return syntaxError ("Missing )") ;
    else
      if (!shuntToQueue ())
	return FALSE ;
  }

// Here, 'p' is pointing to the last token we scanned and 'l' is the number of tokens
//	we scanned, including the one we bowed out on, so we'll subtract one from 'l'
//	so the next thing to use it will have their 'p' pointing to the token we stopped at.

  *len = l - 1 ;

#ifdef	SY_DEBUG2
  printf ("shuntingYard: END: len: %d\n", l) ;
  dumpShuntingYard ("SY") ;
  printf ("\n") ;
#endif

  return TRUE ;
}
