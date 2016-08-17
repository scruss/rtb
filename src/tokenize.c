/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * tokenize2.c:
 *	The second state of input line handling.
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
//#include <errno.h>

#include "tokenize1.h"
#include "tokenize2.h"
#include "tokenize.h"

//#include "bomb.h"

#include "screenKeyboard.h"

#include "bool.h"
#include "lines.h"
#include "keywords.h"
#include "symbolTable.h"

#include "rtb.h"

int tokenize (char *input, uint16_t *tokenizedLine, int *length)
{
  if (tokenize1 (input, tokenizedLine, length))
    if (tokenize2 (tokenizedLine))
      return TRUE ;

  return FALSE ;
}
