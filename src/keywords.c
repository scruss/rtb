/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * keywords.c:
 *	Define lists of BASC keywords
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

//#include "rpnEval.h"

#include "commands.h"
#include "renumber.h"
#include "load.h"

#include "procedures.h"
#include "numFns.h"
#include "relFns.h"
#include "strFns.h"
#include "colourFns.h"
#include "keyFns.h"
#include "textFns.h"
#include "fileFns.h"
#include "serialFns.h"
#include "drcFns.h"
#include "piFns.h"
#include "nesFns.h"
#include "graphicFns.h"
#include "spriteFns.h"

#include "symbolTable.h"
#include "keywords.h"

#include "run.h"
#include "assign.h"
#include "goto.h"
#include "switch.h"
#include "procFn.h"
#include "cycle.h"

// RTB Keywords.
//	Note: Due to the way they're scanned at program entry time, you
//	need to make sure that similar keywords are listed longest first.
//	So CONTINUE needs to be before CONT for example.

// Rules for the Flags and Args column:
//	If it's a simple procedure or instruction - e.g.
//		CLS, PENDOWN, then Flags -> KYF_PROC and Args -> 0
//	in which case the runner will check for the right stuff.
//
//	If it's a procedure that takes arguments - e.g.
//		WAIT(n), MOVE(n), then Flags -> KYF_PROC and Args -> n
//	in which case the runner will check for the right stuff.
//
//	If it's a procedure that wants to do it's own parameter checking - e.g.
//		PRINT, INPUT, then Flags -> KYF_0
//	in which case the runner will call it with the right parameters to enable it
//	to parse its own arguments.

struct keywordStruct keywords [] =
{

//  Keyword		Token		Flags					Args	Precidence

  { "CHR$",		TK_CHRd,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doChrD		},
  { "LEFT$",		TK_LEFTd,	KYF_FUNC | KYF_SHUNTER,			2,	9,	&doLeftD	},
  { "MID$",		TK_MIDd,	KYF_FUNC | KYF_SHUNTER,			3,	9,	&doMidD		},
  { "RIGHT$",		TK_RIGHTd,	KYF_FUNC | KYF_SHUNTER,			2,	9,	&doRightD	},
  { "SPACE$",		TK_SPACEd,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doSpaceD	},
  { "STR$",		TK_STRd,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doStrD		},
  { "DATE$",		TK_DATEd,	KYF_PV | KYF_RO | KYF_SHUNTER,		0,	0,	&doDateD	},
  { "TIME$",		TK_TIMEd,	KYF_PV | KYF_RO | KYF_SHUNTER,		0,	0,	&doTimeD	},

  { "LET",		TK_LET,		KYF_0,					0,	0,	&doLet		},
  { "DIM",		TK_DIM,		KYF_0,					0,	0,	&doDim		},
  { "DATA",		TK_DATA,	KYF_0,					0,	0,	&doData		},
  { "READ",		TK_READ,	KYF_0,					0,	0,	&doRead		},
  { "RESTORE",		TK_RESTORE,	KYF_0,					0,	0,	NULL		}, // Sym

  { "RND",		TK_RND,		KYF_FUNC | KYF_SHUNTER,			1,	9,	&doRnd		},
  { "SEED",		TK_SEED,	KYF_PV   | KYF_SHUNTER,			0,	0,	&doSeed		},

  { "PI",		TK_PI,		KYF_PV   | KYF_SHUNTER | KYF_RO,	0,	0,	&doPI		},
  { "PI2",		TK_PI2,		KYF_PV   | KYF_SHUNTER | KYF_RO,	0,	0,	&doPI2		},
  { "TIME",		TK_TIME,	KYF_PV   | KYF_SHUNTER | KYF_RO,	0,	0,	&doTime		},

