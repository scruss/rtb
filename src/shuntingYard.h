/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 *********************************************************************************
 * shuntingYard.h:
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

#define	DEBUG_SHUNTER	1

#define	O_STACK_SIZE	(8192*1024)

// Globals

extern uint16_t oStack [O_STACK_SIZE] ;	// Output stack
extern int      fStackPtr ;
extern int      oStackPtr ;

// Functions

extern void dumpShuntingYard	(char *prefix) ;
//extern void dumpQueue    (char *prefix) ;
extern int  shuntingYard	(uint16_t *p, int *len) ;
