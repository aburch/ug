// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  assemble.c		                                                                                */
/*																			*/
/* Purpose:   assemble num procs                                                */
/*																			*/
/*																			*/
/* Author:	  Christian Wieners                                                                             */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de						        */
/*																			*/
/* History:   November 29, 1996                                                                         */
/*																			*/
/* Remarks:                                                                                                                             */
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* include files															*/
/*			  system include files											*/
/*			  application include files                                                                     */
/*																			*/
/****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "general.h"
#include "debug.h"
#include "devices.h"
#include "gm.h"
#include "disctools.h"
#include "np.h"

#include "assemble.h"

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* data structures used in this source file (exported data structures are	*/
/*		  in the corresponding include file!)								*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* definition of exported global variables									*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/

static DOUBLE *mat,*sol,*def;
static INT *vecskip;

REP_ERR_FILE;

/* RCS string */
static char RCS_ID("$Header$",UG_RCS_STRING);

/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*D
   assemble - ug offers several classes of assmbling 'num_proc's

   DESCRIPTION:
   The classes defined in ug are:~
   .  NP_ASSEMBLE - type definition for assembling
   .  NP_NL_ASSEMBLE - type definition for nonlinear assembling
   .  NP_LOCAL_ASSEMBLE - type definition for local assembling
   .  NP_T_ASSEMBLE - type definition for time dependent assembling

   For realizations of assembling num_procs (which are defined in the problem
   classes) try typing
   .n     help assemble $k

   SEE ALSO:
   'num_proc', 'NP_ASSEMBLE', 'NP_NL_ASSEMBLE', 'NP_LOCAL_ASSEMBLE', 'NP_T_ASSEMBLE'
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   NP_ASSEMBLE - type definition for assembling

   DESCRIPTION:
   This numproc type is used for the description of assembling.
   It can be called by the given interface from a nonlinear solver.
   Initializing the data is optional; it can be done with

   'INT NPAssembleInit (NP_ASSEMBLE *theNP, INT argc , char **argv);'

   This routine returns 'EXECUTABLE' if the initizialization is complete
   and  'ACTIVE' else.
   The data can be displayed and the num proc can be executed by

   'INT NPAssembleDisplay (NP_ASSEMBLE *theNP);'
   'INT NPAssembleExecute (NP_BASE *theNP, INT argc , char **argv);'

   .vb
   struct np_assemble {

        NP_BASE base;                        // inherits base class

        // data (optinal, necessary for calling the generic execute routine)
    VECDATA_DESC *x;                     // solution
    VECDATA_DESC *b;                     // defect
    MATDATA_DESC *A;                     // matrix

        // functions
        INT (*PreProcess)
             (struct np_assemble *,          // pointer to (derived) object
                  INT,                           // level
                  VECDATA_DESC *,                // solution vector
                  VECDATA_DESC *,                // rhs vector
                  MATDATA_DESC *,                // matrix
                  INT *);                        // result
    INT (*Assemble)
             (struct np_assemble *,          // pointer to (derived) object
                  INT,                           // level
                  VECDATA_DESC *,			     // current solution	(initial)
                  VECDATA_DESC *,			     // right hand side
                  MATDATA_DESC *,                // matrix
                  INT *);                        // result
        INT (*PostProcess)
             (struct np_assemble *,          // pointer to (derived) object
                  INT,                           // level
                  VECDATA_DESC *,                // solution vector
                  VECDATA_DESC *,                // defect vector
                  MATDATA_DESC *,                // matrix
                  INT *);                        // result
   };
   typedef struct np_assemble NP_ASSEMBLE;
   .ve

   SEE ALSO:
   'num_proc', 'NP_NL_ASSEMBLE', 'NP_LOCAL_ASSEMBLE', 'NP_T_ASSEMBLE'
   D*/
/****************************************************************************/

INT NPAssembleInit (NP_BASE *theNP, INT argc , char **argv)
{
  NP_ASSEMBLE *np;

  np = (NP_ASSEMBLE *) theNP;
  np->A = ReadArgvMatDesc(np->base.mg,"A",argc,argv);
  np->x = ReadArgvVecDesc(np->base.mg,"x",argc,argv);
  np->b = ReadArgvVecDesc(np->base.mg,"b",argc,argv);

  if ((np->A == NULL) || (np->b == NULL) || (np->x == NULL))
    return(NP_ACTIVE);

  return(NP_EXECUTABLE);
}

