/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 *********************************************************************************
 * textFns.h:
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

extern int doCls     (void *ptr) ;
extern int doTwidth  (void *ptr) ;
extern int doTheight (void *ptr) ;
extern int doTcolour (void *ptr) ;
extern int doBcolour (void *ptr) ;
extern int doHtab    (void *ptr) ;
extern int doVtab    (void *ptr) ;
extern int doHVtab   (void *ptr) ;

extern int doGet     (void *ptr) ;
extern int doGetD    (void *ptr) ;
extern int doInkey   (void *ptr) ;
extern int doInput   (void *ptr) ;
