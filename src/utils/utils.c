/* utils.c - miscellaneous S3 routines - mainly dealing with resources
 */

#include <rscfile.xg>
#include <utils.h>

LOCAL_D VOID *					u_rcb ;							/* ptr to utils resource control block */
LOCAL_D TEXT *					u_resname ;						/* name of resource file */
LOCAL_D REMEMBER_LLIST		u_rem = { NULL, NULL } ;	/* remember struct for resources */
LOCAL_D BOOL					u_leaves = TRUE ;				/* leaves on by default */

LOCAL_D TEXT *					ReplaceDisk = "Please replace SSD" ;

GLDEF_C BOOL uDisableLeaves(BOOL disable) {
	BOOL		oleaves ;

	oleaves = u_leaves ;
	u_leaves = !disable ;
	return(oleaves) ;
}

/* Returns TRUE if file exists */
GLDEF_C BOOL Exists(TEXT *filename) {
	P_INFO		pinfo ;

	return( !p_finfo(filename, &pinfo) ) ;
}

/* Check a negative error and quit */
GLDEF_C INT Check(INT ret) {
	if ( ret < -128 ) {
		/* App error */
		p_notify("Fatal Application Error:", NULL, NULL, NULL, NULL) ;
		p_exit(E_APP_NONE) ;
	}
	else if ( ret < 0 && ret != E_FILE_EOF ) {
		p_notifyerr(ret, "Error:", NULL, NULL, NULL) ;
		p_exit(ret) ;
	}
	else
		return(ret) ;
}

/* Exit with error index from the resource file */
GLDEF_C VOID Exit_Error(INT index) {
	TEXT *		str = NULL ;

	uDisableLeaves(TRUE) ;
	ReadResource(index, &str) ;
	if ( str != NULL ) {
		uErrorString(str) ;
		p_free(str) ;
	}

	p_exit(E_APP_NONE) ;
}

/* Produces a random INT between low and high inclusive */
GLDEF_C INT Random(ULONG *pseed, INT low, INT high) {
	return( low + (INT) ( p_randl(pseed) % (high - low + 1) ) ) ;
}

GLDEF_C VOID SystemHelp(VOID) {
	hHelpSubSystem(-112, 0) ;
}

GLDEF_C VOID Help(INT help, INT index) {
	hHelpSubSystem(help, index) ;
}

/* Notify the user of out of memory error */
GLDEF_C VOID TellNoMemory(VOID) {
	TEXT		buf[40] ;

	p_errs(buf, E_GEN_NOMEMORY) ;
	wsAlertW(WS_ALERT_CLIENT, 0, buf, 0) ;
}

GLDEF_C VOID OpenResource(TEXT *name) {
	/* Open resource file for help */
	u_resname = name ;
	u_rcb = f_new(1, C_RSCFILE) ;
	f_send3(u_rcb, O_RS_INIT, u_resname) ;
	hDeclareAppRcb(u_rcb);
}

LOCAL_C VOID *u_ReadResource(INT index, BOOL remember) {
	INT				ret ;
	VOID *			pcell = NULL ;
	REMEMBER *		pt ;

	FOREVER {
		ret = p_entersend4(u_rcb, O_RS_READ, index, &pcell) ;
		if ( ret >= 0 ) {

			if ( remember ) {
				pt = p_adjust(pcell, 0, sizeof(REMEMBER)) ;

				if ( pt == NULL ) {
					p_free(pcell) ;
					pcell = NULL ;
					FreeRemember(&u_rem) ;
					if ( u_leaves )
						p_leave(E_UTILS_READ_RES) ;
					else
						return(pcell) ;
				}
				else {
					u_rem.ptail->pnext = pt ;
					pt->pnext = NULL ;
					u_rem.ptail = pt ;
					return(((BYTE *) pt) + sizeof(REMEMBER)) ;
				}
			}
			else {
				return(pcell) ;
			}
		}
		while ( ret < 0 ) {
			if ( ret == E_GEN_NOMEMORY ) {
				TellNoMemory() ;
				if ( remember )
					FreeRemember(&u_rem) ;
				if ( u_leaves )
					p_leave(E_UTILS_READ_RES) ;
				else
					return(NULL) ;
			}
			wsAlertW(WS_ALERT_CLIENT, 0, ReplaceDisk, 0) ;
			p_send2(u_rcb, 0) ;		/* destroy object */
			FOREVER {
				u_rcb = p_new(1, C_RSCFILE) ;
				if ( u_rcb )
					break ;
				TellNoMemory() ;
			}
			ret = p_entersend3(u_rcb, O_RS_INIT, u_resname) ;
			if ( ret >= 0 )
				hDeclareAppRcb(u_rcb) ;
		}
	}
}

