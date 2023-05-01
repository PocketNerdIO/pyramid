/*  pyramid.c - the main source code for the game
 */

/*
    Pyramid, a Patience Game for the Psion Series 3
    Version 1.0a
    Copyright (C) 1993  J Cade Roux

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 * See the file COPYING for the GNU General Public License.
 * Mail: Cade Roux
 *			c/o Dubroca
 *			Box 513
 *			Boutte, LA  70039
 *			USA
 */

#include <plib.h>
#include <wlib.h>
#include <hwif.h>

#include "utils/utils.h"

#include "global.h"			/* global definitions such as the version */
#include "pyramid.h"			/* resource ids                           */

/* Parameters of deck/game */
#define N_RANKS			13
#define N_CARDS			52
#define N_ROWS				7

/* Card positioning */
#define GFX_CARD_WID			18
#define GFX_CARD_HGT			24

#define GFX_RANK_WID			6
#define GFX_RANK_HGT			6

#define GFX_SUIT_S_WID		7
#define GFX_SUIT_S_HGT		7
#define GFX_SUIT_L_WID		13
#define GFX_SUIT_L_HGT		13

#define GFX_CARD_X(row, pos)		(112 - (row) * ((GFX_CARD_WID + 6) / 2) + (pos) * (6 + GFX_CARD_WID))
#define GFX_CARD_Y(row)				((row) * 10)

#define GFX_ARROW_WID		4
#define GFX_ARROW_HGT		4

/* Offsets of design on card */
#define GFX_RANK_TOP			2
#define GFX_RANK_LEFT		2

#define GFX_SUIT_S_TOP		2
#define GFX_SUIT_S_LEFT		9
#define GFX_SUIT_L_TOP		9
#define GFX_SUIT_L_LEFT		3

/* Offsets in graphics data bitmap */
#define ICON_X					0
#define ICON_Y					0

#define GFX_BACK_X			23
#define GFX_BACK_Y			0

#define GFX_RANK_X(rank)	(41 + (rank) * GFX_RANK_WID)
#define GFX_RANK_Y			0

#define GFX_ARROW_X			41
#define GFX_ARROW_Y			19

#define GFX_SUIT_L_X(suit)		(41 + (suit) * GFX_SUIT_L_WID)
#define GFX_SUIT_S_X(suit)		(93 + (suit) * GFX_SUIT_S_WID)
#define GFX_SUIT_Y				6

/* Offsets of cards on screen */
#define GFX_HAND_X				(GFX_ARROW_WID + 2)
#define GFX_HAND_Y				0

#define GFX_TALON_X				(GFX_HAND_X + GFX_CARD_WID + GFX_ARROW_WID + 2)
#define GFX_TALON_Y				GFX_HAND_Y

/* Two formats:
	index = suit * N_RANKS + rank
	rank = index % N_RANKS
	suit = index / N_RANKS

	Rank and Suit:		Suit: 0, 1, 2, 3: Hearts, Diamonds, Clubs, Spades resp.
							Rank: 0-12: Ace-King resp.

	Index:				0-51:		Ace-King Hearts, Ace-King Diamonds,
										Ace-King Clubs, Ace-King Spades
*/

/* The definition of a card */
typedef struct CARD_TAG {
	struct CARD_TAG *		pnext ;	/* Pointer to next card */
	BYTE						index ;	/* high bit indicates if it's been dealt */
	BYTE						fill ;
} CARD ;

/* Masks for cards */
#define DEALT_MASK					0x80			/* high bit   */
#define INDEX_MASK					0x7F			/* low 7 bits */
#define DEALT(card)					(((card).index & DEALT_MASK) != 0)
#define INDEX(pcard)					((pcard)->index & INDEX_MASK)

#define RANK(pcard)					(INDEX(pcard) % N_RANKS)
#define SUIT(pcard)					(INDEX(pcard) / N_RANKS)

LOCAL_D H_MENU_DATA		mdata[] = {	"Pyramid", 2,
												NULL } ;

LOCAL_D TEXT *				cmds[] = {	"nNew Game",
												"xExit",
												NULL } ;

GLREF_D WSERV_SPEC *		wserv_channel ;
GLREF_D TEXT *				DatCommandPtr ;
GLDEF_D TEXT **			_cmds = cmds ;
GLDEF_D H_MENU_DATA *	_mdata = mdata ;

