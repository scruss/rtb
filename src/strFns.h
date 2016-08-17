/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 *********************************************************************************
 * strFns.h:
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

// String Functions

extern int doChrD   (void *ptr) ;
extern int doLeftD  (void *ptr) ;
extern int doRightD (void *ptr) ;
extern int doMidD   (void *ptr) ;
extern int doSpaceD (void *ptr) ;
extern int doStrD   (void *ptr) ;
extern int doStrD   (void *ptr) ;

extern int doDateD  (void *ptr) ;
extern int doTimeD  (void *ptr) ;

// Mixed

extern int doLen    (void *ptr) ;
extern int doVal    (void *ptr) ;
extern int doAsc    (void *ptr) ;