GLDEF_C VOID *ReadResource(INT index, VOID **ppcell) {
	if ( *ppcell != NULL )
		p_free(*ppcell) ;

	return( *ppcell = u_ReadResource(index, FALSE) ) ;
}

/* Read resource <index> into *pcell */
GLDEF_C VOID ReadResource_buf(INT index, VOID *pcell) {
	INT				ret ;

	if ( pcell == NULL )
		return ;

	FOREVER {
		ret = p_entersend4(u_rcb, O_RS_READ_BUF, index, pcell) ;
		if ( ret >= 0 )
			return ;
		while ( ret < 0 ) {
			wsAlertW(WS_ALERT_CLIENT, 0, ReplaceDisk, 0) ;
			p_send2(u_rcb, 0) ;		/* destroy object */
			FOREVER {
				u_rcb = p_new(1, C_RSCFILE) ;
				if ( u_rcb )
					break ;
				TellNoMemory() ;
			}
			ret = p_entersend3(u_rcb, O_RS_INIT, u_resname) ;
			if ( ret >= 0 )
				hDeclareAppRcb(u_rcb) ;
		}
	}
}

#pragma save, ENTER_CALL

GLDEF_C INT Query(INT res_title, INT res_pos, INT res_neg, BOOL *answer) {
	BOOL			ret = TRUE ;

	*answer = TRUE ;

	Res_OpenDialog(res_title) ;
	Res_Add2ButtonList(	res_neg, -W_KEY_ESCAPE,
								res_pos, W_KEY_RETURN ) ;
	ret = Res_RunDialog() ;

	*answer = ret ;

	return(E_APP_NONE) ;
}

GLDEF_C INT QueryStr(TEXT *ptitle, INT res_pos, INT res_neg, BOOL *answer) {
	BOOL			ret = TRUE ;

	*answer = TRUE ;

	if ( uOpenDialog(ptitle) )
		p_leave(E_UTILS_DIALOG) ;

	Res_Add2ButtonList(	res_neg, -W_KEY_ESCAPE,
								res_pos, W_KEY_RETURN ) ;
	ret = Res_RunDialog() ;

	*answer = ret ;

	return(E_APP_NONE) ;
}

GLDEF_C INT ThreeQuery(INT res_title, INT res_pos, INT res_neg, INT res_maybe, INT *answer) {
	INT			ret ;

	Res_OpenDialog(res_title) ;
	ret = uAddButtonList(	(TEXT *) u_ReadResource(res_neg, TRUE), -W_KEY_ESCAPE,
									(TEXT *) u_ReadResource(res_pos, TRUE), W_KEY_RETURN,
									(TEXT *) u_ReadResource(res_maybe, TRUE), W_KEY_TAB,
									NULL) ;
	Res_FreeMem() ;

	if ( ret ) {
		*answer = 0 ;
		return(E_UTILS_DIALOG) ;
	}

	ret = Res_RunDialog() ;

	if ( ret == W_KEY_RETURN )
		*answer = 2 ;
	else if ( ret == W_KEY_TAB )
		*answer = 1 ;
	else
		*answer = 0 ;

	return(E_APP_NONE) ;
}

#pragma restore

