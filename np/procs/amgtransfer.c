// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      amgtransfer.c                                                 */
/*                                                                          */
/* Purpose:   algebraic multigrid numproc                                           */
/*                                                                          */
/* Author:	  Nicolas Neuss                                                                                     */
/*			  Institut fuer Angewandte Mathematik                           */
/*			  Universitaet Heidelberg										*/
/*			  Im Neuenheimer Feld 294										*/
/*			  69120 Heidelberg												*/
/*			  email: neuss@iwr.uni-heidelberg.de			                        */
/*																			*/
/* History:   1994-1995 in old ug2.0							            */
/*            May-June 1997 in new ug3.7                                    */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* include files                                                            */
/*            system include files                                          */
/*            application include files                                     */
/*                                                                          */
/****************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "devices.h"
#include "disctools.h"
#include "evm.h"
#include "general.h"
#include "gm.h"
#include "np.h"
#include "pcr.h"
#include "quadrature.h"
#include "shapes.h"
#include "ugm.h"
#include "ugstruct.h"

#include "amgtools.h"
#include "amgtransfer.h"

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
/* data structures used in this source file (exported data structures are   */
/*        in the corresponding include file!)                               */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/

REP_ERR_FILE;

/* RCS string */
static char RCS_ID("$Header$",UG_RCS_STRING);