  { "SIN",		TK_SIN,		KYF_FUNC | KYF_SHUNTER,			1,	9,	&doSin		},
  { "ASIN",		TK_ASIN,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doAsin		},
  { "COS",		TK_COS,		KYF_FUNC | KYF_SHUNTER,			1,	9,	&doCos		},
  { "ACOS",		TK_ACOS,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doAcos		},
  { "TAN",		TK_TAN,		KYF_FUNC | KYF_SHUNTER,			1,	9,	&doTan		},
  { "ATAN",		TK_ATAN,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doAtan		},
  { "ABS",		TK_ABS,		KYF_FUNC | KYF_SHUNTER,			1,	9,	&doAbs		},
  { "EXP",		TK_EXP,		KYF_FUNC | KYF_SHUNTER,			1,	9,	&doExp		},
  { "LOG",		TK_LOG,		KYF_FUNC | KYF_SHUNTER,			1,	9,	&doLog		},
  { "SQRT",		TK_SQRT,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doSqrt		},
  { "SGN",		TK_SGN,		KYF_FUNC | KYF_SHUNTER,			1,	9,	&doSgn		},
  { "INT",		TK_INT,		KYF_FUNC | KYF_SHUNTER,			1,	9,	&doInt		},
  { "HASH",		TK_HASH,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doHash		},

  { "MAX",		TK_MAX,		KYF_FUNC | KYF_SHUNTER,			2,	9,	&doMax		},
  { "MIN",		TK_MIN,		KYF_FUNC | KYF_SHUNTER,			2,	9,	&doMin		},	
  { "VAL",		TK_VAL,		KYF_FUNC | KYF_SHUNTER,			1,	9,	&doVal		},
  { "SWAP",		TK_SWAP,	KYF_0,					0,	0,	&doSwap		},

  { "DEBUG",		TK_DEBUG,	KYF_PROC,				1,	0,	&doDebug	},

  { "ASC",		TK_ASC,		KYF_FUNC | KYF_SHUNTER,			1,	9,	&doAsc		},
  { "LEN",		TK_LEN,		KYF_FUNC | KYF_SHUNTER,			1,	9,	&doLen		},

  { "GET",		TK_GET,		KYF_PV | KYF_RO | KYF_SHUNTER,		0,	0,	&doGet		},
  { "GET$",		TK_GETd,	KYF_PV | KYF_RO | KYF_SHUNTER,		0,	0,	&doGetD		},
  { "INKEY",		TK_INKEY,	KYF_PV | KYF_RO | KYF_SHUNTER,		0,	0,	&doInkey	},
  { "INPUT",		TK_INPUT,	KYF_0,					0,	0,	&doInput	},
  { "PRINT",		TK_PRINT,	KYF_0,					0,	0,	&doPrint	},
  { "NUMFORMAT",	TK_NUMFORMAT,	KYF_PROC,				2,	0,	&doNumFormat	},

  { "FOR",		TK_FOR,		KYF_0,					0,	0,	&doFor		},
  { "TO",		TK_TO,		KYF_0,					0,	0,	NULL		},
  { "STEP",		TK_STEP,	KYF_0,					0,	0,	NULL		},
  { "NEXT",		TK_NEXT,	KYF_0,					0,	0,	&doNext		},

  { "CYCLE",		TK_CYCLE,	KYF_0,					0,	0,	&doCycle	},
  { "REPEAT",		TK_REPEAT,	KYF_0,					0,	0,	&doRepeat	},
  { "UNTIL",		TK_UNTIL,	KYF_0,					0,	0,	&doUntil	},
  { "DO",		TK_DO,		KYF_0,					0,	0,	&doDo		}, // Sums it up
  { "WHILE",		TK_WHILE,	KYF_0,					0,	0,	&doWhile	},
  { "BREAK",		TK_BREAK,	KYF_0,					0,	0,	&doBreak	},
  { "CONTINUE",		TK_CONTINUE,	KYF_0,					0,	0,	&doContinue	},

  { "DEF",		TK_DEF,		KYF_0,					0,	0,	&doDef		},
  { "FN",		TK_FN,		KYF_0,					0,	0,	NULL		}, // Sym
  { "PROC",		TK_PROC,	KYF_0,					0,	0,	NULL		}, // Sym
  { "ENDPROC",		TK_ENDPROC,	KYF_0,					0,	0,	&doEndProc	},
  { "LOCAL",		TK_LOCAL,	KYF_0,					0,	0,	&doLocal	},