INT NPAssembleDisplay (NP_BASE *theNP)
{
  NP_ASSEMBLE *np;

  np = (NP_ASSEMBLE *) theNP;
  if ((np->A == NULL) && (np->b == NULL) && (np->x == NULL))
    return(0);
  UserWrite("symbolic user data:\n");
  if (np->A != NULL)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"A",ENVITEM_NAME(np->A));
  if (np->b != NULL)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"b",ENVITEM_NAME(np->b));
  if (np->x != NULL)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"x",ENVITEM_NAME(np->x));
  UserWrite("\n");

  return(0);
}

INT NPAssembleExecute (NP_BASE *theNP, INT argc , char **argv)
{
  NP_ASSEMBLE *np;
  INT result,level;

  np = (NP_ASSEMBLE *) theNP;
  level = CURRENTLEVEL(theNP->mg);

  if (np->x == NULL) {
    PrintErrorMessage('E',"NPAssembleExecute","no vector x");
    return (1);
  }
  if (np->b == NULL) {
    PrintErrorMessage('E',"NPAssembleExecute","no vector b");
    return (1);
  }
  if (np->A == NULL) {
    PrintErrorMessage('E',"NPAssembleExecute","no matrix A");
    return (1);
  }

  if (ReadArgvOption("i",argc,argv)) {
    if (np->PreProcess == NULL) {
      PrintErrorMessage('E',"NPAssembleExecute","no PreProcess");
      return (1);
    }
    if ((*np->PreProcess)(np,level,np->x,np->b,np->A,&result)) {
      UserWriteF("NPAssembleExecute: PreProcess failed, error code %d\n",
                 result);
      return (1);
    }
  }

  if (ReadArgvOption("a",argc,argv)) {
    if (np->Assemble == NULL) {
      PrintErrorMessage('E',"NPAssembleExecute","no Assemble");
      return (1);
    }
    if ((*np->Assemble)(np,level,np->x,np->b,np->A,&result)) {
      UserWriteF("NPAssembleExecute: Assemble failed, error code %d\n",
                 result);
      return (1);
    }
  }

  if (ReadArgvOption("p",argc,argv)) {
    if (np->PostProcess == NULL) {
      PrintErrorMessage('E',"NPAssembleExecute","no PostProcess");
      return (1);
    }
    if ((*np->PostProcess)(np,level,np->x,np->b,np->A,&result)) {
      UserWriteF("NPAssembleExecute: PostProcess failed, error code %d\n",
                 result);
      return (1);
    }
  }

  return(0);
}

/****************************************************************************/
/*D
   NP_NL_ASSEMBLE - type definition for nonlinear assembling

   DESCRIPTION:
   This numproc type is used for the description of assembling.
   It can be called by the given interface from a nonlinear solver.
   Initializing the data is optional; it can be done with

   'INT NPNLAssembleInit (NP_BASE *theNP, INT argc , char **argv);'

   This routine returns 'EXECUTABLE' if the initizialization is complete
   and  'ACTIVE' else.
   The data can be displayed and the num proc can be executed by

   'INT NPNLAssembleDisplay (NP_BASE *theNP);'
   'INT NPNLAssembleExecute (NP_BASE *theNP, INT argc , char **argv);'

   .vb
   struct np_nl_assemble {

        NP_BASE base;                        // inherits base class

        // data (optinal, necessary for calling the generic execute routine)
    VECDATA_DESC *x;                     // solution
    VECDATA_DESC *c;                     // correction
    VECDATA_DESC *b;                     // defect
    MATDATA_DESC *A;                     // matrix

        // functions
        INT (*PreProcess)
             (struct np_nl_assemble *,       // pointer to (derived) object
                  INT,                           // from level
                  INT,                           // to level
                  VECDATA_DESC *,                // solution vector
                  INT *);                        // result
    INT (*NLAssembleSolution)
             (struct np_nl_assemble *,       // pointer to (derived) object
                  INT,                           // from level
                  INT,                           // to level
                  VECDATA_DESC *,                // solution vector
                  INT *);                        // result
    INT (*NLAssembleDefect)
             (struct np_nl_assemble *,       // pointer to (derived) object
                  INT,                           // from level
                  INT,                           // to level
                  VECDATA_DESC *,                // solution vector
                  VECDATA_DESC *,                // defect vector
                  MATDATA_DESC *,                // matrix
                  INT *);                        // result
    INT (*NLAssembleMatrix)
             (struct np_nl_assemble *,       // pointer to (derived) object
                  INT,                           // from level
                  INT,                           // to level
                  VECDATA_DESC *,			     // current solution	(initial)
                  VECDATA_DESC *,			     // defect for current solution
                  VECDATA_DESC *,			     // correction to be computed
                  MATDATA_DESC *,                // matrix
                  INT *);                        // result
        INT (*PostProcess)
             (struct np_nl_assemble *,       // pointer to (derived) object
                  INT,                           // from level
                  INT,                           // to level
                  VECDATA_DESC *,                // solution vector
                  VECDATA_DESC *,                // defect vector
                  MATDATA_DESC *,                // matrix
                  INT *);                        // result
   };
   typedef struct np_nl_assemble NP_NL_ASSEMBLE;
   .ve

   SEE ALSO:
   'num_proc', 'NP_ASSEMBLE', 'NP_LOCAL_ASSEMBLE', 'NP_T_ASSEMBLE'
   D*/
