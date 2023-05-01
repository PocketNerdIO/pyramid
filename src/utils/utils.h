/*  utils.h - some miscellaneous S3 utils
 */

#ifndef UTILS_H
	#define UTILS_H
	
#ifndef PLIB_H
	#include <plib.h>
#endif

#ifndef WLIB_H
	#include <wlib.h>
#endif

#ifndef HWIF_H
	#include <hwif.h>
#endif

#ifndef BOOL
	#define BOOL	INT
#endif

#ifndef TRUE
	#define TRUE	1
#endif

#ifndef FALSE
	#define FALSE	0
#endif

#ifndef MIN
	#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
	#define MAX(a,b)	((a) > (b) ? (a) : (b))
#endif

#define E_APP_NONE			0
#define E_UTILS_READ_RES	-129
#define E_UTILS_DIALOG		-130
#define E_APP_START			-256

GLREF_C INT Check(INT ret) ;
GLREF_C VOID Exit_Error(INT index) ;
GLREF_C INT Random(ULONG *pseed, INT low, INT high) ;

GLREF_C TEXT *BCStoZTS(TEXT *str) ;
GLREF_C TEXT *ZTStoBCS(TEXT *str) ;

GLREF_C BOOL Exists(TEXT *filename) ;
GLREF_C VOID SystemHelp(VOID) ;
GLREF_C VOID Help(INT help, INT index) ;

GLREF_C VOID TellNoMemory(VOID) ;

GLREF_C BOOL uDisableLeaves(BOOL disable) ;
GLREF_C VOID OpenResource(TEXT *name) ;
GLREF_C VOID *ReadResource(INT index, VOID **ppcell) ;
GLREF_C VOID ReadResource_buf(INT index, VOID *pcell) ;

#pragma save, ENTER_CALL
GLREF_C INT Query(INT res_title, INT res_pos, INT res_neg, BOOL *answer) ;
GLREF_C INT QueryStr(TEXT *ptitle, INT res_pos, INT res_neg, BOOL *answer) ;
GLREF_C INT ThreeQuery(INT res_title, INT res_pos, INT res_neg, INT res_maybe, INT *answer) ;
#pragma restore

GLREF_C INT Res_OpenDialog(INT rs_title) ;
GLREF_C INT Res_RunDialog(VOID);
GLREF_C INT Res_AddChoiceList(INT rs_prompt, INT rs_1, INT rs2, UWORD *psel) ;
GLREF_C INT Res_AddDialogItem(INT type, INT rs_prompt, VOID *pdata) ;
GLREF_C INT Res_AddDCL(INT rs_prompt, UWORD *psel, H_DI_CHOICE *pch) ;
GLREF_C INT Res_Add2ButtonList(INT rs_1, INT code_1, INT rs_2, INT code_2) ;
GLREF_C VOID Res_FreeMem(VOID) ;

GLREF_C VOID InfoMsg(INT index) ;
GLREF_C VOID InfoMsgCorner(INT index) ;
GLREF_C VOID Res_ErrorString(INT index) ;

/* Allocation remember routines */

typedef struct REMEMBER_TAG {
	struct REMEMBER_TAG *		pnext ;
} REMEMBER ;

typedef struct REMEMBER_LLIST_TAG {
	REMEMBER *			phead ;
	REMEMBER *			ptail ;
} REMEMBER_LLIST ;

GLREF_C VOID *AllocRemember(REMEMBER_LLIST *prem, INT size) ;
GLREF_C VOID FreeRemember(REMEMBER_LLIST *prem) ;

/* Scroll and status bar routines */

#define SCROLL_WID			4

typedef struct SCROLL_BAR_STATE_TAG {
	ULONG		start,
				size,
				total ;
} SCROLL_BAR_STATE ;

typedef struct SCROLL_BAR_TAG {
	SCROLL_BAR_STATE		state ;			/* State of scroll bar */
	BOOL						horz ;			/* Orientation */
} SCROLL_BAR ;

typedef struct STATUS_BAR_TAG {
	UINT					font_id ;
	UINT					style ;
	UINT					ascent ;
} STATUS_BAR ;

typedef union BAR_DATA_TAG {
	SCROLL_BAR			scroll ;
	STATUS_BAR			status ;
} BAR_DATA ;

typedef struct BAR_OBJ_TAG {
	UINT					type ;			/* Type of bar object - BAR_STATUS or BAR_SCROLL */
	P_EXTENT				extent ;			/* Extent of window */
	BOOL					open ;			/* Window open */
	UINT					win_id ;			/* Id of window */
	BAR_DATA				data ;			/* type-specific data */
} BAR_OBJ ;

GLREF_C VOID MakeScrollBar(BAR_OBJ *pbar, WORD tl_x, WORD tl_y, WORD length, BOOL horz) ;

GLREF_C VOID MakeStatusBar(BAR_OBJ *pbar, UINT font_id, UINT style, P_POINT *pscreen_size) ;

GLREF_C VOID UpdateBar(BAR_OBJ *pbar, VOID *pdata) ;
GLREF_C VOID ShowBar(BAR_OBJ *pbar) ;
GLREF_C VOID HideBar(BAR_OBJ *pbar) ;
GLREF_C VOID ToggleBar(BAR_OBJ *pbar) ;
GLREF_C VOID GetBarExtent(BAR_OBJ *pbar, P_EXTENT *pextent) ;

#endif