  { "GOTO",		TK_GOTO,	KYF_0,					0,	0,	NULL		}, // Sym
  { "GOSUB",		TK_GOSUB,	KYF_0,					0,	0,	NULL		}, // Sym
  { "RETURN",		TK_RETURN,	KYF_0,					0,	0,	&doReturn	},

  { "SWITCH",		TK_SWITCH,	KYF_PROC,				1,	0,	&doSwitch	},
  { "CASE",		TK_CASE,	KYF_0,					0,	0,	&doCase		},
  { "ENDCASE",		TK_ENDCASE,	KYF_PROC,				0,	0,	&doEndCase	},
  { "DEFAULT",		TK_DEFAULT,	KYF_PROC,				0,	0,	&doDefault	},
  { "ENDSWITCH",	TK_ENDSWITCH,	KYF_PROC,				0,	0,	&doEndSwitch	},

  { "WAIT",		TK_WAIT,	KYF_PROC,				1,	0,	&doWait		},
  { "STOP",		TK_STOP,	KYF_PROC,				0,	0,	&doStop		},
  { "END",		TK_END,		KYF_PROC,				0,	0,	&doEnd		},

  { "IF",		TK_IF,		KYF_0,					0,	0,	&doIf		},
  { "THEN",		TK_THEN,	KYF_0,					0,	0,	NULL		},
  { "ELSE",		TK_ELSE,	KYF_0,					0,	0,	&doElse		},
  { "ENDIF",		TK_ENDIF,	KYF_0,					0,	0,	&doEndif	},

  { "TRUE",		TK_TRUE,	KYF_PV | KYF_RO | KYF_SHUNTER,		0,	0,	&doTrue		},
  { "FALSE",		TK_FALSE,	KYF_PV | KYF_RO | KYF_SHUNTER,		0,	0,	&doFalse	},

  { "TWIDTH",		TK_TWIDTH,	KYF_PV | KYF_RO | KYF_SHUNTER,		0,	0,	&doTwidth	},
  { "THEIGHT",		TK_THEIGHT,	KYF_PV | KYF_RO | KYF_SHUNTER,		0,	0,	&doTheight	},
  { "GWIDTH",		TK_GWIDTH,	KYF_PV | KYF_RO | KYF_SHUNTER,		0,	0,	&doGwidth	},
  { "GHEIGHT",		TK_GHEIGHT,	KYF_PV | KYF_RO | KYF_SHUNTER,		0,	0,	&doGheight	},

// Standard Colours

  { "Black",		TK_BLACK,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doBlack	},
  { "Navy",		TK_NAVY,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doNavy		},
  { "Green",		TK_GREEN,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doGreen	},
  { "Teal",		TK_TEAL,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doTeal		},
  { "Maroon",		TK_MAROON,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doMaroon	},
  { "Purple",		TK_PURPLE,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doPurple	},
  { "Olive",		TK_OLIVE,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doOlive	},
  { "Silver",		TK_SILVER,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doSilver	},
  { "Grey",		TK_GREY,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doGrey		},
  { "Blue",		TK_BLUE,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doBlue		},
  { "Lime",		TK_LIME,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doLime		},
  { "Aqua",		TK_AQUA,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doAqua		},
  { "Red",		TK_RED,		KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doRed		},
  { "Pink",		TK_FUCHSIA,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doFuchsia	},
  { "Yellow",		TK_YELLOW,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doYellow	},
  { "White",		TK_WHITE,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doWhite	},

// Special Keys - more for convenience than anything else

  { "KeyUp",		TK_K_UP,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKUp		},
  { "KeyDown",		TK_K_DOWN,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKDown	},
  { "KeyLeft",		TK_K_LEFT,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKLeft	},
  { "KeyRight",		TK_K_RIGHT,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKRight	},
  { "KeyIns",		TK_K_INX,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKIns		},
  { "KeyDel",		TK_K_DEL,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKDel		},
  { "KeyHome",		TK_K_HOME,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKHome	},
  { "KeyEnd",		TK_K_END,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKEnd		},
  { "KeyPgUp",		TK_K_PGUP,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKPgUp	},
  { "KeyPgDn",		TK_K_PGDN,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKPgDn	},
  { "KeyF1",		TK_K_F1,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKF1		},
  { "KeyF2",		TK_K_F2,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKF2		},
  { "KeyF3",		TK_K_F3,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKF3		},
  { "KeyF4",		TK_K_F4,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKF4		},
  { "KeyF5",		TK_K_F5,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKF5		},
  { "KeyF6",		TK_K_F6,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKF6		},
  { "KeyF7",		TK_K_F7,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKF7		},
  { "KeyF8",		TK_K_F8,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKF8		},
  { "KeyF9",		TK_K_F9,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKF9		},
  { "KeyF10",		TK_K_F10,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKF10		},
  { "KeyF11",		TK_K_F11,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKF11		},
  { "KeyF12",		TK_K_F12,	KYF_PV | KYF_SHUNTER | KYF_RO,		0,	0,	&doKF12		},

// Text