/****************************************************************************/

INT NPNLAssembleInit (NP_BASE *theNP, INT argc , char **argv)
{
  NP_NL_ASSEMBLE *np;

  np = (NP_NL_ASSEMBLE *) theNP;
  np->A = ReadArgvMatDesc(np->base.mg,"A",argc,argv);
  np->x = ReadArgvVecDesc(np->base.mg,"x",argc,argv);
  np->c = ReadArgvVecDesc(np->base.mg,"c",argc,argv);
  np->b = ReadArgvVecDesc(np->base.mg,"b",argc,argv);

  if ((np->A == NULL) || (np->b == NULL) || (np->x == NULL))
    return(NP_ACTIVE);

  return(NP_EXECUTABLE);
}

INT NPNLAssembleDisplay (NP_BASE *theNP)
{
  NP_NL_ASSEMBLE *np;

  np = (NP_NL_ASSEMBLE *) theNP;
  if ((np->A == NULL) && (np->b == NULL) && (np->x == NULL))
    return(0);
  UserWrite("symbolic user data:\n");
  if (np->A != NULL)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"A",ENVITEM_NAME(np->A));
  if (np->b != NULL)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"b",ENVITEM_NAME(np->b));
  if (np->x != NULL)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"x",ENVITEM_NAME(np->x));
  if (np->c != NULL)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"c",ENVITEM_NAME(np->x));
  UserWrite("\n");

  return(0);
}

INT NPNLAssembleExecute (NP_BASE *theNP, INT argc , char **argv)
{
  NP_NL_ASSEMBLE *np;
  INT result,level;

  np = (NP_NL_ASSEMBLE *) theNP;
  level = CURRENTLEVEL(theNP->mg);

  if (np->x == NULL) {
    PrintErrorMessage('E',"NPNLAssembleExecute","no vector x");
    return (1);
  }
  if (np->b == NULL) {
    PrintErrorMessage('E',"NPNLAssembleExecute","no vector b");
    return (1);
  }
  if (np->A == NULL) {
    PrintErrorMessage('E',"NPNLAssembleExecute","no matrix A");
    return (1);
  }

  if (ReadArgvOption("i",argc,argv)) {
    if (np->PreProcess == NULL) {
      PrintErrorMessage('E',"NPNLAssembleExecute","no PreProcess");
      return (1);
    }
    if ((*np->PreProcess)(np,0,level,np->x,&result)) {
      UserWriteF("NPNLAssembleExecute: PreProcess failed, error code %d\n",
                 result);
      return (1);
    }
  }

  if (ReadArgvOption("s",argc,argv)) {
    if (np->NLAssembleSolution == NULL) {
      PrintErrorMessage('E',"NPNLAssembleExecute","no NLAssembleSolution");
      return (1);
    }
    if ((*np->NLAssembleSolution)(np,0,level,np->x,&result)) {
      UserWriteF("NPNLAssembleExecute: NLAssembleSolution failed, error code %d\n",
                 result);
      return (1);
    }
  }

  if (ReadArgvOption("d",argc,argv)) {
    if (np->NLAssembleDefect == NULL) {
      PrintErrorMessage('E',"NPNLAssembleExecute","no NLAssembleDefect");
      return (1);
    }
    if ((*np->NLAssembleDefect)(np,0,level,np->x,np->b,np->A,&result)) {
      UserWriteF("NPNLAssembleExecute: NLAssembleDefect failed, error code %d\n",
                 result);
      return (1);
    }
  }

  if (ReadArgvOption("M",argc,argv)) {
    if (np->NLAssembleMatrix == NULL) {
      PrintErrorMessage('E',"NPNLAssembleExecute","no NLAssembleMatrix");
      return (1);
    }
    if ((*np->NLAssembleMatrix)(np,0,level,np->x,np->b,np->c,np->A,&result)) {
      UserWriteF("NPNLAssembleExecute: NLAssembleMatrix failed, error code %d\n",
                 result);
      return (1);
    }
  }

  if (ReadArgvOption("p",argc,argv)) {
    if (np->PostProcess == NULL) {
      PrintErrorMessage('E',"NPNLAssembleExecute","no PostProcess");
      return (1);
    }
    if ((*np->PostProcess)(np,0,level,np->x,np->b,np->A,&result)) {
      UserWriteF("NPNLAssembleExecute: PostProcess failed, error code %d\n",
                 result);
      return (1);
    }
  }

  return(0);
}