/* Cards in tableau etc. */
LOCAL_D struct {
	CARD *		deck ;
	CARD *		pyramid[N_ROWS][N_ROWS] ;
	CARD *		hand ;
	CARD *		talon ;
	CARD *		hold ;
	INT			removables_row[N_ROWS] ;
	INT			removables_pos[N_ROWS] ;
	INT			n_removables ;
} cards ;

#define HOLDING		(cards.hold != NULL)
#define REMOVABLE(r,p) (	r >= 0 &&\
									p >= 0 &&\
									r < N_ROWS &&\
									p <= r &&\
									!(	HOLDING &&\
										LPILE == POS_PYRAMID &&\
										r == LROW - 1 &&\
										(	p == LPOS ||\
											p == LPOS - 1 ) ) &&\
									cards.pyramid[r][p] != NULL &&\
									(	r == N_ROWS - 1 ||\
										(	cards.pyramid[r + 1][p] == NULL &&\
											cards.pyramid[r + 1][p + 1] == NULL ) ) )

enum {	POS_HAND = 0,
			POS_TALON,
			POS_PYRAMID } ;

LOCAL_D struct {
	ULONG			seedl ;			/* random number seed               */
	BOOL			quit ;			/* player requested quit            */
	BOOL			new_game ;		/* player requested a new game      */

	INT			cpile ;			/* current pile                     */
	INT			curr_pos ;		/* current position on pyramid      */
	INT			lpile ;			/* pile of last play                */
	INT			lrow ;			/* last play row/pos coords         */
	INT			lpos ;

	INT			num_dealt ;		/* number of cards dealt            */
} status = { 0L, FALSE, FALSE } ;

#define ON_PYRAMID		(CPILE == POS_PYRAMID)
#define ON_TALON			(CPILE == POS_TALON)

#define CPILE	status.cpile
#define CROW	cards.removables_row[status.curr_pos]
#define CPOS	cards.removables_pos[status.curr_pos]

#define LPILE	status.lpile
#define LROW	status.lrow
#define LPOS	status.lpos

#define MATCH(pcard1,pcard2)		(RANK(pcard1) + RANK(pcard2) == 11)

LOCAL_D struct {
	UINT  win ;						/* main window id                   */
	INT   gc ;						/* permanent gc for main window     */
	UINT	ptr ;						/* pointer window id                */
	UINT	bmp ;						/* gfx bitmap id                    */
} gfx ;

/* Deal a single card */
LOCAL_C INT DealCard(VOID) {
	INT			iLp = -1 ;
	INT			count = 0 ;
	INT			index ;

	/* Deal one of the remaining slots, and count down to that undealt card */
	for ( index = Random(&status.seedl, 0, N_CARDS - status.num_dealt - 1) ;
			count <= index ;
			count += !DEALT(cards.deck[++iLp]) ) ;

	/* Mark it dealt */
	cards.deck[iLp].index |= DEALT_MASK ;
	status.num_dealt++ ;
	return(iLp) ;
}

/* Clear a card-sized rectangle at x, y */
LOCAL_C VOID ClrCard(INT x, INT y) {
	P_RECT			rect = {	{0, 0},
									{GFX_CARD_WID, GFX_CARD_HGT} } ;

	p_offrec(&rect, x, y) ;
	gClrRect(&rect, G_TRMODE_CLR) ;
}

/* Draw Rank and Suit of *pcard whose tl is at x, y */
LOCAL_C VOID DrawRankandSuit(BOOL large, CARD *pcard, INT x, INT y) {
	static P_RECT		RECT_RANK = { {0, 0}, {GFX_RANK_WID, GFX_RANK_HGT} } ;
	static P_RECT		RECT_SML = { {0, 0}, {GFX_SUIT_S_WID, GFX_SUIT_S_HGT} } ;
	static P_RECT		RECT_LRG = { {0, 0}, {GFX_SUIT_L_WID, GFX_SUIT_L_HGT} } ;
	P_RECT				rect ;
	P_POINT				pos ;

	rect = RECT_RANK ;
	pos.x = x + GFX_RANK_LEFT ;
	pos.y = y + GFX_RANK_TOP ;
	p_offrec(&rect, GFX_RANK_X(RANK(pcard)), GFX_RANK_Y) ;
	gCopyBit(&pos, gfx.bmp, &rect, G_TRMODE_SET) ;

	rect = (large ? RECT_LRG : RECT_SML) ;
	pos.x = x + (large ? GFX_SUIT_L_LEFT : GFX_SUIT_S_LEFT) ;
	pos.y = y + (large ? GFX_SUIT_L_TOP : GFX_SUIT_S_TOP) ;
	p_offrec(	&rect,
					large ? GFX_SUIT_L_X(SUIT(pcard)) : GFX_SUIT_S_X(SUIT(pcard)),
					GFX_SUIT_Y ) ;
	gCopyBit(&pos, gfx.bmp, &rect, G_TRMODE_SET) ;
}