  { "CLS",		TK_CLS,		KYF_PROC,				0,	0,	&doCls		},
  { "TCOLOUR",		TK_TCOLOUR,	KYF_PV | KYF_SHUNTER,			0,	0,	&doTcolour	},
  { "BCOLOUR",		TK_BCOLOUR,	KYF_PV | KYF_SHUNTER,			0,	0,	&doBcolour	},
  { "HTAB",		TK_HTAB,	KYF_PV | KYF_SHUNTER,			0,	0,	&doHtab		},
  { "VTAB",		TK_VTAB,	KYF_PV | KYF_SHUNTER,			0,	0,	&doVtab		},
  { "HVTAB",		TK_HVTAB,	KYF_PROC,				2,	0,	&doHVtab	},

// Graphics

  { "GR",		TK_GR,		KYF_PROC,				0,	0,	&doGr		},
  { "HGR",		TK_HGR,		KYF_PROC,				0,	0,	&doHgr		},
  { "SaveScreen",	TK_SAVESCR,	KYF_PROC,				1,	0,	&doSaveScreen	},

  { "COLOUR",		TK_COLOUR,	KYF_PV | KYF_SHUNTER,			0,	0,	&doColour	},
  { "rgbCOLOUR",	TK_RGBCOLOUR,	KYF_PROC,				3,	0,	&doRgbColour	},
  { "PLOT",		TK_PLOT,	KYF_PROC,				2,	0,	&doPlot		},
  { "HLINE",		TK_HLINE,	KYF_PROC,				3,	0,	&doHline	},
  { "VLINE",		TK_VLINE,	KYF_PROC,				3,	0,	&doVline	},
  { "LINE",		TK_LINE,	KYF_PROC,				4,	0,	&doLine		},
  { "LINETO",		TK_LINETO,	KYF_PROC,				2,	0,	&doLineTo	},
  { "RECT",		TK_RECT,	KYF_PROC,				5,	0,	&doRectangle	},
  { "TRIANGLE",		TK_TRIANGLE,	KYF_PROC,				7,	0,	&doTriangle	},
  { "CIRCLE",		TK_CIRCLE,	KYF_PROC,				4,	0,	&doCircle	},
  { "ELLIPSE",		TK_ELLIPSE,	KYF_PROC,				5,	0,	&doEllipse	},
  { "ORIGIN",		TK_ORIGIN,	KYF_PROC,				2,	0,	&doOrigin	},
  { "UPDATE",		TK_UPDATE,	KYF_PROC,				0,	0,	&doUpdate	},

// Turtle Graphics

  { "PENDOWN",		TK_PENDOWN,	KYF_PROC,				0,	0,	&doPenDown	},
  { "PENUP",		TK_PENUP,	KYF_PROC,				0,	0,	&doPenUp	},
  { "MOVE",		TK_MOVE,	KYF_PROC,				1,	0,	&doMove		},
  { "MOVETO",		TK_MOVETO,	KYF_PROC,				2,	0,	&doMoveTo	},
  { "LEFT",		TK_LEFT,	KYF_PROC,				1,	0,	&doLeft		},
  { "RIGHT",		TK_RIGHT,	KYF_PROC,				1,	0,	&doRight	},
  { "TANGLE",		TK_TANGLE,	KYF_PV | KYF_SHUNTER,			0,	0,	&doTangle	},

// Sprites