/****************************************************************************/
/*D
   NP_AMG_TRANSFER - type definition for coarsening algorithms

   DESCRIPTION:
   This numproc type is used for the configuration of AMG algorithms.

   Initializing can be done with

   'INT NPAMGTransferInit (NP_ERROR *theNP, INT argc , char **argv);'

   This routine returns 'EXECUTABLE' if the initizialization is complete
   and  'ACTIVE' else.
   The data can be displayed and the num proc can be executed by

   'INT NPAMGTransferDisplay (NP_ERROR *theNP);'
   'INT NPAMGTransferExecute (NP_BASE *theNP, INT argc , char **argv);'

   .vb
   typedef INT (*MarkConnectionsProcPtr) (GRID *, MATDATA_DESC *, DOUBLE);
   typedef INT (*CoarsenProcPtr)         (GRID *);
   typedef INT (*ComputeIRMatProcPtr)    (GRID *, MATDATA_DESC *, MATDATA_DESC *);
   typedef INT (*SetupCGMatProcPtr)      (GRID *, MATDATA_DESC *, MATDATA_DESC *);

   typedef struct
   {
        NP_TRANSFER transfer;
        INT display;

        MarkConnectionsProcPtr *MarkStrong;
        DOUBLE thetaS;

        CoarsenProcPtr Coarsen;

        MATDATA_DESC *I;
        SetupIRMatProcPtr SetupIR;

        SetupCGMatProcPtr SetupCG;

        MarkConnectionsProcPtr *MarkKeep;
        DOUBLE thetaK;

        INT vectLimit;
        INT matLimit;
        DOUBLE bandLimit;
        DOUBLE vRedLimit;
        DOUBLE mRedLimit;
        INT levelLimit;

        INT explicitFlag;

   } NP_AMG_TRANSFER;
   .ve

   SEE ALSO:
   num_proc
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   AMG - numprocs for AMG transfer
   .   selectionAMG - num proc for selection AMG transfer
   .   clusterAMG - num proc for cluster AMG transfer

   DESCRIPTION:
   The numproc 'selectionAMG' defines an AMG where the coarsening is done by
   selection. A prototype is the Ruge-Stueben algorithm described
   in

   'J.~W.~Ruge,~K.~Stueben: Algebraic~Multigrid'

   in

   'S.~F.~McCormick (editor): Multigrid~methods
        SIAM Philadelphia, Pennsylvania (1987)'

   The numproc 'clusterAMG' defines an AMG where the coarsening is done by
   clustering. A prototype is the algorithm described by Vanek, et al. (1994)
   (FTP from tiger.cudenver.edu).

   Some information about both algorithms and a combination
   of both you may find also my thesis

   'Nicolas Neuss: Homogenisierung und Mehrgitter'

   which can be obtained as ICA-Preprint 1996-07 (also via WWW).

   FORMAT:

   .vb
   npcreate <name> $c {selectionAMG|clusterAMG};
   npinit <name> {$strongAll|$strongAbs <thetaS>|$strongRel <thetaS>}
                {$I {Average|RugeStueben} | $I {PiecewiseConstant|Vanek}}
                [{$keepAbs <thetaS>|$keepRel <thetaS>} [$lump]]
                                [{$coarsefine|$finecoarse}]
                            [$display {full|red|no}]
                                [$vectLimit] [$matLimit] [$bandLimit]
                                [$vRedLimit] [$mRedLimit]
                                [$levelLimit]
                                [$explicit]
                                [$hold];
   .ve

   .  $strong... - defines strong connection type for coarsening
   .  $I - specifies the coarsening
   .     - possibilities for selectionAMG: Average, RugeStueben
   .     - possibilities for clusterAMG: PiecewiseConstant, Vanek
   .  $keep... - defines strong connections to keep when sparsening the CG matrix
   .  $lump... - lump omitted element to diagonal when sparsening the CG matrix
   .  $coarsefine - reorder the fine grid points to first coarse, then fine
   .  $finecoarse - reorder the fine grid points to first fine, then coarse
   .  $display - display modus
   .  $vectLimit - stop if vects<=vectLimit
   .  $matLimit - stop if matrices<=matLimit
   .  $bandLimit - stop if matrices/vects>bandLimit
   .  $vRedLimit - stop if vectReduction<vRedLimit
   .  $mRedLimit - stop if matReduction<mRedLimit
   .  $levelLimit - stop if level<=levelLimit (numbers<=0)
   .  $explicit - clear AMG levels only by npexecute
   .  $hold - holds AMG levels after solving

   USE:
   Usually one will apply this transfer procedure
   as a coarse grid solver (in the moment only on level 0)
   inside a usual MG cycle. Then building and removing of the AMG
   grids are done automatically with pre/postprocess of the solver.
   Mainly for debugging purposes you may keep the AMG grids after
   solving by using the $hold-option.

   Alternatively, if the $explicit-option has been given,
   you can building and removing of the AMG grids
   explicitly (without solving) by
   .vb
   npexecute <name> [$i] [$p];
   .ve

   .  $i - preprocess, rebuilds AMG levels
   .  $p - postprocess, clear AMG levels

   APPLICABILITY:
   All schemes are applicable to (multi-)linear FE discretizations of
   diffusion equations. Up to now, for selectionAMG only the averaging
   interpolation and for clusterAMG the piecewise constant interpolation
   are applicable to systems. Applicability does not mean good
   convergence! Up to now there are no AMG schemes which are good
   for large classes of systems and the averaging technique as well as
   the piecewise constant interpolation do not share
   the robustness properties of the RugeStueben/Vanek interpolation
   with respect to singular perturbations like strong convection,
   strong anisotropy and large jumps in the diffusion coefficient.

   MORE~DETAILS:
   For access in scripts the result of an AMG coarsening is stored in the
   structure ':amg' in the form
   .  blevel - bottom level
   .  nvect0, nvect1, ... - number of vectors, 'nvect<n>' is number of vects on level -<n>
   .  ncon0, ncon1, ... - number of connections
   .  nimat0, nimat1, ... - number of interpolation matrices from level -<n>+1 to -<n>
   Please note that these variables are not removed, so you must check
   ':amg:blevel' to see how many of them are valid.
   D*/
/****************************************************************************/