/* Display a resource message */
GLDEF_C VOID InfoMsg(INT index) {
	TEXT *		buf = NULL ;
	BOOL			oleaves ;

	oleaves = uDisableLeaves(TRUE) ;
	ReadResource(index, &buf) ;
	if ( buf != NULL ) {
		wInfoMsg(buf) ;
		p_free(buf) ;
	}
	uDisableLeaves(oleaves) ;
}

/* Display a resource message */
GLDEF_C VOID InfoMsgCorner(INT index) {
	TEXT *		buf = NULL ;
	BOOL			oleaves ;

	oleaves = uDisableLeaves(TRUE) ;
	ReadResource(index, &buf) ;
	if ( buf != NULL ) {
		wInfoMsgCorner(buf, W_CORNER_BOTTOM_LEFT) ;
		p_free(buf) ;
	}
	uDisableLeaves(oleaves) ;
}

/* Allocate and remember - if fail, free all memory in the remember
   structure and call p_leave */
GLDEF_C VOID *AllocRemember(REMEMBER_LLIST *prem, INT size) {
	if ( prem == NULL )
		return(NULL) ;

	prem->ptail->pnext = p_alloc(size + sizeof(REMEMBER)) ;

	if ( prem->ptail->pnext == NULL ) {
		FreeRemember(prem) ;
		if ( u_leaves )
			p_leave(E_GEN_NOMEMORY) ;
		else
			return(NULL) ;
	}

	prem->ptail = prem->ptail->pnext ;
	prem->ptail->pnext = NULL ;

	return( ((BYTE *) prem->ptail) + sizeof(REMEMBER) ) ;
}

GLDEF_C VOID FreeRemember(REMEMBER_LLIST *prem) {
	REMEMBER *		pcell = NULL ;
	REMEMBER *		pnext = NULL ;

	for ( pcell = prem->phead ; pcell != NULL ; ) {
		pnext = pcell->pnext ;
		p_free(pcell) ;
		pcell = pnext ;
	}

	prem->phead = NULL ;
	prem->ptail = NULL ;
}

GLDEF_C INT Res_OpenDialog(INT rs_title) {
	INT			ret ;
	
	ret = uOpenDialog((TEXT *) u_ReadResource(rs_title, TRUE)) ;
	Res_FreeMem() ;

	if ( ret && u_leaves )
		p_leave(E_UTILS_DIALOG) ;
	else
		return(ret) ;
}

GLDEF_C INT Res_RunDialog(VOID) {
	INT			ret ;

	ret = uRunDialog() ;

	if ( ret < 0 && u_leaves )
		p_leave(E_UTILS_DIALOG) ;
	else
		return(ret) ;
}

GLDEF_C INT Res_AddChoiceList(INT rs_prompt, INT rs_1, INT rs_2, UWORD *psel) {
	INT			ret ;

	ret = uAddChoiceList(	(TEXT *) u_ReadResource(rs_prompt, TRUE),
									psel,
									(TEXT *) u_ReadResource(rs_1, TRUE),
									(TEXT *) u_ReadResource(rs_2, TRUE),
									NULL) ;
	Res_FreeMem() ;

	if ( ret && u_leaves )
		p_leave(E_UTILS_DIALOG) ;
	else
		return(ret) ;
}

GLDEF_C INT Res_Add2ButtonList(INT rs_1, INT code_1, INT rs_2, INT code_2) {
	INT			ret ;

	ret = uAddButtonList(	(TEXT *) u_ReadResource(rs_1, TRUE),
									code_1,
									(TEXT *) u_ReadResource(rs_2, TRUE),
									code_2,
									NULL) ;
	Res_FreeMem() ;

	if ( ret && u_leaves )
		p_leave(E_UTILS_DIALOG) ;
	else
		return(ret) ;
}

GLDEF_C INT Res_AddDialogItem(INT type, INT rs_prompt, VOID *pdata) {
	INT			ret ;

	ret = uAddDialogItem(	type,
									(TEXT *) u_ReadResource(rs_prompt, TRUE),
									pdata) ;
	Res_FreeMem() ;

	if ( ret && u_leaves )
		p_leave(E_UTILS_DIALOG) ;
	else
		return(ret) ;
}