/****************************************************************************/
/*D
   NP_LOCAL_ASSEMBLE - type definition for local assembling

   DESCRIPTION:
   This numproc type is used for the description of local assembling.
   It can be called by the given interface from a nonlinear multigrid
   solver.
   Initializing the data is optional; it can with

   'INT NPLocalAssembleInit (NP_LOCAL_ASSEMBLE *theNP, INT argc , char **argv);'

   This routine returns 'EXECUTABLE' if the initizialization is complete
   and  'ACTIVE' else.
   The data can be displayed and the num proc can be executed by

   'INT NPLocalAssembleDisplay (NP_LOCAL_ASSEMBLE *theNP);'
   'INT NPAssembleExecute (NP_BASE *theNP, INT argc , char **argv);'

   The interface functions 'AssemblePreProcess', 'Assemble'
   'AssembleDefect', 'AssembleMatrix' and 'AssemblePostProcess'
   of NP_ASSEMBLE can be constructed by the interface of NP_LOCAL_ASSEMBLE
   by

   'INT LocalAssembleConstruct (NP_ASSEMBLE *theNP);'

   .vb
   struct np_local_assemble {

        NP_ASSEMBLE assemble;                // inherits assemble class

        // data
        INT galerkin;                        // Galerkin assembling

        // functions
        INT (*PreProcess)
             (struct np_local_assemble *,    // pointer to (derived) object
                  INT,                           // level
                  VECDATA_DESC *,                // solution vector
                  VECDATA_DESC *,                // defect vector
                  MATDATA_DESC *,                // matrix
          DOUBLE **,                     // local solution
          DOUBLE **,                     // local defect
          DOUBLE **,                     // local matrix
          INT **,                        // local vecskip
                  INT *);                        // result
    INT (*AssembleLocal)
             (ELEMENT *,                     // pointer to an element
                  INT *);                        // result
    INT (*AssembleLocalDefect)
             (ELEMENT *,                     // pointer to an element
                  INT *);                        // result
    INT (*AssembleLocalMatrix)
             (ELEMENT *,                     // pointer to an element
                  INT *);                        // result
        INT (*PostMatrix)
             (struct np_local_assemble *,    // pointer to (derived) object
                  INT,                           // level
                  VECDATA_DESC *,                // solution vector
                  VECDATA_DESC *,                // defect vector
                  MATDATA_DESC *,                // matrix
                  INT *);                        // result
        INT (*PostProcess)
             (struct np_local_assemble *,    // pointer to (derived) object
                  INT,                           // level
                  VECDATA_DESC *,                // solution vector
                  VECDATA_DESC *,                // defect vector
                  MATDATA_DESC *,                // matrix
                  INT *);                        // result
   };
   typedef struct np_local_assemble NP_LOCAL_ASSEMBLE;
   .ve

   SEE ALSO:
   'num_proc', 'NP_ASSEMBLE', 'NP_NL_ASSEMBLE', 'NP_T_ASSEMBLE'
   D*/
/****************************************************************************/

INT NPLocalAssembleInit (NP_LOCAL_ASSEMBLE *np, INT argc , char **argv)
{
  if (ReadArgvINT("g",&np->galerkin,argc,argv))
    np->galerkin = 0;

  return(NPAssembleInit(&np->assemble.base,argc,argv));
}