INT AMGTransferInit (NP_BASE *theNP, INT argc , char **argv)
{
  INT i;
  NP_AMG_TRANSFER *np;
  char buffer[VALUELEN];

  np = (NP_AMG_TRANSFER *) theNP;

  np->transfer.baselevel=0;       /* is only used as return value */

  /* definition of strong criterion, must be set */
  np->MarkStrong=NULL;
  np->thetaS = 0.0;
  if (ReadArgvOption("strongAll",argc,argv)==1)
  {
    np->MarkStrong=MarkOffDiagWithoutDirichlet;
    np->thetaS=0.0;
  }
  if (ReadArgvDOUBLE("strongAbs",&(np->thetaS),argc,argv)==0)
  {
    if (np->MarkStrong==NULL)
      np->MarkStrong=MarkAbsolute;
    else
    {
      PrintErrorMessage('E',"NPAMGTransferInit","conflicting $strong... definition");
      REP_ERR_RETURN(NP_NOT_ACTIVE);
    }
  }
  if (ReadArgvDOUBLE("strongRel",&(np->thetaS),argc,argv)==0)
  {
    if (np->MarkStrong==NULL)
      np->MarkStrong=MarkRelative;
    else
    {
      PrintErrorMessage('E',"NPAMGTransferInit","conflicting $strong... definition");
      REP_ERR_RETURN(NP_NOT_ACTIVE);
    }
  }
  /* specification of coarsen procedure */
  if (ReadArgvChar("C",buffer,argc,argv) == 1) {
    PrintErrorMessage('E',"NPAMGTransferInit","no $C ... definition");
    REP_ERR_RETURN(NP_NOT_ACTIVE);
  }
  np->Coarsen = NULL;
  if (np->AMGtype==SELECTION_AMG)
  {
    if (strcmp(buffer,"Average") == 0)
      np->Coarsen = CoarsenAverage;
    if (strcmp(buffer,"RugeStueben") == 0)
      np->Coarsen = CoarsenRugeStueben;
  }
  else if (np->AMGtype==CLUSTER_AMG)
  {
    if (strcmp(buffer,"VanekNeuss") == 0)
      np->Coarsen = CoarsenVanek;
  }
  if (np->Coarsen==NULL) {
    PrintErrorMessage('E',"NPAMGTransferInit",
                      "$C ... definition is incorrect");
    REP_ERR_RETURN(NP_NOT_ACTIVE);
  }

  /* specification of interpolation procedure */
  if (ReadArgvChar("I",buffer,argc,argv) == 1) {
    PrintErrorMessage('E',"NPAMGTransferInit","no $I ... definition");
    REP_ERR_RETURN(NP_NOT_ACTIVE);
  }
  np->SetupIR = NULL;
  if (np->AMGtype==SELECTION_AMG)
  {
    if (strcmp(buffer,"Average") == 0)
      np->SetupIR = IpAverage;
    if (strcmp(buffer,"RugeStueben") == 0)
      np->SetupIR = IpRugeStueben;
  }
  else if (np->AMGtype==CLUSTER_AMG)
  {
    if (strcmp(buffer,"PiecewiseConstant") == 0)
      np->SetupIR = IpPiecewiseConstant;
    if (strcmp(buffer,"Vanek") == 0)
      np->SetupIR = IpVanek;
  }
  if (np->SetupIR==NULL) {
    PrintErrorMessage('E',"NPAMGTransferInit","$I ... definition is incorrect");
    REP_ERR_RETURN(NP_NOT_ACTIVE);
  }

  /* definition of sparsen criterion, default = no sparsening, keep all */
  np->MarkKeep=NULL;
  np->thetaK = 0.0;
  np->sparsenFlag=0;
  if (ReadArgvDOUBLE("keepAbs",&(np->thetaK),argc,argv)==0)
  {
    if (np->MarkKeep==NULL)
      np->MarkKeep=MarkAbsolute;
    else
    {
      PrintErrorMessage('E',"NPAMGTransferInit","conflicting $keep... definition");
      REP_ERR_RETURN(NP_NOT_ACTIVE);
    }
    if (ReadArgvOption("lump",argc,argv)==1)
      np->sparsenFlag=1;
  }

  if (ReadArgvDOUBLE("keepRel",&(np->thetaK),argc,argv)==0)
  {
    if (np->MarkKeep==NULL)
      np->MarkKeep=MarkRelative;
    else
    {
      PrintErrorMessage('E',"NPAMGTransferInit","conflicting $keep... definition");
      REP_ERR_RETURN(NP_NOT_ACTIVE);
    }
    if (ReadArgvOption("lump",argc,argv)==1)
      np->sparsenFlag=1;
  }

  np->reorderFlag=0;
  if (ReadArgvOption("coarsefine",argc,argv)==1)
    np->reorderFlag=COARSEFINE;
  if (ReadArgvOption("finecoarse",argc,argv)==1)
    np->reorderFlag=FINECOARSE;

  /* read stopping criteria */
  np->vectLimit=0;
  ReadArgvINT("vectLimit",&(np->vectLimit),argc,argv);

  np->matLimit=0;
  ReadArgvINT("matLimit",&(np->matLimit),argc,argv);

  np->bandLimit=0.0;
  ReadArgvDOUBLE("bandLimit",&(np->bandLimit),argc,argv);

  np->vRedLimit=0.0;
  ReadArgvDOUBLE("vRedLimit",&(np->vRedLimit),argc,argv);

  np->mRedLimit=0.0;
  ReadArgvDOUBLE("mRedLimit",&(np->mRedLimit),argc,argv);

  np->levelLimit=-MAXLEVEL;
  ReadArgvINT("levelLimit",&(np->levelLimit),argc,argv);
  if (np->levelLimit<-MAXLEVEL)
  {
    PrintErrorMessage('E',"NPAMGTransferInit","$levelLimit too small...");
    REP_ERR_RETURN(NP_NOT_ACTIVE);
  }

  np->display = ReadArgvDisplay(argc,argv);

  if (ReadArgvOption("explicit",argc,argv))
    np->explicitFlag=1;
  else
    np->explicitFlag=0;
  np->symmetric = ReadArgvOption("symmetric",argc,argv);

  if (ReadArgvOption("hold",argc,argv)==1)
    np->hold=1;

  /* finally the usual TRANSFER data */
  if (sc_read(np->transfer.damp,np->transfer.x,"damp",argc,argv))
    for (i=0; i<MAX_VEC_COMP; i++)
      np->transfer.damp[i] = 1.0;

  np->transfer.A = ReadArgvMatDesc(np->transfer.base.mg,"A",argc,argv);
  np->transfer.x = ReadArgvVecDesc(np->transfer.base.mg,"x",argc,argv);
  np->transfer.b = ReadArgvVecDesc(np->transfer.base.mg,"b",argc,argv);

  if ((np->transfer.A!=NULL)&&(np->transfer.x!=NULL)&&(np->transfer.b!=NULL))
    return(NP_EXECUTABLE);
  else
    return(NP_ACTIVE);
}