  { "LoadSprite",	TK_LOADSPRITE,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doLoadSprite	},
  { "SaveSprite",	TK_SAVESPRITE,	KYF_FUNC | KYF_SHUNTER,			1,	9,	NULL	},
  { "PlotSprite",	TK_PLOTSPRITE,	KYF_PROC,				3,	0,	&doPlotSprite	},
  { "DelSprite",	TK_DELSPRITE,	KYF_PROC,				1,	0,	&doDelSprite	},

  { "PolyStart",	TK_POLYSTART,	KYF_PROC,				0,	0,	&doPolyStart	},
  { "PolyPlot",		TK_POLYPLOT,	KYF_PROC,				2,	0,	&doPolyPlot	},
  { "PolyEnd",		TK_POLYEND,	KYF_PROC,				0,	0,	&doPolyEnd	},

// File handling

  { "OPENIN",		TK_OPENIN,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doOpenIn	},
  { "OPENOUT",		TK_OPENOUT,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doOpenOut	},
  { "OPENUP",		TK_OPENUP,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doOpenUp	},
  { "CLOSE",		TK_CLOSE,	KYF_PROC,				1,	0,	&doClose	},
  { "EOF",		TK_EOF,		KYF_FUNC | KYF_SHUNTER,			1,	9,	&doEOF		},
  { "REWIND",		TK_REWIND,	KYF_PROC,				1,	0,	&doRewind	},
  { "FFWD",		TK_FFWD,	KYF_PROC,				1,	0,	&doFfwd		},
  { "SEEK",		TK_SEEK,	KYF_PROC,				2,	0,	&doSeek		},
  { "INPUT#",		TK_INPUTx,	KYF_0,					0,	0,	&doInputX	},
  { "PRINT#",		TK_PRINTx,	KYF_0,					0,	0,	&doPrintX	},

// Serial port

  { "SOPEN",		TK_SOPEN,	KYF_FUNC | KYF_SHUNTER,			2,	9,	&doSopen	},
  { "SCLOSE",		TK_SCLOSE,	KYF_PROC,				1,	0,	&doSclose	},
  { "SGET",		TK_SGET,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doSget		},
  { "SGET$",		TK_SGETd,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doSgetD	},
  { "SPUT",		TK_SPUT,	KYF_PROC,				2,	0,	&doSput		},
  { "SPUT$",		TK_SPUTd,	KYF_PROC,				2,	0,	&doSputD	},
  { "SREADY",		TK_SREADY,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doSready	},

// Drogon Remote Control

  { "DrcOpen",		TK_DRC_OPEN,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doDrcOpen	},
  { "DrcClose",		TK_DRC_CLOSE,	KYF_PROC,				1,	0,	&doDrcClose	},
  { "DrcPinMode",	TK_DRC_PINMODE,	KYF_PROC,				3,	0,	&doDrcPinMode	},
  { "DrcPullUpDn",	TK_DRC_PULLUPDN,	KYF_PROC,			3,	0,	&doDrcPullUpDn	},
  { "DrcPwmWrite",	TK_DRC_PWNWRITE,	KYF_PROC,			3,	0,	&doDrcPwmWrite	},
  { "DrcDigitalWrite",	TK_DRC_DWRITE,	KYF_PROC,				3,	0,	&doDrcDigitalWrite	},
  { "DrcDigitalRead",	TK_DRC_DREAD,	KYF_FUNC | KYF_SHUNTER,			2,	9,	&doDrcDigitalRead	},
  { "DrcAnalogRead",	TK_DRC_AREAD,	KYF_FUNC | KYF_SHUNTER,			2,	9,	&doDrcAnalogRead	},

// Raspberry Pi Functions
//	GPIO

  { "PinMode",		TK_PINMODE,	KYF_PROC,				2,	0,	&doPinMode	},
  { "PullUpDn",		TK_PULLUPDN,	KYF_PROC,				2,	0,	&doPullUpDn	},
  { "DigitalWrite",	TK_DWRITE,	KYF_PROC,				2,	0,	&doDigitalWrite	},
  { "DigitalRead",	TK_DREAD,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doDigitalRead	},

//	NES Joystick