GLDEF_C INT Res_AddDCL(INT rs_prompt, UWORD *psel, H_DI_CHOICE *pch) {
	INT			ret ;

	ret = uAddDCL(	(TEXT *) u_ReadResource(rs_prompt, TRUE),
						psel,
						pch) ;
	Res_FreeMem() ;

	if ( ret && u_leaves )
		p_leave(E_UTILS_DIALOG) ;
	else
		return(ret) ;
}

GLDEF_C VOID Res_FreeMem(VOID) {
	FreeRemember(&u_rem) ;
}

/* Converts a BCS string at str to a ZTS string */
GLDEF_C TEXT *BCStoZTS(TEXT *str) {
	*p_bcpy(&str[0], &str[1], str[0])= '\x0' ;

	return(str) ;
}

GLDEF_C TEXT *ZTStoBCS(TEXT *str) {
	TEXT		len ;

	p_bcpy(&str[1], str, len = p_slen(str)) ;
	str[0] = len ;

	return(str) ;
}

GLDEF_C VOID Res_ErrorString(INT index) {
	TEXT *		str = NULL ;
	BOOL			oleaves ;

	oleaves = uDisableLeaves(TRUE) ;

	ReadResource(index, &str) ;
	if ( str != NULL ) {
		uErrorString(str) ;
		p_free(str) ;
	}
	uDisableLeaves(oleaves) ;
}

#define BAR_SCROLL			0
#define BAR_STATUS			1

GLDEF_C VOID ShowBar(BAR_OBJ *pbar) {
	W_WINDATA		windata = {	W_WIN_NO_REDRAW,
										{{0, 0}, 0, 0},
										0,
										W_WIN_BACK_BITMAP } ;
	UINT				oleaves ;

	if ( pbar->open )
		return ;

	windata.extent = pbar->extent ;

	oleaves = wDisableLeaves(TRUE) ;
	pbar->win_id = wCreateWindow(	0,
											W_WIN_EXTENT | W_WIN_BACKGROUND | W_WIN_NO_REDRAW,
											&windata,
											(UWORD) pbar ) ;

	if (pbar->win_id == E_GEN_NOMEMORY )
		pbar->open = FALSE ;
	else {
		wInitialiseWindowTree(pbar->win_id) ;
		pbar->open = TRUE ;
	}

	wDisableLeaves(oleaves) ;
}

GLDEF_C VOID HideBar(BAR_OBJ *pbar) {
	UINT		oleaves ;

	if ( !pbar->open )
		return ;

	oleaves = wDisableLeaves(TRUE) ;

	wCloseWindowTree(pbar->win_id) ;
	pbar->win_id = 0 ;
	pbar->open = FALSE ;
	wDisableLeaves(oleaves) ;
}

GLDEF_C VOID ToggleBar(BAR_OBJ *pbar) {
	(pbar->open ? HideBar : ShowBar)(pbar) ;
}

GLDEF_C VOID GetBarExtent(BAR_OBJ *pbar, P_EXTENT *pextent) {
	*pextent = pbar->extent ;
}

GLDEF_C VOID MakeStatusBar(	BAR_OBJ *pbar,
										UINT font_id,
										UINT style,
										P_POINT *pscreen_size ) {
	G_FONT_INFO		font_info ;

	gFontInfo(font_id, style, &font_info) ;

	pbar->data.status.font_id = font_id ;
	pbar->data.status.style = style ;
	pbar->data.status.ascent = font_info.ascent ;

	pbar->extent.height = 4 + font_info.height ;
	pbar->extent.width = pscreen_size->x ;
	pbar->extent.tl.x = 0 ;
	pbar->extent.tl.y = pscreen_size->y - pbar->extent.height ;

	pbar->open = FALSE ;
	pbar->win_id = 0 ;
	pbar->type = BAR_STATUS ;
}