INT AMGTransferDisplay (NP_BASE *theNP)
{
  NP_AMG_TRANSFER *np;

  np = (NP_AMG_TRANSFER *) theNP;

  UserWrite("Symbolic user data:\n");
  if (np->transfer.A != NULL)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"A",ENVITEM_NAME(np->transfer.A));
  if (np->transfer.b != NULL)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"b",ENVITEM_NAME(np->transfer.b));
  if (np->transfer.x != NULL)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"x",ENVITEM_NAME(np->transfer.x));

  UserWrite("\nConfiguration parameters:\n");
  UserWriteF(DISPLAY_NP_FORMAT_SI,"baselevel",(int)np->transfer.baselevel);
  if (sc_disp(np->transfer.damp,np->transfer.b,"damp"))
    REP_ERR_RETURN (1);

  if (np->display == PCR_NO_DISPLAY)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"display","NO_DISPLAY");
  else if (np->display == PCR_RED_DISPLAY)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"display","RED_DISPLAY");
  else if (np->display == PCR_FULL_DISPLAY)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"display","FULL_DISPLAY");

  if (np->explicitFlag)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"explicit","yes");
  else
    UserWriteF(DISPLAY_NP_FORMAT_SS,"explicit","no");

  UserWrite("\nSpecial AMG parameters:\n");
  if (np->MarkStrong==MarkAll)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"MarkStrong","MarkAll");
  else if (np->MarkStrong==MarkRelative)
  {
    UserWriteF(DISPLAY_NP_FORMAT_SS,"MarkStrong","MarkRelative");
    UserWriteF(DISPLAY_NP_FORMAT_SF,"thetaS",np->thetaS);
  }
  else if (np->MarkStrong==MarkAbsolute)
  {
    UserWriteF(DISPLAY_NP_FORMAT_SS,"MarkStrong","MarkAbsolute");
    UserWriteF(DISPLAY_NP_FORMAT_SF,"thetaS",np->thetaS);
  }
  else
    UserWriteF(DISPLAY_NP_FORMAT_SS,"MarkStrong","unknown");

  if (np->Coarsen==CoarsenRugeStueben)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"Coarsen","RugeStueben");
  else if (np->Coarsen==CoarsenVanek)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"Coarsen","Vanek");
  else if (np->Coarsen==CoarsenAverage)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"Coarsen","Average");
  else
    UserWriteF(DISPLAY_NP_FORMAT_SS,"Coarsen","unknown");

  if (np->SetupIR==IpRugeStueben)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"SetupIR","RugeStueben");
  else if (np->SetupIR==IpAverage)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"SetupIR","Average");
  else if (np->SetupIR==IpPiecewiseConstant)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"SetupIR","PiecewiseConstant");
  else if (np->SetupIR==IpVanek)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"SetupIR","Vanek");
  else
    UserWriteF(DISPLAY_NP_FORMAT_SS,"SetupIR","unknown");

  if (np->SetupCG==GalerkinCGMatrixFromInterpolation)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"SetupCG","Galerkin");
  else
    UserWriteF(DISPLAY_NP_FORMAT_SS,"SetupCG","AssembleGalerkin");

  if (np->MarkKeep==NULL)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"MarkKeep","NULL (keep all)");
  else if (np->MarkKeep==MarkRelative)
  {
    UserWriteF(DISPLAY_NP_FORMAT_SS,"MarkKeep","MarkRelative");
    UserWriteF(DISPLAY_NP_FORMAT_SF,"thetaK",(float)np->thetaK);
  }
  else if (np->MarkKeep==MarkAbsolute)
  {
    UserWriteF(DISPLAY_NP_FORMAT_SS,"MarkKeep","MarkAbsolute");
    UserWriteF(DISPLAY_NP_FORMAT_SF,"thetaK",(float)np->thetaK);
  }
  else
    UserWriteF(DISPLAY_NP_FORMAT_SS,"MarkKeep","unknown");

  if (np->reorderFlag==0)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"reorderFlag","keep order");
  else if (np->reorderFlag==COARSEFINE)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"reorderFlag","C/F order");
  else if (np->reorderFlag==FINECOARSE)
    UserWriteF(DISPLAY_NP_FORMAT_SS,"reorderFlag","F/C order");
  else
    UserWriteF(DISPLAY_NP_FORMAT_SS,"reorderFlag","unknown");

  UserWriteF(DISPLAY_NP_FORMAT_SI,"vectLimit",(int)np->vectLimit);
  UserWriteF(DISPLAY_NP_FORMAT_SI,"matLimit",(int)np->matLimit);
  UserWriteF(DISPLAY_NP_FORMAT_SF,"bandLimit",(float)np->bandLimit);
  UserWriteF(DISPLAY_NP_FORMAT_SF,"vRedLimit",(float)np->vRedLimit);
  UserWriteF(DISPLAY_NP_FORMAT_SF,"mRedLimit",(float)np->mRedLimit);
  UserWriteF(DISPLAY_NP_FORMAT_SI,"levelLimit",(int)np->levelLimit);
  UserWriteF(DISPLAY_NP_FORMAT_SI,"hold",(int)np->hold);

  return (0);
}