/* Draw Card Outline at x, y */
LOCAL_C VOID DrawCardOutline(INT x, INT y) {
	P_RECT		rect = { {0, 0}, {GFX_CARD_WID, GFX_CARD_HGT} } ;

	p_offrec(&rect, x, y) ;
	gDrawBox(&rect) ;
}

/* Draw card *pcard at x, y */
LOCAL_C VOID DrawCard(BOOL large, CARD *pcard, INT x, INT y) {
	if ( pcard == NULL )
		return ;

	ClrCard(x, y) ;

	/* Draw Whole Face Up Card */
	DrawCardOutline(x, y) ;

	/* Draw Rank and Large Suit */
	DrawRankandSuit(large, pcard, x, y) ;
}

/* Draw a card back at x, y */
LOCAL_C VOID DrawBack(INT x, INT y) {
	static P_RECT		RECT_BACK = {	{GFX_BACK_X, GFX_BACK_Y},
												{GFX_BACK_X + GFX_CARD_WID, GFX_BACK_Y + GFX_CARD_HGT} } ;
	P_POINT 	pos ;

	pos.x = x ;
	pos.y = y ;
	gCopyBit(&pos, gfx.bmp, &RECT_BACK, G_TRMODE_SET) ;
}

/* Draw a Pyramid, from row srow and position spos */
LOCAL_C VOID DrawPyramid(INT srow, INT spos) {
	INT				row ;
	INT				pos ;

	srow = MIN(MAX(srow, 0), N_ROWS - 1) ;
	spos = MIN(MAX(spos, 0), srow) ;

	for ( row = srow ; row < N_ROWS ; row++ )
		for (	pos = spos ; pos - spos <= row - srow ; pos++ )
			DrawCard(	REMOVABLE(row, pos),
							cards.pyramid[row][pos],
							GFX_CARD_X(row, pos),
							GFX_CARD_Y(row) ) ;
}

/* Draw pile */
LOCAL_C VOID DrawPile(INT pile) {
	if ( pile == POS_HAND ) {
		ClrCard(GFX_HAND_X, GFX_HAND_Y) ;
		if ( cards.hand == NULL )
			DrawCardOutline(GFX_HAND_X, GFX_HAND_Y) ;
		else
			DrawBack(GFX_HAND_X, GFX_HAND_Y) ;
	}
	else if ( pile == POS_TALON ) {
		ClrCard(GFX_TALON_X, GFX_TALON_Y) ;
		if ( cards.talon == NULL )
			DrawCardOutline(GFX_TALON_X, GFX_TALON_Y) ;
		else
			DrawCard(TRUE, cards.talon, GFX_TALON_X, GFX_TALON_Y) ;
	}
}

/* Update the array of the positions of removable cards */
LOCAL_C VOID UpdateRemovables(VOID) {
	INT			row ;
	INT			pos ;

	for ( cards.n_removables = 0, pos = 0 ; pos < N_ROWS ; pos++ )
		for ( row = N_ROWS - 1 ; row >= pos ; row-- )
			if ( REMOVABLE(row, pos) ) {
				cards.removables_row[cards.n_removables] = row ;
				cards.removables_pos[cards.n_removables] = pos ;
				cards.n_removables++ ;
				DrawCard(TRUE, cards.pyramid[row][pos], GFX_CARD_X(row,pos), GFX_CARD_Y(row)) ;
				break ;
			}
}