LOCAL_C VOID UpdateStatusBar(BAR_OBJ *pbar, TEXT *str) {
	G_GC			g_gc ;
	P_RECT		rect = { {0, 0} } ;

	if (	!pbar->open ||
			pbar->type != BAR_STATUS )
		return ;

	g_gc.style = pbar->data.status.style ;
	g_gc.font = pbar->data.status.font_id ;
	gCreateTempGC(pbar->win_id, G_GC_MASK_STYLE | G_GC_MASK_FONT, &g_gc) ;

	rect.br.x = pbar->extent.width ;
	rect.br.y = pbar->extent.height ;
	gBorderRect(&rect, W_BORD_CORNER_1) ;

	p_insrec(&rect, 2, 2) ;

	gPrintBoxText(	&rect,
						pbar->data.status.ascent,
						G_TEXT_ALIGN_LEFT,
						0,
						str, p_slen(str) ) ;

	gFreeTempGC() ;
}

#define SCROLL_BORD_WID			1
#define SCROLL_MIN				4

GLDEF_C VOID MakeScrollBar(	BAR_OBJ *pbar,
										WORD tl_x,
										WORD tl_y,
										WORD length,
										BOOL horz) {

	pbar->data.scroll.state.start = 0L ;
	pbar->data.scroll.state.size = 0L ;
	pbar->data.scroll.state.total = 0L ;
	pbar->data.scroll.horz = horz ;

	pbar->extent.tl.x = tl_x ;
	pbar->extent.tl.y = tl_y ;
	pbar->extent.width = (horz ? length : SCROLL_WID) ;
	pbar->extent.height = (horz ? SCROLL_WID : length) ;

	pbar->open = FALSE ;
	pbar->win_id = 0 ;
	pbar->type = BAR_SCROLL ;
}

LOCAL_C VOID UpdateScrollBar(BAR_OBJ *pbar, SCROLL_BAR_STATE *pstate) {
	P_RECT		rect = { {0, 0} } ;
	ULONG			bar_total_pixels ;
	UINT			oleaves ;

	if ( pbar->type != BAR_SCROLL )
		return ;

	if ( pstate != NULL )
		pbar->data.scroll.state = *pstate ;

	if ( !pbar->open )
		return ;

	rect.br.x = pbar->extent.width ;
	rect.br.y = pbar->extent.height ;

	oleaves = wDisableLeaves(TRUE) ;

	gCreateTempGC0(pbar->win_id) ;

	if ( pbar->data.scroll.state.size >= pbar->data.scroll.state.total )
		gClrRect(&rect, G_TRMODE_CLR) ;
	else {
		gBorderRect(&rect, W_BORD_CORNER_1) ;
		p_insrec(&rect, SCROLL_BORD_WID, SCROLL_BORD_WID) ;
		gFillPattern(&rect, WS_BITMAP_GREY, G_TRMODE_REPL) ;

		if ( pbar->data.scroll.horz ) {
			bar_total_pixels = rect.br.x - rect.tl.x ;
			rect.tl.x += (pbar->data.scroll.state.start * bar_total_pixels) / pbar->data.scroll.state.total ;
			rect.br.x = rect.tl.x + MAX((pbar->data.scroll.state.size * bar_total_pixels) / pbar->data.scroll.state.total, SCROLL_MIN) ;
		}
		else {
			bar_total_pixels = rect.br.y - rect.tl.y ;
			rect.tl.y += (pbar->data.scroll.state.start * bar_total_pixels) / pbar->data.scroll.state.total ;
			rect.br.y = rect.tl.y + MAX((pbar->data.scroll.state.size * bar_total_pixels) / pbar->data.scroll.state.total, SCROLL_MIN) ;
		}

		gClrRect(&rect, G_TRMODE_SET) ;
	}

	gFreeTempGC() ;

	wDisableLeaves(oleaves) ;
}

GLDEF_C VOID UpdateBar(BAR_OBJ *pbar, VOID *pdata) {
	if ( pbar->type == BAR_STATUS ) {
		UpdateStatusBar(pbar, pdata) ;
	}
	else if ( pbar->type == BAR_SCROLL ) {
		UpdateScrollBar(pbar, pdata) ;
	}
}