INT AMGTransferPreProcess (NP_TRANSFER *theNP, INT *fl, INT tl,
                           VECDATA_DESC *x, VECDATA_DESC *b,
                           MATDATA_DESC *A, INT *result)
{
  NP_AMG_TRANSFER *np;
  MULTIGRID *theMG;
  GRID *theGrid,*newGrid;
  INT i,error,level,nVect,nMat;
  char varname[32];
  char text[DISPLAY_WIDTH+4];

  if (tl!=0) {
    PrintErrorMessage('E',"AMGTransferPreProcess",
                      "AMG can only be used on level 0!");
    result[0]=1;
    REP_ERR_RETURN(result[0]);
  }

  theMG = theNP->base.mg;
  np = (NP_AMG_TRANSFER *) theNP;

  /* we do nothing, if levels are to be built up and destroyed
     only by explicit calls of 'npexecute' */
  if (np->explicitFlag!=0) {
    result[0]=0;
    return(0);
  }
  theGrid = GRID_ON_LEVEL(theMG,0);
  if ((theGrid->coarser == NULL) || (np->hold == 0)) {
    /* clear AMG levels */
    if (DisposeAMGLevels(theMG)!=0) {
      PrintErrorMessage('E',"AMGTransferPreProcess",
                        "could not dispose AMG levels");
      result[0]=1;
      REP_ERR_RETURN(result[0]);
    }
    SetStringValue(":amg:blevel",0);
    SetStringValue(":amg:vect0",(double)theGrid->nVector);
    SetStringValue(":amg:con0",(double)theGrid->nCon);
    if (np->display == PCR_FULL_DISPLAY) {
      CenterInPattern(text,DISPLAY_WIDTH,ENVITEM_NAME(np),'*',"\n");
      UserWrite(text);
      UserWrite(DISPLAY_NP_AMG_STRING);
      UserWriteF(DISPLAY_NP_AMG_FORMAT,0,(int)theGrid->nVector,
                 (int)theGrid->nCon,0);
    }
    /* coarsen until criteria are fulfilled */
    while (theMG->bottomLevel>np->levelLimit) {
      level=theMG->bottomLevel;
      theGrid=GRID_ON_LEVEL(theMG,level);
      nVect=theGrid->nVector;
      nMat=2*theGrid->nCon;
      if (np->vectLimit!=0)
        if (nVect<=np->vectLimit)
          break;
      if (np->matLimit!=0)
        if (nMat<=np->matLimit)
          break;
      if (np->bandLimit!=0.0)
        if ((DOUBLE)nMat/(DOUBLE)nVect>np->bandLimit)
          break;
      if (np->MarkStrong != NULL) {
        UnmarkAll(theGrid,NULL,0.0);
        if ((result[0]=(np->MarkStrong)(theGrid,A,np->thetaS))!=0)
          REP_ERR_RETURN(result[0]);
      }
      if ((result[0]=(np->Coarsen)(theGrid))!=0)
        break;
      newGrid=theGrid->coarser;
      if (newGrid==NULL)
        break;
            #ifdef ModelP
      if (a_vector_vecskip(theMG,level-1,level-1,x) != NUM_OK)
        REP_ERR_RETURN(1);
            #endif
      if ((result[0]=(np->SetupIR)(theGrid,A,NULL /*preliminary!*/))!=0)
        REP_ERR_RETURN(result[0]);
      if (AllocMDFromMD(theMG,level-1,level-1,A,&A))
        REP_ERR_RETURN(1);
      if (l_dmatset(GRID_ON_LEVEL(theMG,level-1),A,0.0) != NUM_OK)
        REP_ERR_RETURN(1);
      if (AssembleGalerkinByMatrix(theGrid,A,np->symmetric))
        REP_ERR_RETURN(1);
      if (np->MarkKeep!=NULL) {
        UnmarkAll(newGrid,NULL,0.0);
        if ((result[0]=(np->MarkKeep)(newGrid,A,np->thetaK))!=0)
          REP_ERR_RETURN(result[0]);
        if ((result[0]=SparsenCGMatrix(newGrid,A,np->sparsenFlag))!=0)
          REP_ERR_RETURN(result[0]);
      }
      if (np->reorderFlag!=0) {
        if ((result[0]=ReorderFineGrid(theGrid,np->reorderFlag))!=0)
          REP_ERR_RETURN(result[0]);
        /* it is important to reset the index field accordingly */
        l_setindex(theGrid);
      }
      /* set the index field on the new grid
         (even if the ordering might be changed again) */
      l_setindex(newGrid);
      if (np->display == PCR_FULL_DISPLAY)
        UserWriteF(DISPLAY_NP_AMG_FORMAT,
                   (int)theMG->bottomLevel,(int)newGrid->nVector,
                   (int)newGrid->nCon,(int)theGrid->nIMat);

      sprintf(varname,":amg:vect%d",-theMG->bottomLevel);
      SetStringValue(varname,(double)newGrid->nVector);
      sprintf(varname,":amg:con%d",-theMG->bottomLevel);
      SetStringValue(varname,(double)newGrid->nCon);
      sprintf(varname,":amg:imat%d",1-theMG->bottomLevel);
      SetStringValue(varname,(double)theGrid->nIMat);
      SetStringValue(":amg:blevel",(double)theMG->bottomLevel);

      if (np->vRedLimit!=0.0)
        if ((DOUBLE)newGrid->nVector/(DOUBLE)nVect > np->vRedLimit)
          break;
      if (np->mRedLimit!=0.0)
        if ((DOUBLE)2*newGrid->nCon/(DOUBLE)nVect > np->mRedLimit)
          break;
    }
  }
  else {
    for (level=0; level>theMG->bottomLevel; level--) {
      if (AllocMDFromMD(theMG,level-1,level-1,A,&A))
        REP_ERR_RETURN(1);
      if (l_dmatset(GRID_ON_LEVEL(theMG,level-1),A,0.0) != NUM_OK)
        REP_ERR_RETURN(1);
      if (AssembleGalerkinByMatrix(GRID_ON_LEVEL(theMG,level),
                                   A,np->symmetric))
        REP_ERR_RETURN(1);
    }
  }
  for (level=0; level>= theMG->bottomLevel; level--)
    if (AssembleDirichletBoundary (GRID_ON_LEVEL(theMG,level),A,x,b))
      REP_ERR_RETURN(1);

  /* we set the baselevel for the following cycle!! */
  *fl=theMG->bottomLevel;

  result[0]=0;
  return(0);
}

