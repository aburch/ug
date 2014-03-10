// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  MacGraph.h													*/
/*																			*/
/* Purpose:   header file for MacGraph.c									*/
/*																			*/
/* Author:	  Peter Bastian/Henrik Rentz-Reichert							*/
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de				                        */
/*																			*/
/* History:   26.03.92 begin, ug version 2.0								*/
/*			  13.02.95 begin, ug version 3.0								*/
/*																			*/
/* Remarks:                                                                                                                             */
/*																			*/
/****************************************************************************/


/* RCS_ID
   $Header$
 */

/****************************************************************************/
/*																			*/
/* auto include mechanism and other include files							*/
/*																			*/
/****************************************************************************/

#ifndef __MACGRAPH__
#define __MACGRAPH__


#ifndef __COMPILER__
#include "ugtypes.h"
#endif

#ifndef __DEVICESH__
#include "ugdevices.h"
#endif

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

#define GRAPHWIN_MINSIZE                135
#define MAXTITLELENGTH                  128

/********************************************************************************/
/*																				*/
/* data structures exported by the corresponding source file					*/
/*																				*/
/********************************************************************************/

/********************************************************************************/
/*																				*/
/* macros for graph windows                                                                                                     */
/*																				*/
/********************************************************************************/

/********************************************************************************/
/*																				*/
/* data structures exported by the corresponding source file					*/
/*																				*/
/********************************************************************************/

struct graphwindow {

  WindowPtr theWindow;                                  /* used by mac's window manager         */

  /* now graph window specific stuff */
  struct graphwindow *next;                             /* linked list of all graph windows     */

  /* drawing parameters */
  short marker_size;                                            /* size of markers in pixels			*/
  short marker_id;                                              /* number of marker                                     */
  short textSize;                                               /* text size							*/

  /* save size of viewport */
  INT Global_LL[2];                                             /* global x-coord of plotting region	*/
  INT Global_UR[2];                                             /* global y-coord of plotting region	*/
  INT Local_LL[2];                                              /* local  x-coord of plotting region	*/
  INT Local_UR[2];                                              /* local  y-coord of plotting region	*/

  INT currTool;

  /* windows current point */
  int x;
  int y;

  ControlHandle vScrollBar;                             /* vertical scroll bar					*/
  ControlHandle hScrollBar;                             /* horizontal scroll bar				*/
  short nx,ny;                                                  /* discretization for scroll bars		*/
  short i,j;                                                            /* discrete midpoint coordinates		*/
};

typedef struct graphwindow GRAPH_WINDOW;

/********************************************************************************/
/*																				*/
/* function declarations														*/
/*																				*/
/********************************************************************************/

OUTPUTDEVICE    *InitMacOutputDevice    (void);

INT                      Mac_ActivateOutput     (WINDOWID win);
INT                      Mac_UpdateOutput               (WINDOWID win, INT tool);
GRAPH_WINDOW    *WhichGW                                (WindowPtr win);
void                     SetCurrentGW                   (GRAPH_WINDOW *g);
INT                      GrowGraphWindow                (GRAPH_WINDOW *gw, EventRecord *theEvent, DOC_GROW_EVENT *docGrow);
INT                              DragGraphWindow                (GRAPH_WINDOW *gw, EventRecord *theEvent, DOC_DRAG_EVENT *docDrag);

#endif
