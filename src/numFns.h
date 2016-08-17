/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 *********************************************************************************
 * numFns.h:
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

// Basic arithmetic

extern int doPlus    (void *ptr) ;
extern int doMinus   (void *ptr) ;
extern int doUnMinus (void *ptr) ;
extern int doTimes   (void *ptr) ;
extern int doDiv     (void *ptr) ;
extern int doIdiv    (void *ptr) ;
extern int doMod     (void *ptr) ;
extern int doPow     (void *ptr) ;

// Bitwise operators

extern int doBor     (void *ptr) ;
extern int doBand    (void *ptr) ;
extern int doBxor    (void *ptr) ;


// Other numeric functions

extern int doSin     (void *ptr) ;
extern int doCos     (void *ptr) ;
extern int doTan     (void *ptr) ;
extern int doAsin    (void *ptr) ;
extern int doAcos    (void *ptr) ;
extern int doAtan    (void *ptr) ;
extern int doAbs     (void *ptr) ;
extern int doExp     (void *ptr) ;
extern int doLog     (void *ptr) ;
extern int doSqrt    (void *ptr) ;
extern int doSgn     (void *ptr) ;
extern int doInt     (void *ptr) ;
extern int doMax     (void *ptr) ;
extern int doMin     (void *ptr) ;
extern int doRnd     (void *ptr) ;
extern int doSeed    (void *ptr) ;
extern int doPI      (void *ptr) ;
extern int doPI2     (void *ptr) ;
extern int doTrue    (void *ptr) ;
extern int doFalse   (void *ptr) ;
extern int doTime    (void *ptr) ;

extern int doHash    (void *ptr) ;

extern void setupRND (void) ;
