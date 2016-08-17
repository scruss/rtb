/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * cycle.h:
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

#define	CYCLE_STACK_CHUNK	32
#define	FOR_STACK_CHUNK		32

extern int  initCycle  (void) ;
extern void clearCycle (void) ;

extern int doCycle    (void *ptr) ;
extern int doRepeat   (void *ptr) ;
extern int doDo       (void *ptr) ;
extern int doWhile    (void *ptr) ;
extern int doUntil    (void *ptr) ;
extern int doContinue (void *ptr) ;
extern int doBreak    (void *ptr) ;
extern int doFor      (void *ptr) ;
extern int doNext     (void *ptr) ;