/* Return card to pile it was taken from */
LOCAL_C VOID CancelPlay(VOID) {
	if ( LPILE == POS_PYRAMID ) {
		cards.pyramid[LROW][LPOS] = cards.hold ;
		cards.hold = NULL ;
		LPILE = POS_HAND ;
		DrawPyramid(LROW - 2, LPOS - 1) ;
		UpdateRemovables() ;
	}
	else if ( LPILE == POS_TALON ) {
		cards.hold->pnext = cards.talon ;
		cards.talon = cards.hold ;
		cards.hold = NULL ;
		DrawPile(LPILE) ;
		LPILE = POS_HAND ;
	}
}

/* Draw a card from the hand */
LOCAL_C VOID HandDraw(VOID) {
	CARD *		pcard ;

	if ( cards.hand != NULL ) {
		LPILE = POS_HAND ;
		pcard = cards.hand ;
		cards.hand = cards.hand->pnext ;
		pcard->pnext = cards.talon ;
		cards.talon = pcard ;

		DrawPile(POS_HAND) ;
		DrawPile(POS_TALON) ;
	}
}

/* Pick up card from the current position */
LOCAL_C VOID PickUpCard(VOID) {
	if ( ON_PYRAMID ) {
		if ( RANK(cards.pyramid[CROW][CPOS]) == 12 ) {
			cards.hold = NULL ;
			cards.pyramid[CROW][CPOS] = NULL ;
			LPILE = POS_HAND ;
		}
		else {
			cards.hold = cards.pyramid[CROW][CPOS] ;
			cards.pyramid[CROW][CPOS] = NULL ;
			cards.hold->pnext = NULL ;
			LPILE = CPILE ;
			LROW = CROW ;
			LPOS = CPOS ;
		}
		ClrCard(GFX_CARD_X(CROW, CPOS), GFX_CARD_Y(CROW)) ;
		DrawPyramid(CROW - 2, CPOS - 1) ;
		UpdateRemovables() ;
	}
	else if (	ON_TALON &&
					cards.talon != NULL ) {
		if ( RANK(cards.talon) == 12 ) {
			cards.hold = NULL ;
			cards.talon = cards.talon->pnext ;
			LPILE = POS_HAND ;
		}
		else {
			cards.hold = cards.talon ;
			cards.talon = cards.talon->pnext ;
			cards.hold->pnext = NULL ;
			LPILE = CPILE ;
		}
		DrawPile(POS_TALON) ;
	}
	else if (	ON_TALON &&
					cards.talon == NULL )
		HandDraw() ;
}

/* Try to match the currently held card on the current card under the
   and remove them both if they add up to a King */
LOCAL_C VOID PutDownCard(VOID) {
	BOOL		match = FALSE ;

	if (	ON_PYRAMID &&
			MATCH(cards.hold, cards.pyramid[CROW][CPOS]) ) {
		match = TRUE ;
		cards.pyramid[CROW][CPOS] = NULL ;
		cards.hold = NULL ;
		ClrCard(GFX_CARD_X(CROW, CPOS), GFX_CARD_Y(CROW)) ;
		DrawPyramid(CROW - 2, CPOS - 1) ;
		UpdateRemovables() ;
	}
	else if (	ON_TALON &&
					cards.talon != NULL &&
					MATCH(cards.hold, cards.talon) ) {
		match = TRUE ;
		cards.hold = NULL ;
		cards.talon = cards.talon->pnext ;
		DrawPile(POS_TALON) ;
	}

	if ( match ) {
		if ( LPILE == POS_PYRAMID )
			DrawPyramid(LROW - 2, LPOS - 1) ;
		LPILE = POS_HAND ;
		UpdateRemovables() ;
	}
}

/* Execute menu command */
LOCAL_C VOID ExecuteCommand(INT keycode) {
	if ( (keycode = uLocateCommand(keycode)) < 0 )
		return ;

	switch(keycode) {

		case 0 : /* New Game */
			Check( p_enter5(Query, RS_NEW_GAME, RS_YES, RS_NO, &status.new_game) ) ;
			break ;

		case 1 : /* Exit */
			Check( p_enter5(Query, RS_QUIT, RS_YES, RS_NO, &status.quit) ) ;
			break ;

		default :
			break ;
	}
}