  { "NesOpen",		TK_NESOPEN,	KYF_FUNC | KYF_SHUNTER,			2,	9,	&doNesOpen	},
  { "NesRead",		TK_NESREAD,	KYF_FUNC | KYF_SHUNTER,			2,	9,	&doNesRead	},
  { "NesUp",		TK_NESUP,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doNesUp	},
  { "NesDown",		TK_NESDOWN,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doNesDown	},
  { "NesLeft",		TK_NESLEFT,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doNesLeft	},
  { "NesRight",		TK_NESRIGHT,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doNesRight	},
  { "NesSelect",	TK_NESELECT,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doNesSelect	},
  { "NesStart",		TK_NESSTART,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doNesStart	},
  { "NesA",		TK_NESA,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doNesA		},
  { "NesB",		TK_NESV,	KYF_FUNC | KYF_SHUNTER,			1,	9,	&doNesB		},

// Misc

  { "DEG",		TK_DEG,		KYF_PROC,				0,	0,	&doDeg		},
  { "RAD",		TK_RAD,		KYF_PROC,				0,	0,	&doRad		},
  { "CLOCK",		TK_CLOCK,	KYF_PROC,				0,	0,	&doClock	},

  { "REM",		TK_REM1,	KYF_0,					0,	0,	NULL		}, // Sym
  { "//",		TK_REM2,	KYF_0,					0,	0,	NULL		}, // Sym

// Arithmetic operators

  { "^",		TK_POW,		KYF_ARITH | KYF_SHUNTER | KYF_RA,	0,	8,	&doPow		},
  { "*",		TK_TIMES,	KYF_ARITH | KYF_SHUNTER,		0,	7,	&doTimes	},
  { "/",		TK_DIV,		KYF_ARITH | KYF_SHUNTER,		0,	7,	&doDiv		},
  { "MOD",		TK_MOD,		KYF_ARITH | KYF_SHUNTER,		0,	7,	&doMod		},
  { "DIV",		TK_IDIV,	KYF_ARITH | KYF_SHUNTER,		0,	7,	&doIdiv		},
  { "+",		TK_PLUS,	KYF_ARITH | KYF_SHUNTER,		0,	6,	&doPlus		},
  { "-",		TK_MINUS,	KYF_ARITH | KYF_SHUNTER,		0,	6,	&doMinus	},

// Binary arithmetic operators

  { "|",		TK_BOR,		KYF_ARITH | KYF_SHUNTER,		0,	5,	&doBor		},
  { "&",		TK_BAND,	KYF_ARITH | KYF_SHUNTER,		0,	5,	&doBand		},
  { "XOR",		TK_BXOR,	KYF_ARITH | KYF_SHUNTER,		0,	5,	&doBxor		},

// Unary minus
//	Placed after the arithmetic subtraction as it shares the same symbol, however it's
//	detected at tokenize time.

  { "-",		TK_UNMINUS,	KYF_ARITH | KYF_SHUNTER | KYF_RA,	0,	9,	&doUnMinus	},

// Relational operators

  { "NOT",		TK_NOT,		KYF_REL | KYF_SHUNTER | KYF_RA,		0,	8,	&doNot		},
  { "<",		TK_LTHAN,	KYF_REL | KYF_SHUNTER,			0,	4,	&doLessThan	},
  { "<=",		TK_LEQUAL,	KYF_REL | KYF_SHUNTER,			0,	4,	&doLessThanEq	},
  { ">",		TK_GTHAN,	KYF_REL | KYF_SHUNTER,			0,	4,	&doGtThan	},
  { ">=",		TK_GEQUAL,	KYF_REL | KYF_SHUNTER,			0,	4,	&doGtThanEq	},
  { "=",		TK_EQUALS,	KYF_REL | KYF_SHUNTER,			0,	3,	&doEquals	},
  { "<>",		TK_NOT_EQUALS,	KYF_REL | KYF_SHUNTER,			0,	3,	doNotEquals	},
  { "AND",		TK_AND,		KYF_REL | KYF_SHUNTER,			0,	2,	&doAnd		},
  { "OR",		TK_OR,		KYF_REL | KYF_SHUNTER,			0,	2,	&doOr		},

// Others