INT NPLocalAssembleDisplay (NP_LOCAL_ASSEMBLE *np)
{
  NPAssembleDisplay(&np->assemble.base);

  UserWrite("configuration parameters:\n");
  UserWriteF(DISPLAY_NP_FORMAT_SI,"g",(int)np->galerkin);

  return(0);
}

INT NPLocalAssemblePostMatrix (NP_LOCAL_ASSEMBLE *theNP, INT level,
                               VECDATA_DESC *x,
                               VECDATA_DESC *b, MATDATA_DESC *A, INT *result)
{
  INT lev;

        #ifdef ModelP
  if (a_vector_vecskip(theNP->assemble.base.mg,0,level,x) != NUM_OK)
    return (1);
        #endif

  for (lev=0; lev<=level; lev++)
    AssembleDirichletBoundary(GRID_ON_LEVEL(theNP->assemble.base.mg,lev),
                              A,x,b);
  UserWrite(" [d]");

  return(0);
}

static INT AssemblePreProcess (NP_ASSEMBLE *theNP, INT level, VECDATA_DESC *x,
                               VECDATA_DESC *b, MATDATA_DESC *A, INT *result)
{
  NP_LOCAL_ASSEMBLE *np;

  np = (NP_LOCAL_ASSEMBLE *) theNP;
  if ((*np->PreProcess)(np,level,x,b,A,&sol,&def,&mat,&vecskip,result)) {
    UserWriteF("PreProcess failed, error code %d\n",result[0]);
    return (1);
  }

  return(0);
}

static INT Assemble (NP_ASSEMBLE *theNP, INT level, VECDATA_DESC *x,
                     VECDATA_DESC *b, MATDATA_DESC *A, INT *result)
{
  NP_LOCAL_ASSEMBLE *np;
  MULTIGRID *theMG;
  GRID *theGrid;
  ELEMENT *theElement;
  DOUBLE *mptr[MAX_NODAL_VALUES*MAX_NODAL_VALUES];
  DOUBLE *sptr[MAX_NODAL_VALUES];
  DOUBLE *rptr[MAX_NODAL_VALUES];
  INT i,l,m;

  np = (NP_LOCAL_ASSEMBLE *) theNP;
  theMG = theNP->base.mg;
  for (l=0; l<=level; l++) {
    UserWriteF(" [%d:",l);
    theGrid = GRID_ON_LEVEL(theMG,l);
    if (l_dset(theGrid,b,EVERY_CLASS,0.0)!=NUM_OK) NP_RETURN(1,result[0]);
    if (l_dmatset(theGrid,A,0.0)!=NUM_OK) NP_RETURN(1,result[0]);
    CLEAR_VECSKIP_OF_GRID(theGrid);
    for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL;
         theElement=SUCCE(theElement)) {
      if (np->galerkin)
        if (NSONS(theElement) > 2) continue;
      m = GetElementVVMPtrs(theElement,x,b,A,sptr,rptr,mptr,vecskip);
      for (i=0; i<m; i++) sol[i] = *sptr[i];
      for (i=0; i<m; i++) def[i] = 0.0;
      for (i=0; i<m*m; i++) mat[i] = 0.0;
      if ((*np->AssembleLocal)(theElement,result)) {
        UserWriteF("AssembleLocal failed for element %d, error code %d\n",
                   ID(theElement),result[0]);
        return (1);
      }
      for (i=0; i<m; i++) *rptr[i] += def[i];
      for (i=0; i<m*m; i++) *mptr[i] += mat[i];
      for (i=0; i<m; i++) *sptr[i] = sol[i];
      if (OBJT(theElement) == BEOBJ)
        SetElementDirichletFlags(theElement,x,vecskip);
    }
    UserWrite("a]");
  }
  if (np->PostMatrix != NULL)
    if ((*np->PostMatrix)(np,level,x,b,A,result)) {
      UserWriteF("(PostMatrix failed, error code %d\n",result[0]);
      return (1);
    }
  UserWrite("\n");

  return(0);
}

static INT AssemblePostProcess (NP_ASSEMBLE *theNP, INT level, VECDATA_DESC *x,
                                VECDATA_DESC *b, MATDATA_DESC *A, INT *result)
{
  NP_LOCAL_ASSEMBLE *np;

  np = (NP_LOCAL_ASSEMBLE *) theNP;
  if (np->PostProcess != NULL)
    if ((*np->PostProcess)(np,level,x,b,A,result)) {
      UserWriteF("PostProcess failed, error code %d\n",result[0]);
      return (1);
    }
  UserWrite("\n");

  return(0);
}