/* Modify the pointer to take account of the game status */
LOCAL_C VOID UpdatePointer(BOOL drawflag) {
	static P_RECT		RECT_PTR = {	{GFX_ARROW_X, GFX_ARROW_Y},
												{GFX_ARROW_X + GFX_ARROW_WID, GFX_ARROW_Y + GFX_ARROW_HGT} } ;
	W_WINDATA			windata ;
	P_POINT				pos = { 0, 0 } ;

	status.curr_pos = MAX(0, MIN(cards.n_removables - 1, status.curr_pos)) ;

	if ( CPILE == POS_TALON ) {
		windata.extent.tl.x = GFX_TALON_X ;
		windata.extent.tl.y = GFX_TALON_Y ;
	}
	else {
		/* On pyramid */
		windata.extent.tl.x = GFX_CARD_X(CROW, CPOS) ;
		windata.extent.tl.y = GFX_CARD_Y(CROW) ;
	}

	windata.extent.tl.x -= (!HOLDING ? 5 : 6) ;
	windata.extent.tl.y += (!HOLDING ? 2 : 9) ;
	windata.extent.width = (!HOLDING ? GFX_ARROW_WID : GFX_CARD_WID) ;
	windata.extent.height = (!HOLDING ? GFX_ARROW_HGT : GFX_CARD_HGT) ;

	wSetWindow(gfx.ptr, W_WIN_EXTENT, &windata) ;
	if ( drawflag ) {
		gCreateTempGC0(gfx.ptr) ;
		if ( HOLDING ) {
			ClrCard(0, 0) ;
			DrawCard(TRUE, cards.hold, 0, 0) ;
		}
		else
			gCopyBit(&pos, gfx.bmp, &RECT_PTR, G_TRMODE_REPL) ;
		gFreeTempGC() ;
	}
}

/* Are there any plays possible */
LOCAL_C BOOL PlayPossible(VOID) {
	CARD *		picard ;
	CARD *		pjcard ;
	INT			icard ;
	INT			jcard ;

	if (	cards.hand != NULL ||
			HOLDING )
		return(TRUE) ;

	for ( icard = 0 ; icard < cards.n_removables + 1 ; icard++ ) {
		picard =	icard < cards.n_removables ?
						cards.pyramid[cards.removables_row[icard]][cards.removables_pos[icard]] :
						cards.talon ;
		if ( picard == NULL )
			continue ;

		for ( jcard = icard + 1 ; jcard < cards.n_removables + 1 ; jcard++ ) {
			pjcard =	jcard < cards.n_removables ?
							cards.pyramid[cards.removables_row[jcard]][cards.removables_pos[jcard]] :
							cards.talon ;
			if ( pjcard == NULL )
				continue ;

			if (	RANK(picard) == 12 ||
					RANK(pjcard) == 12 ||
					MATCH(picard, pjcard) )
				return(TRUE) ;
		}
	}

	return(FALSE) ;
}

#pragma save, ENTER_CALL

/* Deal a new game */
LOCAL_C INT Deal(VOID) {
	INT				iLp ;
	INT				row ;
	INT				pos ;

	CPILE = POS_TALON ;
	LPILE = POS_HAND ;
	status.seedl = p_date() ;
	status.num_dealt = 0 ;

	/* Clear Deck */
	for (	iLp = 0 ;
			iLp < N_CARDS ;
			iLp++ ) {
		cards.deck[iLp].pnext = NULL ;
		cards.deck[iLp].index = iLp ;
	}

	/* Deal Pyramid */
	for ( row = 0 ;
			row < N_ROWS ;
			row++ ) {
		for (	pos = 0 ;
				pos <= row ;
				pos++ ) {
			cards.pyramid[row][pos] = &cards.deck[DealCard()] ;
			cards.pyramid[row][pos]->pnext = NULL ;
		}
	}

	/* Deal remaining cards to hand */
	for ( cards.hand = NULL ; status.num_dealt < N_CARDS ; ) {
		CARD *		pcard ;

		pcard = cards.hand ;
		cards.hand = &cards.deck[DealCard()] ;
		cards.hand->pnext = pcard ;
	}
	cards.talon = cards.hold = NULL ;

	UpdateRemovables() ;
	DrawPyramid(0, 0) ;
	DrawPile(POS_HAND) ;
	DrawPile(POS_TALON) ;

	UpdatePointer(TRUE) ;

	return(E_APP_NONE) ;
}

