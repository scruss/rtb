/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * procFn.h:
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

#define	PROC_STACK_CHUNK	64
#define	FUNC_STACK_CHUNK	64

extern void clearProcs (void) ;
extern int  initProcs  (void) ;

extern int doLocal    (void *ptr) ;

extern int doDef      (void *ptr) ;
extern int doProc     (void *ptr) ;
extern int doEndProc  (void *ptr) ;

extern int doFunc     (uint16_t symbol) ;
extern int doEndFunc  (void *ptr) ;