INT NPLocalAssembleConstruct (NP_ASSEMBLE *np)
{
  np->PreProcess = AssemblePreProcess;
  np->Assemble = Assemble;
  np->PostProcess = AssemblePostProcess;

  return(0);
}

/****************************************************************************/
/*D
   NP_T_ASSEMBLE - type definition for time dependent assembling

   DESCRIPTION:
   This is the interface for a time dependent problem as it is required
   by the tsolver. An NP_T_ASSEMBLE object is never executable, only its
   functional interface is used.

   .vb
   struct np_t_assemble {

        NP_BASE base;                        // inherits base class

        // functions
        INT (*TAssemblePreProcess)               // call at begin of timestep
             (struct np_t_assemble *,        // pointer to (derived) object
                  INT,                           // from level
                  INT,                           // to level
                  DOUBLE,						 // time t_k+1
                  DOUBLE,						 // time t_k
                  DOUBLE,						 // time t_k-1
                  VECDATA_DESC *,                // (unknown) solution at t_k+1
                  VECDATA_DESC *,                // solution vector at t_k
                  VECDATA_DESC *,                // solution vector at t_k-1
                  INT *);                        // result
    INT (*TAssembleInitial)                      // set initial values
             (struct np_t_assemble *,        // pointer to (derived) object
                  INT,                           // from level
                  INT,                           // to level
                  DOUBLE,						 // time value t
                  VECDATA_DESC *,                // solution vector at time t
                  INT *);                        // result
    INT (*TAssembleSolution)             // set dirichlet conditions in sol.
             (struct np_t_assemble *,        // pointer to (derived) object
                  INT,                           // from level
                  INT,                           // to level
                  DOUBLE,						 // time value t
                  VECDATA_DESC *,                // solution vector at time t
                  INT *);                        // result
    INT (*TAssembleDefect)                   // accumulate to defect vector
             (struct np_t_assemble *,        // pointer to (derived) object
                  INT,                           // from level
                  INT,                           // to level
                  DOUBLE,						 // time value t
                  DOUBLE,						 // scaling for m-term: s_m
                  DOUBLE,						 // scaling for a-term: s_a
                  VECDATA_DESC *,                // solution vector y
                  VECDATA_DESC *,                // accumulate s_m*m(t,y)+s_a*a(t,y)
                  MATDATA_DESC *,                // matrix may be handy for Picard
                  INT *);                        // result
    INT (*TAssembleMatrix)                       // compute linearization (Jacobian)
             (struct np_t_assemble *,        // pointer to (derived) object
                  INT,                           // from level
                  INT,                           // to level
                  DOUBLE,						 // time value t
                  DOUBLE,						 // scaling for a-term: s_a	(s_m=1!)
                  VECDATA_DESC *,			     // current sol (linearization pt)
                  VECDATA_DESC *,			     // defect for current solution
                  VECDATA_DESC *,			     // correction to be computed
                  MATDATA_DESC *,                // matrix
                  INT *);                        // result
        INT (*TAssemblePostProcess)          // call after solution t_k+1 known
             (struct np_t_assemble *,        // pointer to (derived) object
                  INT,                           // from level
                  INT,                           // to level
                  DOUBLE,						 // time t_k+1
                  DOUBLE,						 // time t_k
                  DOUBLE,						 // time t_k-1
                  VECDATA_DESC *,                // solution t_k+1 (just computed!)
                  VECDATA_DESC *,                // solution vector at t_k
                  VECDATA_DESC *,                // solution vector at t_k-1
                  INT *);                        // result
   };
   typedef struct np_t_assemble NP_T_ASSEMBLE;
   .ve

   SEE ALSO:
   'num_proc', 'NP_ASSEMBLE', 'NP_NL_ASSEMBLE', 'NP_LOCAL_ASSEMBLE'
   D*/
/****************************************************************************/

INT NPTAssembleInit (NP_BASE *theNP, INT argc , char **argv)
{
  return(NP_ACTIVE);
}

INT NPTAssembleDisplay (NP_BASE *theNP)
{
  return(0);
}

INT NPTAssembleExecute (NP_BASE *theNP, INT argc , char **argv)
{
  return(1);   /* never executable */
}