/* Play a single game until the player quits, or a new game is requested */
LOCAL_C INT PlayGame(VOID) {
	WMSG_KEY		key ;
	INT			ret ;

	status.new_game = FALSE ;

	do {

		uGetKey(&key) ;

		if ( key.keycode == CONS_EVENT_COMMAND )
			status.quit = TRUE ;
		else if ( key.keycode & W_SPECIAL_KEY )
			ExecuteCommand(key.keycode & ~W_SPECIAL_KEY) ;
		else switch( key.keycode ) {

			case W_KEY_HELP :
				if ( key.modifiers == W_CTRL_MODIFIER )
					SystemHelp() ;
				else
					Help(RH_MAIN, RH_MAIN) ;
				break ;

			case W_KEY_MENU :
				if ( (ret = uPresentMenus()) > 0 )
					ExecuteCommand(ret) ;
				break ;

			case W_KEY_RIGHT :
				if ( ON_TALON ) {
					CPILE++ ;
					status.curr_pos = 0 ;
				}
				else if ( status.curr_pos < cards.n_removables - 1 )
					status.curr_pos++ ;
				break ;

			case W_KEY_LEFT :
				if ( ON_PYRAMID )
					if ( status.curr_pos == 0 )
						CPILE-- ;
					else
						status.curr_pos-- ;
				break ;

			case W_KEY_UP :
			case W_KEY_HOME :
				CPILE = POS_TALON ;
				break ;

			case W_KEY_DOWN :
			case W_KEY_END :
				CPILE = POS_PYRAMID ;
				status.curr_pos = cards.n_removables - 1 ;
				break ;

			case ' ' :
				if ( HOLDING )
					PutDownCard() ;
				else
					PickUpCard() ;

				UpdatePointer(TRUE) ;
				break ;

			case W_KEY_RETURN :
				if ( !HOLDING )
					HandDraw() ;
				break ;

			case W_KEY_ESCAPE :
				if (	HOLDING &&
						LPILE != POS_HAND ) {
					CancelPlay() ;
					UpdatePointer(TRUE) ;
				}
				break ;

			default :
				break ;
		}

		if ( key.keycode >= W_KEY_UP && key.keycode <= W_KEY_END )
			UpdatePointer(FALSE) ;

		if ( !PlayPossible() ) {
			Check( p_enter5(Query, RS_NEW_GAME, RS_YES, RS_NO, &status.new_game) ) ;
			status.quit = !status.new_game ;
		}

	} while ( !(status.new_game || status.quit) ) ;

	return(E_APP_NONE) ;
}

/* General initialization of global variables etc. */
LOCAL_C INT SpecificInit(VOID) {
	P_POINT		bit_size ;
	W_WINDATA	windata = { W_WIN_NO_REDRAW,
									{ {0, 0}, GFX_ARROW_WID, GFX_ARROW_HGT },
									W_WIN_NO_MOUSE,
									W_WIN_BACK_BITMAP } ;

	hCrackCommandLine() ;
	p_unmarka() ;

	OpenResource(DatCommandPtr) ;

	/* Gfx init */
	gfx.win = uFindMainWid() ;
	gfx.bmp = gOpenBit(DatCommandPtr, 0, 0, &bit_size) ;
	gfx.gc = gCreateGC0(gfx.win) ;
	gfx.ptr = wCreateWindow(	gfx.win,
										W_WIN_NO_REDRAW | W_WIN_EXTENT | W_WIN_BACKGROUND,
										&windata, (UWORD) &gfx) ;
	wInitialiseWindowTree(gfx.ptr) ;
	wMakeVisible(gfx.ptr) ;

	/* Mem alloc */
	cards.deck = f_alloc(N_CARDS * sizeof(CARD)) ;
	
	/* About message */
	Help(RH_ABOUT, RH_MAIN) ;

	return(E_APP_NONE) ;
}

#pragma restore

GLDEF_C VOID main(VOID) {
	uCommonInit() ;
	wDisableLeaves(FALSE) ;
	Check( p_enter1(SpecificInit) ) ;
	do {
		Check( p_enter1(Deal) ) ;
		Check( p_enter1(PlayGame) ) ;
	} while ( !status.quit ) ;
	p_exit(E_APP_NONE) ;
}
