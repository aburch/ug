// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      tecplot.h                                                     */
/*                                                                          */
/* Purpose:   tecplot output header file	                                                */
/*                                                                          */
/* Author:	  Peter Bastian                                                                                     */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  email: peter@ica3.uni-stuttgart.de							*/
/*			  fon: 0049-(0)711-685-7003										*/
/*			  fax: 0049-(0)711-685-7000										*/
/*																			*/
/* History:   29.06.95 begin, ug version 3.0								*/
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __AVS__
#define __AVS__

#ifndef __GM__
#include "gm.h"
#endif

#ifndef __NUMERICS__
#include "num.h"
#endif


/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* data structures exported by the corresponding source file                */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/

/* exports only the init function */
INT InitAVS (void);

#endif