static INT RestrictDefect (NP_TRANSFER *theNP, INT level,
                           VECDATA_DESC *to, VECDATA_DESC *from,
                           MATDATA_DESC *A, VEC_SCALAR damp,
                           INT *result)
{
  NP_AMG_TRANSFER *np;

  np = (NP_AMG_TRANSFER *) theNP;
  result[0] = RestrictByMatrix(GRID_ON_LEVEL(theNP->base.mg,level),
                               to,from,damp);

  return(result[0]);
}

static INT InterpolateCorrection (NP_TRANSFER *theNP, INT level,
                                  VECDATA_DESC *to, VECDATA_DESC *from,
                                  MATDATA_DESC *A, VEC_SCALAR damp,
                                  INT *result)
{
  NP_AMG_TRANSFER *np;

  np = (NP_AMG_TRANSFER *) theNP;
  result[0] =
    InterpolateCorrectionByMatrix(GRID_ON_LEVEL(theNP->base.mg,level),
                                  to,from,damp);

  return(result[0]);
}


static INT AMGTransferPostProcess (NP_TRANSFER *theNP, INT *fl, INT tl,
                                   VECDATA_DESC *x, VECDATA_DESC *b,
                                   MATDATA_DESC *A, INT *result)
{
  MULTIGRID *theMG;
  NP_AMG_TRANSFER *np;
  INT level;

  result[0]=0;
  np = (NP_AMG_TRANSFER *) theNP;
  theMG = theNP->base.mg;

  for (level=-1; level>=theMG->bottomLevel; level--)
    if (FreeMD(theMG,level,level,A))
      REP_ERR_RETURN(1);

  /* are levels to be built up and destroyed only
     by explicit calls of 'npexecute'? */
  if (np->explicitFlag!=0)
    return(0);
  if (np->hold!=0)
    return(0);

  if (DisposeAMGLevels(theMG) != 0) {
    PrintErrorMessage('E',"AMGTransferPostProcess",
                      "could not dispose AMG levels");
    result[0]=1;
    REP_ERR_RETURN(1);
  }
  *fl=0;

  return(0);
}

