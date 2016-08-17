/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 *********************************************************************************
 * fileFns.h:
 *	Functions to manipulate disk files.
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


#define	MAX_OPEN_FILES	8

extern int doOpenIn  (void *ptr) ;
extern int doOpenUp  (void *ptr) ;
extern int doOpenOut (void *ptr) ;
extern int doClose   (void *ptr) ;
extern int doEOF     (void *ptr) ;
extern int doRewind  (void *ptr) ;
extern int doFfwd    (void *ptr) ;
extern int doSeek    (void *ptr) ;

extern int doInputX  (void *ptr) ;
extern int doPrintX  (void *ptr) ;

void fileCloseAll    (void) ;
void setupFiles      (void) ;
