/*
 * returnToBasic:
 *	A new implementation of BASIC
 *	Copyright (c) 2012 Gordon Henderson
 ***********************************************************************
 * keywords.h:
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

struct keywordStruct
{
  const char		*keyword ;
  const uint16_t	token ;
  const uint8_t		flags ;
  const uint8_t		args ;
  const uint8_t		precidence ;
  int			(*function)(void *) ;
} ;

struct commandStruct
{
  const char	*command ;
  void		(*function)(int argc, char *argv []) ;
} ;

extern struct keywordStruct keywords       [] ;
extern struct keywordStruct keywordAliases [] ;
extern struct commandStruct commands       [] ;

extern char *symbolNames [16] ;

// Keyword type flags

#define	KYF_0		0x000
#define	KYF_FUNC	0x001
#define	KYF_PROC	0x002
#define	KYF_PV		0x004
#define	KYF_RO		0x008
#define	KYF_ARITH	0x010
#define	KYF_REL		0x020
#define	KYF_SHUNTER	0x040
// Right Assoc.
#define	KYF_RA		0x080

// Can execute in immediate mode
//#define	KYF_IMM		0x100

// 16-bit Tokens:
//	The values are unsigned 16-bit numbers. They are coded as:
//	0x0000 -> 0x0FFF - built-in keywords
//	0x1000 -> 0xFxxx - index into the global symbol table of variables,
//		constants, GOTO/GOSUB and defined procecure and function targets.

// Prefix tokens. We have 14 at present which will need 4 bits to encode,
//	leaving 4 bits in the token byte and 8 bits in the next byte,
//	which allows for a maximum of 2^12 = 4096 symbols.
//	However we need to keep the top-bit set to indicate an extended
//	token, so we lose one bit, leaving 2048 possible symbols before
//	we need to move to a 3-byte format.

#define	TK_SYM_KEYWORD		0x0000
#define	TK_SYM_CONST_NUM	0x1000
#define	TK_SYM_CONST_STR	0x2000
#define	TK_SYM_VAR_NUM		0x3000
#define	TK_SYM_VAR_STR		0x4000
#define	TK_SYM_VAR_NUM_ARR	0x5000
#define	TK_SYM_VAR_STR_ARR	0x6000
#define	TK_SYM_VAR_MAP		0x7000	// Spare
#define	TK_SYM_PROC		0x8000
#define	TK_SYM_FUNC		0x9000
#define	TK_SYM_GOTO		0xA000
#define	TK_SYM_GOSUB		0xB000
#define	TK_SYM_RESTORE		0xC000
#define	TK_SYM_REM1		0xD000
#define	TK_SYM_REM2		0xE000
#define	TK_SYM_EOL		0xFFFF

#define	TK_SYM_MASK		0xF000

// Keyword token values

#define TK_CHRd	0
#define TK_LEFTd	1
#define TK_MIDd	2
#define TK_RIGHTd	3
#define TK_SPACEd	4
#define TK_STRd	5
#define TK_DATEd	6
#define TK_TIMEd	7
#define TK_LET	8
#define TK_DIM	9
#define TK_DATA	10
#define TK_READ	11
#define TK_RESTORE	12
#define TK_RND	13
#define TK_SEED	14
#define TK_PI	15
#define TK_PI2	16
#define TK_TIME	17
#define TK_SIN	18
#define TK_ASIN	19
#define TK_COS	20
#define TK_ACOS	21
#define TK_TAN	22
#define TK_ATAN	23
#define TK_ABS	24
#define TK_EXP	25
#define TK_LOG	26
#define TK_SQRT	27
#define TK_SGN	28
#define TK_INT	29
#define TK_HASH	30
#define TK_MAX	31
#define TK_MIN	32
#define TK_VAL	33
#define TK_SWAP	34
#define TK_DEBUG	35
#define TK_ASC	36
#define TK_LEN	37
#define TK_GET	38
#define TK_GETd	39
#define TK_INKEY	40
#define TK_INPUT	41
#define TK_PRINT	42
#define TK_NUMFORMAT	43
#define TK_FOR	44
#define TK_TO	45
#define TK_STEP	46
#define TK_NEXT	47
#define TK_CYCLE	48
#define TK_REPEAT	49
#define TK_UNTIL	50
#define TK_DO	51
#define TK_WHILE	52
#define TK_BREAK	53
#define TK_CONTINUE	54
#define TK_DEF	55
#define TK_FN	56
#define TK_PROC	57
#define TK_ENDPROC	58
#define TK_LOCAL	59
#define TK_GOTO	60
#define TK_GOSUB	61
#define TK_RETURN	62
#define TK_SWITCH	63
#define TK_CASE	64
#define TK_ENDCASE	65
#define TK_DEFAULT	66
#define TK_ENDSWITCH	67
#define TK_WAIT	68
#define TK_STOP	69
#define TK_END	70
#define TK_IF	71
#define TK_THEN	72
#define TK_ELSE	73
#define TK_ENDIF	74
#define TK_TRUE	75
#define TK_FALSE	76
#define TK_TWIDTH	77
#define TK_THEIGHT	78
#define TK_GWIDTH	79
#define TK_GHEIGHT	80
#define TK_BLACK	81
#define TK_NAVY	82
#define TK_GREEN	83
#define TK_TEAL	84
#define TK_MAROON	85
#define TK_PURPLE	86
#define TK_OLIVE	87
#define TK_SILVER	88
#define TK_GREY	89
#define TK_BLUE	90
#define TK_LIME	91
#define TK_AQUA	92
#define TK_RED	93
#define TK_FUCHSIA	94
#define TK_YELLOW	95
#define TK_WHITE	96
#define TK_K_UP	97
#define TK_K_DOWN	98
#define TK_K_LEFT	99
#define TK_K_RIGHT	100
#define TK_K_INX	101
#define TK_K_DEL	102
#define TK_K_HOME	103
#define TK_K_END	104
#define TK_K_PGUP	105
#define TK_K_PGDN	106
#define TK_K_F1	107
#define TK_K_F2	108
#define TK_K_F3	109
#define TK_K_F4	110
#define TK_K_F5	111
#define TK_K_F6	112
#define TK_K_F7	113
#define TK_K_F8	114
#define TK_K_F9	115
#define TK_K_F10	116
#define TK_K_F11	117
#define TK_K_F12	118
#define TK_CLS	119
#define TK_TCOLOUR	120
#define TK_BCOLOUR	121
#define TK_HTAB	122
#define TK_VTAB	123
#define TK_HVTAB	124
#define TK_GR	125
#define TK_HGR	126
#define TK_SAVESCR	127
#define TK_COLOUR	128
#define TK_RGBCOLOUR	129
#define TK_PLOT	130
#define TK_HLINE	131
#define TK_VLINE	132
#define TK_LINE	133
#define TK_LINETO	134
#define TK_RECT	135
#define TK_TRIANGLE	136
#define TK_CIRCLE	137
#define TK_ELLIPSE	138
#define TK_ORIGIN	139
#define TK_UPDATE	140
#define TK_PENDOWN	141
#define TK_PENUP	142
#define TK_MOVE	143
#define TK_MOVETO	144
#define TK_LEFT	145
#define TK_RIGHT	146
#define TK_TANGLE	147
#define TK_LOADSPRITE	148
#define TK_SAVESPRITE	149
#define TK_PLOTSPRITE	150
#define TK_DELSPRITE	151
#define TK_POLYSTART	152
#define TK_POLYPLOT	153
#define TK_POLYEND	154
#define TK_OPENIN	155
#define TK_OPENOUT	156
#define TK_OPENUP	157
#define TK_CLOSE	158
#define TK_EOF	159
#define TK_REWIND	160
#define TK_FFWD	161
#define TK_SEEK	162
#define TK_INPUTx	163
#define TK_PRINTx	164
#define TK_SOPEN	165
#define TK_SCLOSE	166
#define TK_SGET	167
#define TK_SGETd	168
#define TK_SPUT	169
#define TK_SPUTd	170
#define TK_SREADY	171
#define TK_DRC_OPEN	172
#define TK_DRC_CLOSE	173
#define TK_DRC_PINMODE	174
#define TK_DRC_PULLUPDN	175
#define TK_DRC_PWNWRITE	176
#define TK_DRC_DWRITE	177
#define TK_DRC_DREAD	178
#define TK_DRC_AREAD	179
#define TK_PINMODE	180
#define TK_PULLUPDN	181
#define TK_DWRITE	182
#define TK_DREAD	183
#define TK_NESOPEN	184
#define TK_NESREAD	185
#define TK_NESUP	186
#define TK_NESDOWN	187
#define TK_NESLEFT	188
#define TK_NESRIGHT	189
#define TK_NESELECT	190
#define TK_NESSTART	191
#define TK_NESA	192
#define TK_NESV	193
#define TK_DEG	194
#define TK_RAD	195
#define TK_CLOCK	196
#define TK_REM1	197
#define TK_REM2	198
#define TK_POW	199
#define TK_TIMES	200
#define TK_DIV	201
#define TK_MOD	202
#define TK_IDIV	203
#define TK_PLUS	204
#define TK_MINUS	205
#define TK_BOR	206
#define TK_BAND	207
#define TK_BXOR	208
#define TK_UNMINUS	209
#define TK_NOT	210
#define TK_LTHAN	211
#define TK_LEQUAL	212
#define TK_GTHAN	213
#define TK_GEQUAL	214
#define TK_EQUALS	215
#define TK_NOT_EQUALS	216
#define TK_AND	217
#define TK_OR	218
#define TK_BRA	219
#define TK_KET	220
#define TK_COMMA	221
#define TK_SEMICO	222
#define TK_LAST_TOKEN	223
