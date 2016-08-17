/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * commands.h:
 *	Convenient place to locate all the command function prototypes
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

extern void doRunCommand	(int argc, char *argv []) ;
extern void doContCommand	(int argc, char *argv []) ;

extern void doClearCommand	(int argc, char *argv []) ;
extern void doNewCommand	(int argc, char *argv []) ;

extern void doTronCommand	(int argc, char *argv []) ;
extern void doTroffCommand	(int argc, char *argv []) ;

extern void doListCommand	(int argc, char *argv []) ;
extern void doEditCommand	(int argc, char *argv []) ;

extern void doSaveCommand	(int argc, char *argv []) ;
extern void doSaveNNCommand	(int argc, char *argv []) ;
extern void doLoadCommand	(int argc, char *argv []) ;

extern void doDirCommand	(int argc, char *argv []) ;
extern void doCdCommand		(int argc, char *argv []) ;
extern void doPwdCommand	(int argc, char *argv []) ;

extern void doVersionCommand    (int argc, char *argv []) ;
extern void doDebugCommand	(int argc, char *argv []) ;
extern void doExitCommand	(int argc, char *argv []) ;
