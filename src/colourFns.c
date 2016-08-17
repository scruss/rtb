/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 *********************************************************************************
 * colourFns.c:
 *	Pesudo variables to reperesent our standard colours
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "rtb.h"
//#include "bomb.h"

#include "bool.h"

//#include "shuntingYard.h"

#include "rpnEval.h"
#include "numFns.h"

#include "colourFns.h"


/*
 * doBlack:
 *********************************************************************************
 */

static int doColour (void *ptr, int colour)
{
  if (ptr != NULL)
    return FALSE ;
  pushN ((double)colour) ;
  return TRUE ;
}

int doBlack   (void *ptr) { return doColour (ptr,  0) ; }
int doNavy    (void *ptr) { return doColour (ptr,  1) ; }
int doGreen   (void *ptr) { return doColour (ptr,  2) ; }
int doTeal    (void *ptr) { return doColour (ptr,  3) ; }
int doMaroon  (void *ptr) { return doColour (ptr,  4) ; }
int doPurple  (void *ptr) { return doColour (ptr,  5) ; }
int doOlive   (void *ptr) { return doColour (ptr,  6) ; }
int doSilver  (void *ptr) { return doColour (ptr,  7) ; }
int doGrey    (void *ptr) { return doColour (ptr,  8) ; }
int doBlue    (void *ptr) { return doColour (ptr,  9) ; }
int doLime    (void *ptr) { return doColour (ptr, 10) ; }
int doAqua    (void *ptr) { return doColour (ptr, 11) ; }
int doRed     (void *ptr) { return doColour (ptr, 12) ; }
int doFuchsia (void *ptr) { return doColour (ptr, 13) ; }
int doYellow  (void *ptr) { return doColour (ptr, 14) ; }
int doWhite   (void *ptr) { return doColour (ptr, 15) ; }
