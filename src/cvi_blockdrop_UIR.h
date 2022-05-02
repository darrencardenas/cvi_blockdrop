/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  PNLMAIN                          1
#define  PNLMAIN_BTNSTART                 2       /* control type: command, callback function: CB_BtnStart */
#define  PNLMAIN_BTNQUIT                  3       /* control type: command, callback function: CB_BtnQuit */
#define  PNLMAIN_GRID                     4       /* control type: table, callback function: (none) */
#define  PNLMAIN_BTNPAUSE                 5       /* control type: command, callback function: CB_BtnPause */
#define  PNLMAIN_BTNDOWN                  6       /* control type: command, callback function: CB_BtnMoveDown */
#define  PNLMAIN_BTNRIGHT                 7       /* control type: command, callback function: CB_BtnMoveRight */
#define  PNLMAIN_BTNLEFT                  8       /* control type: command, callback function: CB_BtnMoveLeft */
#define  PNLMAIN_BTNROTATECW              9       /* control type: command, callback function: CB_BtnRotateCW */
#define  PNLMAIN_TEXTLOG                  10      /* control type: textBox, callback function: (none) */
#define  PNLMAIN_TIMERADVANCE             11      /* control type: timer, callback function: CB_TimerAdvanceBlock */
#define  PNLMAIN_TEXTGAMEOVER             12      /* control type: textMsg, callback function: (none) */
#define  PNLMAIN_NUMCLEARED               13      /* control type: numeric, callback function: (none) */


     /* Control Arrays: */

          /* (no control arrays in the resource file) */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK CB_BtnMoveDown(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CB_BtnMoveLeft(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CB_BtnMoveRight(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CB_BtnPause(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CB_BtnQuit(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CB_BtnRotateCW(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CB_BtnStart(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CB_TimerAdvanceBlock(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif