/****************************************************************************/
/*																			*/
/* File:	  MInterface.m													*/
/*																			*/
/* Purpose:   MacOS X graphical user interface for ug	 					*/
/*            All functions UG needs for user interaction are				*/
/*            defined here.	See below for comments.							*/
/*																			*/
/* Author:	  Volker Reichenberger											*/
/*			  Interdisziplin"ares Zentrum f"ur Wissenschaftliches Rechnen	*/
/*			  Universit"at Heidelberg										*/
/*			  Im Neuenheimer Feld 368										*/
/*			  69210 Heidelberg												*/
/*			  email: Volker.Reichenberger@IWR.Uni-Heidelberg.DE		    	*/
/*																			*/
/*	History:  June 4, 1999 begin (based on OPENSTEP code)					*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* include files															*/
/*			  system include files											*/
/*			  application include files 									*/
/*																			*/
/****************************************************************************/

/* OPENSTEP specific includes */
#import <Cocoa/Cocoa.h>

/* standard C includes */
#include <string.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/* interface includes */
#include "compiler.h"
#include "ugdevices.h"
#include "initdev.h"
#include "debug.h"
#include "general.h"
#include "misc.h"
#include "heaps.h"
#include "defaults.h"
#include "cmdint.h"

#include "MGraphicWindow.h"
#include "MGraphicView.h"
#include "MInterface.h"

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/
#define VERSION 	"This is "UG_VERSION" from $Date$\n"

/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/



static HEAP *guiHeap=NULL;
static long guiHeapSize=32000;
SHORT_POINT moveto_point;
static GRAPH_WINDOW *currgw;
static GRAPH_WINDOW *windowList=NULL;

OUTPUTDEVICE *MacOSXServerOutputDevice;

struct ugcolortable	{
	int red;
	int green;
	int blue;
};
typedef struct ugcolortable UGColorTable;
static UGColorTable colorTable[256];
NSColor	*currentColor=nil;

NSAutoreleasePool *pool;

static int dbcount=0;

/****************************************************************************/
/*																			*/
/* export global variables													*/
/*																			*/
/****************************************************************************/




/****************************************************************************/
/*																			*/
/* export global variables per function call								*/
/*																			*/
/****************************************************************************/

/****************************************************************************
	The following functions are connected with UGs graphics system in 
	InitScreen.
*****************************************************************************/

static void MacOSXServerMove (SHORT_POINT point)
{
	PRINTDEBUG(dev,4,("MacOSXServerMove(%ld)\n",dbcount++))
	[currgw->theGraphWindow moveToPoint:point];
	return;
}

static void MacOSXServerDraw (SHORT_POINT point)
{
	PRINTDEBUG(dev,4,("MacOSXServerDraw(%ld)\n",dbcount++))
	[currgw->theGraphWindow drawLineTo:point];
	return;
}

static void MacOSXServerPolyline (SHORT_POINT *points, INT n)
{
	PRINTDEBUG(dev,4,("MacOSXServerPolyline(%ld)\n",dbcount++))
	[currgw->theGraphWindow drawPolyLine:points noOfPoints:n];
}

static void MacOSXServerInversePolyline (SHORT_POINT *points, INT n)
{
	PRINTDEBUG(dev,4,("MacOSXServerInversePolyline(%ld)\n",dbcount++))
	[currgw->theGraphWindow drawInversePolygon:points noOfPoints:n];
	return;
}

static void MacOSXServerPolygon (SHORT_POINT *points, INT n)
{
	PRINTDEBUG(dev,4,("MacOSXServerPolygon(%ld)\n",dbcount++))
	[currgw->theGraphWindow drawPolygon:points noOfPoints:n];
	return;
}

static void MacOSXServerShadedPolygon (SHORT_POINT *points, INT n, DOUBLE i)
{
	PRINTDEBUG(dev,4,("MacOSXServerShadedPolygon(%ld)\n",dbcount++))
	[currgw->theGraphWindow drawShadedPolygon:points noOfPoints:n intensity:i];
	return;
}

static void MacOSXServerInversePolygon (SHORT_POINT *points, INT n)
{
	PRINTDEBUG(dev,4,("MacOSXServerInversePolygon(%ld)\n",dbcount++))
	[currgw->theGraphWindow drawInversePolygon:points noOfPoints:n];
	return;
}

static void MacOSXServerErasePolygon (SHORT_POINT *points, INT n)
{
	PRINTDEBUG(dev,4,("MacOSXServerErasePolygon(%ld)\n",dbcount++))
	[currgw->theGraphWindow erasePolygon:points noOfPoints:n];
	return;
}

static void MacOSXServerSetMarker (short n)
{
	PRINTDEBUG(dev,4,("MacOSXServerSetMarker(%ld)\n",dbcount++))
	[currgw->theGraphWindow setMarker:n];
	return;
}

static void MacOSXServerSetMarkerSize (short s)
{
	PRINTDEBUG(dev,4,("MacOSXServerSetMarkerSize(%ld)\n",dbcount++))
	[currgw->theGraphWindow setMarkerSize:s];
	return;
}

static void MacOSXServerPolymark (short n, SHORT_POINT *points)
{
	PRINTDEBUG(dev,4,("MacOSXServerPolymark(%ld)\n",dbcount++))
	[currgw->theGraphWindow polyMark:points noOfPoints:n];
	return;
}

/*static void MacOSXServerInvMarker (SHORT_POINT point)
{
	[currgw->theGraphWindow invMarker:point];
	return;
}*/

		
static void MacOSXServerInvPolymark (short n, SHORT_POINT *points)
{
	PRINTDEBUG(dev,4,("MacOSXServerInvPolymark(%ld)\n",dbcount++))
	[currgw->theGraphWindow invPolyMark:points noOfPoints:n];
	return;
}

static void MacOSXServerDrawText (const char *s, INT m)
{
	PRINTDEBUG(dev,4,("MacOSXServerDrawText(%ld)\n",dbcount++))
	[currgw->theGraphWindow drawText:s mode:m];
	return;
}

static void MacOSXServerCenteredText (SHORT_POINT point, const char *s, INT m)
{
	PRINTDEBUG(dev,4,("MacOSXServerCenteredText(%ld)\n",dbcount++))
	[currgw->theGraphWindow drawCenteredText:s
	                                 atPoint:point mode:m];
	return;
}

static void MacOSXServerClearViewPort (void)
{
	PRINTDEBUG(dev,4,("MacOSXServerClearViewPort(%ld)\n",dbcount++))
	[currgw->theGraphWindow clearView];
	return;
}

static void MacOSXServerSetLineWidth (short w)
{
	PRINTDEBUG(dev,4,("MacOSXServerSetLineWidth(%ld)\n",dbcount++))
	[currgw->theGraphWindow setLineWidth:w];
	return;
}

static void MacOSXServerSetTextSize (short s)
{
	PRINTDEBUG(dev,4,("MacOSXServerSetTextSize(%ld)\n",dbcount++))
	[currgw->theGraphWindow setTextSize:s];
	return;
}

static void MacOSXServerSetColor (long index)
{	
	PRINTDEBUG(dev,4,("MacOSXServerSetColor(%ld)\n",dbcount++))
	[currgw->theGraphWindow setColorRed:(float)colorTable[index].red/(float)0xFFFF
	                              green:(float)colorTable[index].green/(float)0xFFFF
	                               blue:(float)colorTable[index].blue/(float)0xFFFF];
	/*printf ("c[%ld] = (%f, %f, %f)\n", index,
	        (float)colorTable[index].red/(float)0xFFFF,
	        (float)colorTable[index].green/(float)0xFFFF,
	        (float)colorTable[index].blue/(float)0xFFFF);*/
	return;
}

static void MacOSXServerSetPaletteEntry (long index, short r, short g, short b)
{
	PRINTDEBUG(dev,4,("MacOSXServerSetPaletteEntry %d %d %d %d\n",index,r,g,b))
	[currgw->theGraphWindow setPaletteEntryWithIndex:index red:r green:g blue:b];
	return;
}

static void MacOSXServerSetNewPalette (long index, long count, short *r, short *g, short *b)
{
	PRINTDEBUG(dev,4,("MacOSXServerSetNewPalette %d %ld %d %d %d\n",index,count,*r,*g,*b))
	[currgw->theGraphWindow setNewPaletteEntryWithIndex:index
	                                       withCount:count
	                                       red:*r green:*g blue:*b];
	return;
}

static void MacOSXServerGetPaletteEntry (long index, short *r, short *g, short *b)
{
	PRINTDEBUG(dev,4,("MacOSXServerGetPaletteEntry %d %d %d %d\n",index,*r,*g,*b))
	[currgw->theGraphWindow getPaletteEntryWithIndex:index red:r green:g blue:b];
	return;
}

static void MacOSXServerFlush (void)
{
	PRINTDEBUG(dev,4,("MacOSXServerFlush(%ld)\n",dbcount++))
	[currgw->theGraphWindow flush];
	return;
}




/****************************************************************************/
/*
   GetScreenSize - Return sreen size 												

   SYNOPSIS:
   INT GetScreenSize (INT size[2]);

   PARAMETERS:
.  size[2] - pointer to the size of screen

   RETURN VALUE:
   INT
.n     0 if ok														
.n     1 if error occured.
*/
/****************************************************************************/

INT GetScreenSize (INT size[2])
{
	NSRect screenRect = [[NSScreen mainScreen] frame];
	
	size[0] = screenRect.size.width;
	size[1] = screenRect.size.height;
	
	return (0);
}

/****************************************************************************/
/*																			
   GUI_GetNextEvent - Process an event from the system and pass it to ug												

   SYNOPSIS:
   INT GetNextUGEvent (EVENT *theEvent, INT EventMask);

   PARAMETERS:
.  theEvent - pointer to ug event
.  EventMask - determines which events are reported

   DESCRIPTION:
   This function processes an event from the system and passes it to ug if necessary.
   This is a dummy function for NEXTSTEP, OPENSTEP and MacOS X systems, because the
   event handling is done elsewhere.

   RETURN VALUE:
   INT
.n     0 if no event occurred (ug or system)
.n     1 if an event occurred (ug or system).																			
*/																			
/****************************************************************************/

INT GetNextUGEvent (EVENT *theEvent, INT EventMask)
{	
	/* no event as default */
	theEvent->Type = NO_EVENT;
	theEvent->NoEvent.InterfaceEvent = 0;
	
	/* we have no command keys */	
	if (EventMask==TERM_CMDKEY) return 0;
	
	/* read in a string from the user and store it in event structure */
	fgets(theEvent->TermString.String,INPUTBUFFERLEN,stdin);
	theEvent->Type = TERM_STRING;
	
	return 0;
}


/****************************************************************************/
/*																			
   WriteString - write a string to a terminal window													

   SYNOPSIS:
   void WriteString (const char *s);

   PARAMETERS:
.  s - string to write
  
   DESCRIPTION:
   This function writes a string to the shell.
   
   RETURN VALUE:
   void												
*/																			
/****************************************************************************/

void WriteString (const char *s)
{
	fprintf(stdout,"%s",s);
	return;
}


/****************************************************************************/
/*
   MouseStillDown - Determine if mouse button is still pressed


   SYNOPSIS:
   INT MouseStillDown (void);

   PARAMETERS:
   no parameters

   DESCRIPTION:
   This function returns true (1) if the mouse button is still pressed.
   The function should only be called after a button pressed event has been
   reported.


   RETURN VALUE:
   INT
.n 0 if mouse button has been released
.n 1 if mouse button is still pressed
*/
/****************************************************************************/

INT MouseStillDown (void)
{
	printf("MouseStillDown was called\n");
	return (0);
}

void DrawInfoBox (WINDOWID win, const char *info)
{
	printf("DrawInfoBox called\n");
	return;
}

INT WhichTool (WINDOWID win, const INT mouse[2], INT *tool)
{
	printf("WhichTool called\n");
	return (0);
}



static WINDOWID MacOSXServer_OpenOutput (
	const char *title,						/* title of the window	 		*/
	INT rename,                                     
	INT x, INT y, INT width, INT height,	/* plot rgn in standard coord.	*/
	INT *Global_LL, INT *Global_UR, 		/* global machine coordinates	*/
	INT *Local_LL, INT *Local_UR,			/* local machine coordinates	*/
	INT *error) 							/* error code					*/
{
	GRAPH_WINDOW	*gw;
	NSRect viewRect = NSMakeRect(0, 0, width, height);
	*error = 0;
	
	/* create GRAPH_WINDOW structure and put in list */
	gw = (GRAPH_WINDOW*) GetMem(guiHeap,sizeof(GRAPH_WINDOW),GENERAL_HEAP);
	if (gw==NULL)	{*error=1; return(0);}
	gw->next   = windowList;
	windowList = gw;
	
	gw->currTool = arrowTool;

	gw->theGraphWindow = [[MGraphicWindow alloc]
	            initWithContentRect:viewRect
	            styleMask:  ( NSTitledWindowMask
	                        | NSResizableWindowMask
	                        | NSMiniaturizableWindowMask)
	            backing:	NSBackingStoreBuffered
	            defer:		NO];

	[gw->theGraphWindow setTitle:[NSString stringWithCString:title]];
	[gw->theGraphWindow setFrameOrigin:NSMakePoint(x,y)];
	[gw->theGraphWindow makeKeyAndOrderFront:gw->theGraphWindow];
	
	/* fill global and local lower left and upper right in the devices coordinate system */
	gw->Global_LL[0] = Global_LL[0] = x;
	gw->Global_LL[1] = Global_LL[1] = y+height;
	gw->Global_UR[0] = Global_UR[0] = x+width;
	gw->Global_UR[1] = Global_UR[1] = y;
	
	gw->Local_LL[0] = Local_LL[0] = 0;
	gw->Local_LL[1] = Local_LL[1] = height;
	gw->Local_UR[0] = Local_UR[0] = width;
	gw->Local_UR[1] = Local_UR[1] = 0;

	return((WINDOWID)gw);
}


static INT MacOSXServer_CloseOutput (WINDOWID win)
{
	PRINTDEBUG(dev,1,("MacOSXServer_CloseOutput\n"))
	[((GRAPH_WINDOW *)win)->theGraphWindow close];
	return 0;
}


INT MacOSXServer_ActivateOutput (WINDOWID win)
{
	PRINTDEBUG(dev,1,("MacOSXServer_ActivateOutput\n"))
	currgw = (GRAPH_WINDOW *)(win);

	[currgw->theGraphWindow activateOutput];
	return 0;
}

INT MacOSXServer_UpdateOutput (WINDOWID win, INT tool)
{
	currgw = (GRAPH_WINDOW *)(win);
	PRINTDEBUG(dev,1,("MacOSXServer_UpdateOutput\n"))

	[currgw->theGraphWindow updateOutput];
	return 0;
}

void MousePosition (INT *ScreenPoint)
{
	ScreenPoint[0] = 0.0;
	ScreenPoint[1] = 0.0;
	return;
}

/****************************************************************************/
/*																			
   InitScreen - Init rest of GUI and return pointer to screen outputdevice													

   SYNOPSIS:
   OUTPUTDEVICE *InitScreen (int *argcp, char **argv, INT *error);

   PARAMETERS:
.  argcp - pointer to argument counter
.  argv  - argument vector
.  error - errorcode

   DESCRIPTION:
   Do setup for gui and return pointer to screen outputdevice. 
   
   RETURN VALUE:
   OUTPUTDEVICE *
.n      POINTER if all is o.k.
.n      NULL if an error occurred. 

*/																			
/****************************************************************************/

OUTPUTDEVICE *InitScreen (int *argcp, char **argv, INT *error)
{
	char buffer[256];
	int i,j;
	unsigned short res,delta,max,r,g,b;
	int scrollback;
	int charsperline;
	int fontNum;
	int fontSize;
	int TermWinH=0,TermWinV=0,TermWinDH=400,TermWinDV=300;
	
	/*
	 * Get some default values
	 */
	if (GetDefaultValue(DEFAULTSFILENAME,"scrollback",buffer)==0)
		sscanf(buffer," %d ",&scrollback);
	if (GetDefaultValue(DEFAULTSFILENAME,"charsperline",buffer)==0)
		sscanf(buffer," %d ",&charsperline);
	if (GetDefaultValue(DEFAULTSFILENAME,"size",buffer)==0)
		sscanf(buffer," %d ",&fontSize);
	if (GetDefaultValue(DEFAULTSFILENAME,"font",buffer)==0)
	{
		if (strcmp(buffer,"Ohlfs")==0) 	fontNum = 0;
	}
	
	/* read default size of shell window */
	if (GetDefaultValue(DEFAULTSFILENAME,"TermWinH",buffer)==0)
		sscanf(buffer," %d ",&TermWinH);
	if (GetDefaultValue(DEFAULTSFILENAME,"TermWinV",buffer)==0)
		sscanf(buffer," %d ",&TermWinV);
	if (GetDefaultValue(DEFAULTSFILENAME,"TermWinDH",buffer)==0)
		sscanf(buffer," %d ",&TermWinDH);
	if (GetDefaultValue(DEFAULTSFILENAME,"TermWinDV",buffer)==0)
		sscanf(buffer," %d ",&TermWinDV);
	
	pool = [[NSAutoreleasePool alloc] init];
	[NSApplication sharedApplication];
	
	/* create output device */
	if ((MacOSXServerOutputDevice=CreateOutputDevice("screen"))==NULL) 
	{	
		fprintf(stderr,"InitScreen: Couldn't create output device screen\n");
		return(NULL);
	}
	
	/* init output device 'MacOSXServer' */
	MacOSXServerOutputDevice->OpenOutput	= MacOSXServer_OpenOutput;
	MacOSXServerOutputDevice->CloseOutput	= MacOSXServer_CloseOutput;
	MacOSXServerOutputDevice->ActivateOutput= MacOSXServer_ActivateOutput;
	MacOSXServerOutputDevice->UpdateOutput	= MacOSXServer_UpdateOutput;
	MacOSXServerOutputDevice->v.locked		= 1;
	

	/*
	 * Now do a lot of setup 
	 * (connect drawing functions, do color initialization, ...)
	 */
	
	res = 31;
	delta = 2048;
	max = 63488;

	/* fixed colors */
	i = 0;
	colorTable[i].red = 0xFFFF;	colorTable[i].green = 0xFFFF; colorTable[i++].blue = 0xFFFF;
	colorTable[i].red = 0xD000; colorTable[i].green = 0xD000; colorTable[i++].blue = 0xD000;
	colorTable[i].red = 0xFFFF; colorTable[i].green = 0xFFFF; colorTable[i++].blue = 0x0   ;
	colorTable[i].red = 0xFFFF; colorTable[i].green = 0x0	; colorTable[i++].blue = 0xFFFF;
	colorTable[i].red = 0xFFFF; colorTable[i].green = 0x0	; colorTable[i++].blue = 0x0   ;
	colorTable[i].red = 0x0;	colorTable[i].green = 0xFFFF; colorTable[i++].blue = 0xFFFF;
	colorTable[i].red = 0x0;	colorTable[i].green = 0xFFFF; colorTable[i++].blue = 0x0   ;
	colorTable[i].red = 0x0;	colorTable[i].green = 0x0	; colorTable[i++].blue = 0xFFFF;
	colorTable[i].red = 0x0;	colorTable[i].green = 0x0	; colorTable[i++].blue = 0x0   ;
	colorTable[i].red = 65520 ; colorTable[i].green = 32240 ; colorTable[i++].blue = 0x0   ;
	colorTable[i].red = 65520 ; colorTable[i].green = 60000 ; colorTable[i++].blue = 0x0   ;

	/* color spectrum */
	/* TODO: This can be done much nicer on a MacOSXServer */
	r = g = 0; b = max;
	colorTable[i].red = r; colorTable[i].green = g; colorTable[i].blue = b; i++;

	/* blue to cyan */
	for (j=0; j<res; j++)
	{
		g += delta;
		colorTable[i].red = r; colorTable[i].green = g; colorTable[i].blue = b; i++;
	}
	/* cyan to green */
	for (j=0; j<res; j++)
	{
		b -= delta;
		colorTable[i].red = r; colorTable[i].green = g; colorTable[i].blue = b; i++;
	}
	/* green to yellow */
	for (j=0; j<res; j++)
	{
		r += delta;
		colorTable[i].red = r; colorTable[i].green = g; colorTable[i].blue = b; i++;
	}
	/* yellow to red */
	for (j=0; j<res; j++)
	{
		g -= delta;
		colorTable[i].red = r; colorTable[i].green = g; colorTable[i].blue = b; i++;
	}

	MacOSXServerOutputDevice->black	= 8;
	MacOSXServerOutputDevice->gray	= 1;
	MacOSXServerOutputDevice->white	= 0;
	MacOSXServerOutputDevice->red	= 4;
	MacOSXServerOutputDevice->green = 6;
	MacOSXServerOutputDevice->blue	= 7;
	MacOSXServerOutputDevice->cyan	= 5;
	MacOSXServerOutputDevice->yellow	= 2;
	MacOSXServerOutputDevice->darkyellow	= 10;
	MacOSXServerOutputDevice->magenta	= 3;
	MacOSXServerOutputDevice->orange	= 9;
	MacOSXServerOutputDevice->hasPalette	= 1;
	MacOSXServerOutputDevice->range = i;
	MacOSXServerOutputDevice->spectrumStart = 11;
	MacOSXServerOutputDevice->spectrumEnd = i-1;

	if (i>0) printf("Using color map with %d entries\n",i);

	MacOSXServerOutputDevice->signx		 		= 1;
	MacOSXServerOutputDevice->signy		 		= -1;
	
	/* init pointers to basic drawing functions */
	MacOSXServerOutputDevice->Move				= MacOSXServerMove;
	MacOSXServerOutputDevice->Draw				= MacOSXServerDraw;
	MacOSXServerOutputDevice->Polyline			= MacOSXServerPolyline;
	MacOSXServerOutputDevice->InversePolyline	= MacOSXServerInversePolyline;
	MacOSXServerOutputDevice->Polygon			= MacOSXServerPolygon;
	MacOSXServerOutputDevice->ShadedPolygon		= MacOSXServerShadedPolygon;
	MacOSXServerOutputDevice->InversePolygon 	= MacOSXServerInversePolygon;
	MacOSXServerOutputDevice->ErasePolygon		= MacOSXServerErasePolygon;
	MacOSXServerOutputDevice->Polymark			= MacOSXServerPolymark;
	MacOSXServerOutputDevice->InvPolymark		= MacOSXServerInvPolymark;
	MacOSXServerOutputDevice->DrawText			= MacOSXServerDrawText;
	MacOSXServerOutputDevice->CenteredText		= MacOSXServerCenteredText;
	MacOSXServerOutputDevice->ClearViewPort		= MacOSXServerClearViewPort; 					
	
	/* init pointers to set functions */
	MacOSXServerOutputDevice->SetLineWidth		= MacOSXServerSetLineWidth;
	MacOSXServerOutputDevice->SetTextSize		= MacOSXServerSetTextSize;
	MacOSXServerOutputDevice->SetMarker			= MacOSXServerSetMarker;
	MacOSXServerOutputDevice->SetMarkerSize		= MacOSXServerSetMarkerSize;
	MacOSXServerOutputDevice->SetColor			= MacOSXServerSetColor;
	MacOSXServerOutputDevice->SetPaletteEntry	= MacOSXServerSetPaletteEntry;
	MacOSXServerOutputDevice->SetNewPalette		= MacOSXServerSetNewPalette;
	
	/* init pointers to miscellaneous functions */
	MacOSXServerOutputDevice->GetPaletteEntry	= MacOSXServerGetPaletteEntry;
	MacOSXServerOutputDevice->Flush				= MacOSXServerFlush;
	MacOSXServerOutputDevice->PlotPixelBuffer	= NULL;

	printf("output device 'screen' for ");
	printf(ARCHNAME);
	printf(" window manager created\n");

	/* get gui heapsize */
	if (GetDefaultValue(DEFAULTSFILENAME,"guimemory",buffer)==0)
		sscanf(buffer," %ld ",&guiHeapSize);

	/* allocate gui heap */
	if ((guiHeap=NewHeap(GENERAL_HEAP,guiHeapSize,malloc(guiHeapSize)))==NULL)
		return(NULL);
		
	return (MacOSXServerOutputDevice);

	*error=0;
	return(NULL);
}


void ExitScreen (void)
{
	// what is a good way to do this?  If we call [pool release] here, we
	// exit with a segfault.  I think that the process releases its autorelease 
	// pools when it quits. So it should be okay.
	
	//[pool release];
}
