// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  ugm.h                                                                                                                 */
/*																			*/
/* Purpose:   unstructured grid manager header file                                             */
/*			  internal interface in grid manager module                                     */
/*																			*/
/* Author:	  Peter Bastian                                                                                                 */
/*			  Interdisziplinaeres Zentrum fuer Wissenschaftliches Rechnen	*/
/*			  Universitaet Heidelberg										*/
/*			  Im Neuenheimer Feld 368										*/
/*			  6900 Heidelberg												*/
/*			  internet: ug@ica3.uni-stuttgart.de			                        */
/*																			*/
/* History:   09.03.92 begin, ug version 2.0								*/
/*			  13.12.94 begin, ug version 3.0								*/
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

#ifndef __UGM__
#define __UGM__

#include "compiler.h"
#include "gm.h"
#include "dlmgr.h"

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

#define MAX_PAR_DIST    1.0E-6          /* max.dist between different parameter */

#ifndef DYNAMIC_MEMORY_ALLOCMODEL
        #ifdef ModelP
        #define PutFreeObject(theMG,object,size,type) PutFreeObject_par(MGHEAP(theMG),(object),(size),(type))
        #define GetMemoryForObject(theMG,size,type) GetMemoryForObject_par(MGHEAP(theMG),(size),(type))
        #else
        #define GetMemoryForObject(theMG,size,type) GetFreelistMemory(MGHEAP(theMG),(size))
        #define PutFreeObject(theMG,object,size,type) PutFreelistMemory(MGHEAP(theMG),(object),(size))
        #endif
#else
        #define GetMemoryForObject(theMG,size,type) GetMemoryForObjectNew(MGHEAP(theMG),(size),(type))
        #define PutFreeObject(theMG,object,size,type) PutFreeObjectNew(MGHEAP(theMG),(object),(size),(type))
#endif

/****************************************************************************/
/*																			*/
/* data structures exported by the corresponding source file				*/
/*																			*/
/****************************************************************************/


/****************************************************************************/
/*																			*/
/* definition of exported global variables									*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* function declarations													*/
/*																			*/
/****************************************************************************/

/* init */
INT              InitUGManager                  (void);

/* object handling */
INT              GetFreeOBJT                    (void);
INT              ReleaseOBJT                    (INT type);

/* create basic objects */
#ifdef ModelP
EDGE *CreateEdge (GRID *theGrid, ELEMENT *theElement, INT i, INT with_vector);
#endif
ELEMENT     *CreateElement          (GRID *theGrid, INT tag, INT objtype,
                                     NODE **nodes, ELEMENT *Father, INT with_vector);
INT         CreateSonElementSide    (GRID *theGrid, ELEMENT *theElement,
                                     INT side, ELEMENT *theSon, INT son_side);

GRID            *CreateNewLevel                 (MULTIGRID *theMG, INT algebraic);
GRID        *CreateNewLevelAMG      (MULTIGRID *theMG);

/* dispose basic objects */
INT              DisposeElement                 (GRID *theGrid, ELEMENT *theElement, INT dispose_connections);
INT              DisposeTopLevel                (MULTIGRID *theMG);
INT              DisposeNode                    (GRID *theGrid, NODE *theNode);

/* miscellaneous */
ELEMENT     *FindFather             (VERTEX *vptr);
INT              FindNeighborElement    (const ELEMENT *theElement, INT Side, ELEMENT **theNeighbor, INT *NeighborSide);
INT              PointInElement                 (const DOUBLE*, const ELEMENT *theElement);
INT          PointOnSide            (const DOUBLE *global, const ELEMENT *theElement, INT side);
DOUBLE       DistanceFromSide       (const DOUBLE *global, const ELEMENT *theElement, INT side);
VIRT_HEAP_MGMT *GetGenMGUDM             (void);
INT             CheckOrientation                (INT n, VERTEX **vertices);
INT             CheckOrientationInGrid  (GRID *theGrid);

NODE        *CreateSonNode          (GRID *theGrid, NODE *FatherNode);
NODE            *CreateMidNode                  (GRID *theGrid, ELEMENT *theElement, VERTEX *theVertex, INT edge);
NODE            *GetCenterNode                  (ELEMENT *theElement);
NODE        *CreateCenterNode       (GRID *theGrid, ELEMENT *theElement, VERTEX *theVertex);

#ifdef __THREEDIM__
NODE            *CreateSideNode                 (GRID *theGrid, ELEMENT *theElement, VERTEX *theVertex, INT side);
NODE            *GetSideNode                    (ELEMENT *theElement, INT side);
#endif
INT          GetSideIDFromScratch   (ELEMENT *theElement, NODE *theNode);
NODE        *GetMidNode             (ELEMENT *theElement, INT edge);
INT                     GetNodeContext                  (ELEMENT *theElement, NODE **theElementContext);
void            GetNbSideByNodes                (ELEMENT *theNeighbor, INT *nbside, ELEMENT *theElement, INT side);


#if defined(ModelP) && !defined(DYNAMIC_MEMORY_ALLOCMODEL)
void *GetMemoryForObject_par (HEAP *theHeap, INT size, INT type);
INT PutFreeObject_par (HEAP *theHeap, void *object, INT size, INT type);
#endif

#ifdef DYNAMIC_MEMORY_ALLOCMODEL
void *GetMemoryForObjectNew (HEAP *theHeap, INT size, INT type);
INT PutFreeObjectNew (HEAP *theHeap, void *object, INT size, INT type);

/* determination of node classes */
INT             ClearNodeClasses                        (GRID *theGrid);
INT             SeedNodeClasses                     (ELEMENT *theElement);
INT             PropagateNodeClasses            (GRID *theGrid);
INT             ClearNextNodeClasses            (GRID *theGrid);
INT             SeedNextNodeClasses             (ELEMENT *theElement);
INT             PropagateNextNodeClasses        (GRID *theGrid);
INT             MaxNextNodeClass                        (ELEMENT *theElement);
INT             MinNodeClass                            (ELEMENT *theElement);
INT             MinNextNodeClass                        (ELEMENT *theElement);
#endif

#ifdef __PERIODIC_BOUNDARY__
INT             MG_GeometricToPeriodic          (MULTIGRID *mg, INT fl, INT tl);
INT                     Grid_GeometricToPeriodic        (GRID *g);
INT                     MGSetPerVecCount                        (MULTIGRID *mg);
INT                     GridSetPerVecCount                      (GRID *g);
INT                     SetPerVecVOBJECT                        (GRID *g);
INT                     Grid_CheckPeriodicity           (GRID *g);
#endif

#endif