  { "(",		TK_BRA,		KYF_SHUNTER,				0,	0,	NULL		},
  { ")",		TK_KET,		KYF_SHUNTER,				0,	0,	NULL		},
  { ",",		TK_COMMA,	KYF_SHUNTER,				0,	0,	NULL		},
  { ";",		TK_SEMICO,	KYF_0,					0,	0,	NULL		},

// Must be last

  { NULL,		TK_LAST_TOKEN,	0,					0,	0,	NULL		},
} ;


// Aliases 

struct keywordStruct keywordAliases [] =
{
  { "?",		TK_PRINT,	KYF_0,					0,	0,	NULL		},
  { "><",		TK_NOT_EQUALS,	KYF_REL  | KYF_SHUNTER,			0,	0,	NULL		},
  { "=<",		TK_LEQUAL,	KYF_REL  | KYF_SHUNTER,			0,	0,	NULL		},
  { "=>",		TK_GEQUAL,	KYF_REL  | KYF_SHUNTER,			0,	0,	NULL		},
  { "OPEN",		TK_OPENUP,	KYF_FUNC | KYF_SHUNTER,			1,	9,	NULL		},
  { "TCOLOR",		TK_TCOLOUR,	KYF_PV   | KYF_SHUNTER,			0,	0,	NULL		},
  { "BCOLOR",		TK_BCOLOUR,	KYF_PV   | KYF_SHUNTER,			0,	0,	NULL		},
  { "COLOR",		TK_COLOUR,	KYF_PV   | KYF_SHUNTER,			0,	0,	NULL		},
  { "PENCOLOUR",	TK_COLOUR,	KYF_PV   | KYF_SHUNTER,			0,	0,	NULL		},
  { "PENCOLOR",		TK_COLOUR,	KYF_PV   | KYF_SHUNTER,			0,	0,	NULL		},
  { "TGR",		TK_HGR,		KYF_PV   | KYF_SHUNTER,			0,	0,	NULL		},
  { "Cyan",		TK_AQUA,	KYF_PV   | KYF_RO,			0,	0,	NULL		},
  { "Magenta",		TK_FUCHSIA,	KYF_PV   | KYF_RO,			0,	0,	NULL		},
  { "Fuchsia",		TK_FUCHSIA,	KYF_PV   | KYF_RO,			0,	0,	NULL		},
  { "Gray",		TK_GREY,	KYF_PV   | KYF_RO,			0,	0,	NULL		},

  { NULL,		0,		0,					0,	0,	NULL		},
} ;


/*
 * Commands
 *********************************************************************************
 */

struct commandStruct commands [] =
{
  { "RUN",	&doRunCommand		},
  { "CONT",	&doContCommand		},
  { "RESUME",	&doContCommand		},
  { "LIST",	&doListCommand		},
  { "CLEAR",	&doClearCommand		},
  { "NEW",	&doNewCommand		},
  { "TRON",	&doTronCommand		},
  { "TROFF",	&doTroffCommand		},
  { "TRACE",	NULL			},
  { "UNTRACE",	NULL			},
  { "RENUMBER",	&doRenumberCommand	},
  { "SAVE",	&doSaveCommand		},
  { "SAVENN",	&doSaveNNCommand	},
  { "LOAD",	&doLoadCommand		},
  { "ED",	&doEditCommand		},
  { "DIR",	&doDirCommand		},
  { "CD",	&doCdCommand		},
  { "PWD",	&doPwdCommand		},
  { "VERSION",	&doVersionCommand	},
  { "EXIT",	&doExitCommand		},
  { ">>>",	&doDebugCommand		},

  {  NULL,	NULL			},
} ;


char *symbolNames [16] = 
{
  "TOKEN",			//  0
  "TK_SYM_CONST_NUM",		//  1
  "TK_SYM_CONST_STR",
  "TK_SYM_VAR_NUM",
  "TK_SYM_VAR_STR",
  "TK_SYM_VAR_NUM_ARR",
  "TK_SYM_VAR_STR_ARR",
  "TK_SYM_MAP",
  "TK_SYM_PROC",
  "TK_SYM_FUNC",
  "TK_SYM_GOTO",
  "TK_SYM_GOSUB",
  "TK_SYM_RESTORE",
  "TK_SYM_REM1",
  "TK_SYM_REM2",
  "TK_SYM_EOL",			// 15
} ;