INT AMGTransferExecute (NP_BASE *theNP, INT argc , char **argv)
{
  NP_TRANSFER *np;
  NP_AMG_TRANSFER *npa;
  INT result,level;

  if ((level = CURRENTLEVEL(theNP->mg))!=0)
  {
    PrintErrorMessage('E',"AMGTransferExecute",
                      "AMG can only be used on level 0!");
    REP_ERR_RETURN(1);
  }

  np = (NP_TRANSFER *) theNP;
  npa = (NP_AMG_TRANSFER *) theNP;

  if (npa->explicitFlag==0)
  {
    PrintErrorMessage('E',"AMGTransferExecute",
                      "you must set the $explicit-option in npinit!");
    REP_ERR_RETURN(1);
  }

  if (ReadArgvOption("i",argc,argv)) {
    if (np->PreProcess == NULL) {
      PrintErrorMessage('E',"AMGTransferExecute","no PreProcess");
      REP_ERR_RETURN (1);
    }
    if (np->A == NULL) {
      PrintErrorMessage('E',"AMGTransferExecute","no matrix A");
      REP_ERR_RETURN (1);
    }

    npa->explicitFlag=0;
    (*np->PreProcess)(np,&(np->baselevel),level,np->x,np->b,np->A,&result);
    npa->explicitFlag=1;

    if (result) {
      UserWriteF("AMGTransferExecute: PreProcess failed, error code %d\n",
                 result);
      REP_ERR_RETURN (1);
    }
  }

  if (ReadArgvOption("p",argc,argv)) {
    if (np->PostProcess == NULL) {
      PrintErrorMessage('E',"AMGTransferExecute","no PostProcess");
      REP_ERR_RETURN (1);
    }
    if (np->A == NULL) {
      PrintErrorMessage('E',"AMGTransferExecute","no matrix A");
      REP_ERR_RETURN (1);
    }

    npa->explicitFlag=0;
    (*np->PostProcess)(np,&(np->baselevel),level,np->x,np->b,np->A,&result);
    npa->explicitFlag=1;

    if (result) {
      UserWriteF("AMGTransferExecute: PostProcess failed, error code %d\n",
                 result);
      REP_ERR_RETURN (1);
    }
  }

  return(0);
}

