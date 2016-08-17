/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * symbolTable.h:
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

#define	DEBUG_SYMBOL_TABLE

/*
 * Symbol prefixes. Top 4 bits define the symbol type,
 *	bottom 12 bits define the offset in the symbol table.
 * This limits us to 2^12, 4096 symbols. It'll do for now.
 *********************************************************************************
 */

#define	MAX_SYMBOLS	4096

struct symbolTableStruct
{
  char *name ;
  union
  {
    double  realVal ;
    double *realArray ;
    char   *stringVal ;
    char   **stringArray ;
    char   **map ;
    int    lineNumber ;
    int    linePtr ;
  } value ;
  uint16_t type ;
  uint16_t index ;
  uint8_t  args ;	// PROC/FUNC No. arguments
  uint8_t  arrayDims ;	// Number of dimensions
  int     *dimensions ;
  int      writeCount ;
} ;

extern int symbolTableStackPtr ;

extern int numSymbols ;
extern struct symbolTableStruct *symbolTable ;

#ifdef	DEBUG_SYMBOL_TABLE
extern void dumpSymbolTable (char *prefix) ;
#endif

extern void setupSymbolTable	(void) ;
extern void deleteSymbols	(void) ;
extern void clearVariables	(void) ;

extern int pushSymbol		(uint16_t index) ;
extern int popSymbol		(void) ;

//extern int  newSymbol		(char *name, int type, int lineNum) ;

extern uint16_t newSymbolVariableReal		(char *name) ;
extern uint16_t newSymbolVariableString		(char *name) ;

extern uint16_t newSymbolVariableRealArray	(char *name) ;
extern uint16_t newSymbolVariableStringArray	(char *name) ;

extern uint16_t newSymbolVariableMap            (char *name) ;

extern uint16_t newSymbolConstantReal	(char *original, double value) ;
extern uint16_t newSymbolConstantString	(char *data) ;
extern uint16_t newSymbolRem1           (char *data) ;
extern uint16_t newSymbolRem2           (char *data) ;

extern uint16_t newSymbolGotoSubRest	(int lineNumber, uint16_t token) ;

extern uint16_t newSymbolProcFn         (char *name, uint16_t token) ;

extern int  storeStringVar   (uint16_t sIndex, char *string) ;
extern int  storeStringVarNA (uint16_t sIndex, char *string) ;
extern void storeRealVar     (uint16_t sIndex, double  realNum) ;
extern void storeArray       (uint16_t sIndex, uint16_t oSymbol) ;

extern int  getRealVar       (uint16_t sIndex, double *realNum) ;
extern int  getArray         (uint16_t sIndex, uint16_t nSymbol) ;

