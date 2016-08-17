/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * procedures.h:
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


extern int doEnd     (void *ptr) ;
extern int doStop    (void *ptr) ;

extern int doDeg     (void *ptr) ;
extern int doRad     (void *ptr) ;
extern int doClock   (void *ptr) ;

extern int doWait    (void *ptr) ;
extern int doSwap    (void *ptr) ;

extern int doPrint     (void *ptr) ;
extern int doNumFormat (void *ptr) ;

extern int doRead    (void *ptr) ;
extern int doData    (void *ptr) ;
extern int doRestore (void *ptr) ;

extern int doDim     (void *ptr) ;

extern int doIf      (void *ptr) ;
extern int doElse    (void *ptr) ;
extern int doEndif   (void *ptr) ;

extern int doDebug   (void *ptr) ;