static INT AMGTransferConstruct (NP_BASE *theNP)
{
  NP_TRANSFER *np;

  theNP->Init = AMGTransferInit;
  theNP->Display = AMGTransferDisplay;
  theNP->Execute = AMGTransferExecute;

  np = (NP_TRANSFER *) theNP;
  np->PreProcess = AMGTransferPreProcess;
  np->PreProcessProject = NULL;
  np->PreProcessSolution = NULL;
  np->RestrictDefect = RestrictDefect;
  np->InterpolateCorrection = InterpolateCorrection;
  np->InterpolateNewVectors = NULL;
  np->ProjectSolution = NULL;
  np->AdaptCorrection = NULL;
  np->PostProcess = AMGTransferPostProcess;
  np->PostProcessProject = NULL;

  return(0);
}

static INT SelectionAMGConstruct (NP_BASE *theNP)
{
  NP_AMG_TRANSFER *np;

  AMGTransferConstruct(theNP);

  np =(NP_AMG_TRANSFER *) theNP;
  np->AMGtype = SELECTION_AMG;
  np->Coarsen = CoarsenRugeStueben;
  np->SetupIR = IpRugeStueben;
  np->SetupCG = NULL;

  return(0);
}

static INT ClusterAMGConstruct (NP_BASE *theNP)
{
  NP_AMG_TRANSFER *np;

  AMGTransferConstruct(theNP);

  np =(NP_AMG_TRANSFER *) theNP;
  np->AMGtype = CLUSTER_AMG;
  np->Coarsen = CoarsenVanek;
  np->SetupIR = IpVanek;
  np->SetupCG = NULL;

  return(0);
}


/****************************************************************************/
/*
   InitAMGTransfer	- Init this file

   SYNOPSIS:
   INT InitPlotProc ();

   PARAMETERS:
   .  void -

   DESCRIPTION:
   This function inits this file.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
 */
/****************************************************************************/

INT InitAMGTransfer ()
{
  if (CreateClass(TRANSFER_CLASS_NAME ".selectionAMG",
                  sizeof(NP_AMG_TRANSFER),SelectionAMGConstruct))
    return (__LINE__);

  if (CreateClass(TRANSFER_CLASS_NAME ".clusterAMG",
                  sizeof(NP_AMG_TRANSFER),ClusterAMGConstruct))
    return (__LINE__);

  if (MakeStruct(":amg"))
    return (__LINE__);

  return (0);
}
