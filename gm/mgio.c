// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  mgio.c														*/
/*																			*/
/* Purpose:   input/output of loc ref mg	                                                                */
/*																			*/
/* Author:	  Klaus Johannsen                                                                                               */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70550 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de								*/
/*																			*/
/* History:   18.11.96 begin,												*/
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bio.h"
#include "mgio.h"

#ifdef __MGIO_USE_IN_UG__

        #include "defaults.h"
        #include "fileopen.h"
        #include "domain.h"

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

#define MGIO_TITLE_LINE                 "####.sparse.mg.storage.format.####"

#define MGIO_INTSIZE                            1000    /* minimal 497 !!!		*/
#define MGIO_DOUBLESIZE                         200
#define MGIO_BUFFERSIZE                         1024

#define MGIO_CHECK_INTSIZE(n)           if ((n)>MGIO_INTSIZE) return (1)
#define MGIO_CHECK_DOUBLESIZE(n)        if ((n)>MGIO_DOUBLESIZE) return (1)
#define MGIO_CHECK_BUFFERIZE(s)         if (strlen(s)>MGIO_BUFFERSIZE) return (1)

#define MGIO_ECTRL(ref,nf,nm,refc)                                      ((nf)&31) | (((nm)&31)<<5)  | (((ref)&262143)<<10) | (((refc)&7)<<28)
#define MGIO_ECTRL_NC(ctrl)                                             ((ctrl)&31)
#define MGIO_ECTRL_NM(ctrl)                                             (((ctrl)>>5)&31)
#define MGIO_ECTRL_REF(ctrl)                                    (((ctrl)>>10)&262143)
#define MGIO_ECTRL_REFCLASS(ctrl)                               (((ctrl)>>28)&7)

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

static FILE *stream;                            /* file                                                 */
static int mgpathes_set;                        /* pathes used in ug			*/
static int rw_mode;                                     /* ASCII or ... (see header)	*/
static char buffer[MGIO_BUFFERSIZE]; /* general purpose buffer		*/
static int intList[MGIO_INTSIZE];       /* general purpose integer list */
static double doubleList[MGIO_DOUBLESIZE]; /* general purpose double list*/

/* local storage of general elements */
static MGIO_GE_ELEMENT lge[MGIO_TAGS];


#ifdef __MGIO_USE_IN_UG__

static char RCS_ID("$Header$",UG_RCS_STRING);

#endif

/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/


/****************************************************************************/
/*D
   Read_OpenMGFile - opens file for reading

   SYNOPSIS:
   int Read_OpenMGFile (char *filename);

   PARAMETERS:
   .  filename - name of file

   DESCRIPTION:
   opens a file with specified name for reading

   RETURN VALUE:
   int
   .n    0 if ok
   .n    1 when error occured.

   SEE ALSO:
   D*/
/****************************************************************************/

int Read_OpenMGFile (char *filename)
{

#ifdef __MGIO_USE_IN_UG__
  if (mgpathes_set) stream = FileOpenUsingSearchPaths(filename,"r","mgpaths");
  else stream = fileopen(filename,"r");
#else
  stream = fopen(filename,"r");
#endif

  if (stream==NULL) return (1);

  return (0);
}

/****************************************************************************/
/*D
        Write_OpenMGFile - opens file for reading

   SYNOPSIS:
   int Write_OpenMGFile (char *filename);

   PARAMETERS:
   .  filename - name of file

   DESCRIPTION:
   opens a file with specified name for writing

   RETURN VALUE:
   int
   .n    0 if ok
   .n    1 when error occured.

   SEE ALSO:
   D*/
/****************************************************************************/

int Write_OpenMGFile (char *filename)
{

#ifdef __MGIO_USE_IN_UG__
  if (mgpathes_set) stream = FileOpenUsingSearchPaths(filename,"w","mgpaths");
  else stream = fileopen(filename,"w");
#else
  stream = fopen(filename,"w");
#endif

  if (stream==NULL) return (1);
  return (0);
}

/****************************************************************************/
/*
   Read_MG_General - reads general information about mg

   SYNOPSIS:
   int Read_MG_General (MGIO_MG_GENERAL *mg_general);

   PARAMETERS:
   .  mg_general - general information about mg

   DESCRIPTION:
   function reads general information about the mg

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int     Read_MG_General (MGIO_MG_GENERAL *mg_general)
{
  /* initialize basic i/o */
  if (Bio_Initialize(stream,BIO_ASCII)) return (1);

  /* head always in ACSII */
  if (Bio_Read_string(buffer)) return (1);if (strcmp(buffer,MGIO_TITLE_LINE)!=0) return (1);
  if (Bio_Read_mint(1,intList)) return (1);
  mg_general->mode                = intList[0];

  /* re-initialize basic i/o */
  if (Bio_Initialize(stream,mg_general->mode)) return (1);

  /* now special mode */
  if (Bio_Read_string(mg_general->version)) return (1);
  if (Bio_Read_string(mg_general->ident)) return (1);
  if (Bio_Read_string(mg_general->DomainName)) return (1);
  if (Bio_Read_string(mg_general->MultiGridName)) return (1);
  if (Bio_Read_string(mg_general->Formatname)) return (1);
  if (Bio_Read_mint(8,intList)) return (1);
  mg_general->dim                 = intList[0];
  mg_general->magic_cookie= intList[1];
  mg_general->heapsize    = intList[2];
  mg_general->nLevel              = intList[3];
  mg_general->nNode               = intList[4];
  mg_general->nPoint              = intList[5];
  mg_general->nElement    = intList[6];
  mg_general->VectorTypes = intList[7];

  return (0);
}

/****************************************************************************/
/*
   Write_MG_General - writes general information about mg

   SYNOPSIS:
   int Write_MG_General (MGIO_MG_GENERAL *mg_general);

   PARAMETERS:
   .  mg_general - general information about mg

   DESCRIPTION:
   function writes general information about the mg

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int     Write_MG_General (MGIO_MG_GENERAL *mg_general)
{
  /* initialize basic i/o */
  if (Bio_Initialize(stream,BIO_ASCII)) return (1);

  /* head always in ACSII */
  if (Bio_Write_string(MGIO_TITLE_LINE)) return (1);
  intList[0] = mg_general->mode;
  if (Bio_Write_mint(1,intList)) return (1);

  /* initialize basic i/o */
  if (Bio_Initialize(stream,mg_general->mode)) return (1);

  /* now special mode */
  if (Bio_Write_string(mg_general->version)) return (1);
  if (Bio_Write_string(mg_general->ident)) return (1);
  if (Bio_Write_string(mg_general->DomainName)) return (1);
  if (Bio_Write_string(mg_general->MultiGridName)) return (1);
  if (Bio_Write_string(mg_general->Formatname)) return (1);
  intList[0] = mg_general->dim;
  intList[1] = mg_general->magic_cookie;
  intList[2] = mg_general->heapsize;
  intList[3] = mg_general->nLevel;
  intList[4] = mg_general->nNode;
  intList[5] = mg_general->nPoint;
  intList[6] = mg_general->nElement;
  intList[7] = mg_general->VectorTypes;
  if (Bio_Write_mint(8,intList)) return (1);

  return (0);
}

/****************************************************************************/
/*
   Read_GE_General - reads general information about general elements

   SYNOPSIS:
   int	Read_GE_General (MGIO_GE_GENERAL *ge_general);

   PARAMETERS:
   .  ge_general - general information about general elements

   DESCRIPTION:
   function reads general information about general elements

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int     Read_GE_General (MGIO_GE_GENERAL *ge_general)
{
  if (Bio_Read_mint(1,intList)) return (1);
  ge_general->nGenElement = intList[0];

  return (0);
}

/****************************************************************************/
/*
   Write_GE_General - writes general information about general elements

   SYNOPSIS:
   int	Write_GE_General (MGIO_GE_GENERAL *ge_general);

   PARAMETERS:
   .  ge_general - general information about general elements

   DESCRIPTION:
   function writes general information about general elements

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int     Write_GE_General (MGIO_GE_GENERAL *ge_general)
{
  intList[0] = ge_general->nGenElement;
  if (Bio_Write_mint(1,intList)) return (1);

  return (0);
}

/****************************************************************************/
/*
   Read_GE_Elements - reads general elements

   SYNOPSIS:
   int	Read_GE_Elements (int n, MGIO_GE_ELEMENT *ge_element);

   PARAMETERS:
   .  n - nb of general elements to read
   .  ge_element - first general elements

   DESCRIPTION:
   function reads general elements

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int     Read_GE_Elements (int n, MGIO_GE_ELEMENT *ge_element)
{
  int i,j,s;
  MGIO_GE_ELEMENT *pge;

  pge = ge_element;
  for (i=0; i<n; i++)
  {
    if (Bio_Read_mint(4,intList)) return (1);s=0;
    pge->tag                = lge[i].tag            = intList[s++];
    pge->nCorner    = lge[i].nCorner        = intList[s++];
    pge->nEdge              = lge[i].nEdge          = intList[s++];
    pge->nSide              = lge[i].nSide          = intList[s++];
    if (pge->nEdge>0 || pge->nSide>0)
    {
      if (Bio_Read_mint(2*pge->nEdge+4*pge->nSide,intList)) return (1);s=0;
      for (j=0; j<pge->nEdge; j++)
      {
        pge->CornerOfEdge[j][0] = lge[i].CornerOfEdge[j][0] = intList[s++];
        pge->CornerOfEdge[j][1] = lge[i].CornerOfEdge[j][1] = intList[s++];
      }
      for (j=0; j<pge->nSide; j++)
      {
        pge->CornerOfSide[j][0] = lge[i].CornerOfSide[j][0] = intList[s++];
        pge->CornerOfSide[j][1] = lge[i].CornerOfSide[j][1] = intList[s++];
        pge->CornerOfSide[j][2] = lge[i].CornerOfSide[j][2] = intList[s++];
        pge->CornerOfSide[j][3] = lge[i].CornerOfSide[j][3] = intList[s++];
      }
    }
    pge++;
  }

  return (0);
}

/****************************************************************************/
/*
   Write_GE_Elements - writes general elements

   SYNOPSIS:
   int	Write_GE_Elements (int n, MGIO_GE_ELEMENT *ge_element);

   PARAMETERS:
   .  n - nb of general elements to write
   .  ge_element - first general elements

   DESCRIPTION:
   function writes general elements

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int     Write_GE_Elements (int n, MGIO_GE_ELEMENT *ge_element)
{
  int i,j,s;
  MGIO_GE_ELEMENT *pge;

  pge = ge_element;
  for (i=0; i<n; i++)
  {
    s=0;
    intList[s++] = lge[i].tag = pge->tag;
    intList[s++] = lge[i].nCorner = pge->nCorner;
    intList[s++] = lge[i].nEdge = pge->nEdge;
    intList[s++] = lge[i].nSide = pge->nSide;
    for (j=0; j<pge->nEdge; j++)
    {
      intList[s++] = lge[i].CornerOfEdge[j][0] = pge->CornerOfEdge[j][0];
      intList[s++] = lge[i].CornerOfEdge[j][1] = pge->CornerOfEdge[j][1];
    }
    for (j=0; j<pge->nSide; j++)
    {
      intList[s++] = lge[i].CornerOfSide[j][0] = pge->CornerOfSide[j][0];
      intList[s++] = lge[i].CornerOfSide[j][1] = pge->CornerOfSide[j][1];
      intList[s++] = lge[i].CornerOfSide[j][2] = pge->CornerOfSide[j][2];
      intList[s++] = lge[i].CornerOfSide[j][3] = pge->CornerOfSide[j][3];
    }
    MGIO_CHECK_INTSIZE(s);
    if (Bio_Write_mint(s,intList)) return (1);
    pge++;
  }

  return (0);
}

/****************************************************************************/
/*
   Read_RR_General - reads general information about refinement rules

   SYNOPSIS:
   int	Read_RR_General (MGIO_RR_GENERAL *mgio_rr_general);

   PARAMETERS:
   .  mgio_rr_general - general information about refinement rules

   DESCRIPTION:
   function reads general information about refinement rules

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int     Read_RR_General (MGIO_RR_GENERAL *mgio_rr_general)
{
  int i,s;

  if (Bio_Read_mint(1+MGIO_TAGS,intList)) return (1);
  s=0;
  mgio_rr_general->nRules = intList[s++];
  for (i=0; i<MGIO_TAGS; i++)
    mgio_rr_general->RefRuleOffset[i] = intList[s++];

  return (0);
}

/****************************************************************************/
/*
   Write_GE_General - writes general information about refinement rules

   SYNOPSIS:
   int	Write_RR_General (MGIO_RR_GENERAL *mgio_rr_general);

   PARAMETERS:
   .  mgio_rr_general - general information about refinement rules

   DESCRIPTION:
   function writes general information about refinement rules

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int     Write_RR_General (MGIO_RR_GENERAL *mgio_rr_general)
{
  int i,s;

  s=0;
  intList[s++] = mgio_rr_general->nRules;
  for (i=0; i<MGIO_TAGS; i++)
    intList[s++] = mgio_rr_general->RefRuleOffset[i];
  if (Bio_Write_mint(s,intList)) return (1);

  return (0);
}

/****************************************************************************/
/*
   Read_RR_Rules - reads refinement rules

   SYNOPSIS:
   int	Read_RR_Rules (int n, MGIO_RR_RULE *rr_rules);

   PARAMETERS:
   .  n - nb of refinement rules to read
   .  rr_rules - first refinement rule

   DESCRIPTION:
   function reads refinement rules

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int     Read_RR_Rules (int n, MGIO_RR_RULE *rr_rules)
{
  int i,j,k,m,s;
  MGIO_RR_RULE *prr;

  prr = rr_rules;
  for (i=0; i<n; i++)
  {
    if (Bio_Read_mint(2,intList)) return (1);
    prr->class = intList[0];
    prr->nsons = intList[1];
    m = MGIO_MAX_NEW_CORNERS+2*MGIO_MAX_NEW_CORNERS+prr->nsons*(1+MGIO_MAX_CORNERS_OF_ELEM+MGIO_MAX_SIDES_OF_ELEM+1);
    if (Bio_Read_mint(m,intList)) return (1);
    s=0;
    for (j=0; j<MGIO_MAX_NEW_CORNERS; j++)
      prr->pattern[j] = intList[s++];
    for (j=0; j<MGIO_MAX_NEW_CORNERS; j++)
    {
      prr->sonandnode[j][0] = intList[s++];
      prr->sonandnode[j][1] = intList[s++];
    }
    for (j=0; j<prr->nsons; j++)
    {
      prr->sons[j].tag = intList[s++];
      for (k=0; k<MGIO_MAX_CORNERS_OF_ELEM; k++)
        prr->sons[j].corners[k] = intList[s++];
      for (k=0; k<MGIO_MAX_SIDES_OF_ELEM; k++)
        prr->sons[j].nb[k] = intList[s++];
      prr->sons[j].path = intList[s++];
    }
    prr++;
  }

  return (0);
}

/****************************************************************************/
/*
   Write_RR_Rules - writes refinement rules

   SYNOPSIS:
   int	Write_RR_Rules (int n, MGIO_RR_RULE *rr_rules);

   PARAMETERS:
   .  n - nb of refinement rules to write
   .  rr_rules - first refinement rule

   DESCRIPTION:
   function writes refinement rules

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int     Write_RR_Rules (int n, MGIO_RR_RULE *rr_rules)
{
  int i,j,k,s;
  MGIO_RR_RULE *prr;

  prr = rr_rules;
  for (i=0; i<n; i++)
  {
    s=0;
    intList[s++] = prr->class;
    intList[s++] = prr->nsons;
    for (j=0; j<MGIO_MAX_NEW_CORNERS; j++)
      intList[s++] = prr->pattern[j];
    for (j=0; j<MGIO_MAX_NEW_CORNERS; j++)
    {
      intList[s++] = prr->sonandnode[j][0];
      intList[s++] = prr->sonandnode[j][1];
    }
    for (j=0; j<prr->nsons; j++)
    {
      intList[s++] = prr->sons[j].tag;
      for (k=0; k<MGIO_MAX_CORNERS_OF_ELEM; k++)
        intList[s++] = prr->sons[j].corners[k];
      for (k=0; k<MGIO_MAX_SIDES_OF_ELEM; k++)
        intList[s++] = prr->sons[j].nb[k];
      intList[s++] = prr->sons[j].path;
    }
    MGIO_CHECK_INTSIZE(s);
    if (Bio_Write_mint(s,intList)) return (1);
    prr++;
  }

  return (0);
}

/****************************************************************************/
/*
   Read_CG_General - reads general information about coarse grid

   SYNOPSIS:
   int Read_CG_General (MGIO_CG_GENERAL *cg_general);

   PARAMETERS:
   .  cg_general - general information about coarse grid

   DESCRIPTION:
   function reads general information about coarse grid

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int Read_CG_General (MGIO_CG_GENERAL *cg_general)
{
  if (Bio_Read_mint(6,intList)) return (1);
  cg_general->nPoint                      = intList[0];
  cg_general->nBndPoint           = intList[1];
  cg_general->nInnerPoint         = intList[2];
  cg_general->nElement            = intList[3];
  cg_general->nBndElement         = intList[4];
  cg_general->nInnerElement       = intList[5];

  return (0);
}

/****************************************************************************/
/*
   Write_CG_General - writes general information about coarse grid

   SYNOPSIS:
   int Write_CG_General (MGIO_CG_GENERAL *cg_general);

   PARAMETERS:
   .  cg_general - general information about coarse grid

   DESCRIPTION:
   function writes general information about coarse grid

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int Write_CG_General (MGIO_CG_GENERAL *cg_general)
{

  intList[0] = cg_general->nPoint;
  intList[1] = cg_general->nBndPoint;
  intList[2] = cg_general->nInnerPoint;
  intList[3] = cg_general->nElement;
  intList[4] = cg_general->nBndElement;
  intList[5] = cg_general->nInnerElement;
  if (Bio_Write_mint(6,intList)) return (1);

  return (0);
}

/****************************************************************************/
/*
   Read_CG_Points - reads coarse grid points

   SYNOPSIS:
   int Read_CG_Points (MGIO_CG_POINT *cg_point);

   PARAMETERS:
   .  n - nb of coarse grid points to read
   .  cg_point - first coarse grid point

   DESCRIPTION:
   function reads coarse grid points

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int Read_CG_Points (int n, MGIO_CG_POINT *cg_point)
{
  int i,j,s,nmax,read,copy_until,still_to_read;

  copy_until=0; still_to_read=n;
  nmax = MGIO_DOUBLESIZE-MGIO_DOUBLESIZE%MGIO_DIM; nmax /= MGIO_DIM;
  for(i=0; i<n; i++)
  {
    if (i>=copy_until)
    {
      if (still_to_read<=nmax) read=still_to_read;
      else read=nmax;
      if (Bio_Read_mdouble(MGIO_DIM*read,doubleList)) return (1);
      still_to_read -= read;
      copy_until += read;
      s=0;
    }
    for(j=0; j<MGIO_DIM; j++)
      cg_point[i].position[j] = doubleList[s++];
  }

  return (0);
}

/****************************************************************************/
/*
   Write_CG_Points - writes coarse grid points

   SYNOPSIS:
   int Write_CG_Points (MGIO_CG_POINT *cg_point);

   PARAMETERS:
   .  n - nb of coarse grid points to write
   .  cg_point - first coarse grid point

   DESCRIPTION:
   function writes coarse grid points

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int Write_CG_Points (int n, MGIO_CG_POINT *cg_point)
{
  int i,j,s;

  s=0;
  for(i=0; i<n; i++)
  {
    for(j=0; j<MGIO_DIM; j++)
      doubleList[s++] = cg_point[i].position[j];
    if (s>MGIO_DOUBLESIZE-MGIO_DIM)
    {
      if (Bio_Write_mdouble(s,doubleList)) return (1);
      s=0;
    }
  }
  if (Bio_Write_mdouble(s,doubleList)) return (1);

  return (0);
}

/****************************************************************************/
/*
   Read_CG_Elements - reads coarse grid elements

   SYNOPSIS:
   int Read_CG_Elements (int n, MGIO_CG_ELEMENT *cg_element);

   PARAMETERS:
   .  n - nb of coarse grid elements to read
   .  cg_element - first coarse grid element

   DESCRIPTION:
   function reads coarse grid elements

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int Read_CG_Elements (int n, MGIO_CG_ELEMENT *cg_element)
{
  int i,j,m,s;
  MGIO_CG_ELEMENT *pe;

  pe = cg_element;
  for (i=0; i<n; i++)
  {
    /* coarse grid part */
    if (Bio_Read_mint(1,&pe->ge)) return (1);
    m=lge[pe->ge].nCorner+lge[pe->ge].nSide+2;
    if (Bio_Read_mint(m,intList)) return (1);
    s=0;
    pe->nhe = intList[s++];
    for (j=0; j<lge[pe->ge].nCorner; j++)
      pe->cornerid[j] = intList[s++];
    for (j=0; j<lge[pe->ge].nSide; j++)
      pe->nbid[j] = intList[s++];
    pe->subdomain = intList[s++];
    pe++;
  }

  return (0);
}

/****************************************************************************/
/*
   Write_CG_Elements - writes coarse grid elements

   SYNOPSIS:
   int Write_CG_Elements (int n, MGIO_CG_ELEMENT *cg_element);

   PARAMETERS:
   .  n - nb of coarse grid elements to write
   .  rr_rules - first coarse grid element

   DESCRIPTION:
   function writes coarse grid elements

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int Write_CG_Elements (int n, MGIO_CG_ELEMENT *cg_element)
{
  int i,j,s;
  MGIO_CG_ELEMENT *pe;

  pe = cg_element;
  for (i=0; i<n; i++)
  {
    /* coarse grid part */
    s=0;
    intList[s++] = pe->ge;
    intList[s++] = pe->nhe;
    for (j=0; j<lge[pe->ge].nCorner; j++)
      intList[s++] = pe->cornerid[j];
    for (j=0; j<lge[pe->ge].nSide; j++)
      intList[s++] = pe->nbid[j];
    intList[s++] = pe->subdomain;
    MGIO_CHECK_INTSIZE(s);
    if (Bio_Write_mint(s,intList)) return (1);

    pe++;
  }

  return (0);
}

/****************************************************************************/
/*
   Read_HE_Refinement - reads hierarchical elements

   SYNOPSIS:
   int Read_HE_Refinement (MGIO_HE_ELEMENT *he_element);

   PARAMETERS:
   .  n - nb of hierarchical elements to read
   .  he_element - hierarchical element

   DESCRIPTION:
   function reads hierarchical elements

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int Read_Refinement (int n, MGIO_REFINEMENT *refinement)
{
  int i,j,k,s;
  unsigned int ctrl;
  MGIO_REFINEMENT *pr;

  pr = refinement;
  for (i=0; i<n; i++)
  {
    if (Bio_Read_mint(2,intList)) return (1);
    ctrl = intList[0];
    pr->sonref = intList[1];
    pr->refrule = MGIO_ECTRL_REF(ctrl)-1;
    if (pr->refrule>-1)
    {
      pr->nnewcorners = MGIO_ECTRL_NC(ctrl);
      pr->nmoved = MGIO_ECTRL_NM(ctrl);
      pr->refclass = MGIO_ECTRL_REFCLASS(ctrl);
      if (pr->nnewcorners+pr->nmoved>0)
        if (Bio_Read_mint(pr->nnewcorners+pr->nmoved,intList)) return (1);
      s=0;
      for (j=0; j<pr->nnewcorners; j++)
        pr->newcornerid[j] = intList[s++];
      for (j=0; j<pr->nmoved; j++)
        pr->mvcorner[j].id = intList[s++];
      if (pr->nmoved>0)
      {
        if (Bio_Read_mdouble(MGIO_DIM*pr->nmoved,doubleList)) return (1);
        s=0;
        for (j=0; j<pr->nmoved; j++)
          for (k=0; k<MGIO_DIM; k++)
            pr->mvcorner[j].position[k] = doubleList[s++];
      }
    }
    pr++;
  }

  return (0);
}

/****************************************************************************/
/*
   Write_HE_Refinement - writes hierarchical elements

   SYNOPSIS:
   int Write_HE_Refinement (int n, MGIO_HE_ELEMENT *he_element);

   PARAMETERS:
   .  n - nb of hierarchical elements to write
   .  he_element - hierarchical element

   DESCRIPTION:
   function writes hierarchical elements

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int Write_Refinement (int n, MGIO_REFINEMENT *refinement)
{
  int i,j,k,s,t;
  MGIO_REFINEMENT *pr;

  pr = refinement;
  for (i=0; i<n; i++)
  {
    s=t=0;
    intList[s++] = MGIO_ECTRL(pr->refrule+1,pr->nnewcorners,pr->nmoved,pr->refclass);
    intList[s++] = pr->sonref;
    if (pr->refrule>-1)
    {
      for (j=0; j<pr->nnewcorners; j++)
        intList[s++] = pr->newcornerid[j];
      for (j=0; j<pr->nmoved; j++)
        intList[s++] = pr->mvcorner[j].id;
      for (j=0; j<pr->nmoved; j++)
        for (k=0; k<MGIO_DIM; k++)
          doubleList[t++] = pr->mvcorner[j].position[k];
    }

    MGIO_CHECK_INTSIZE(s);
    if (Bio_Write_mint(s,intList)) return (1);
    MGIO_CHECK_DOUBLESIZE(t);
    if (t>0) if (Bio_Write_mdouble(t,doubleList)) return (1);

    pr++;
  }

  return (0);
}

/****************************************************************************/
/*
   Read_BD_General - reads general information about boundary description

   SYNOPSIS:
   int Read_BD_General (MGIO_BD_GENERAL *bd_general);

   PARAMETERS:
   .  bd_general - information about boundary description

   DESCRIPTION:
   function reads general information about boundary description

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int Read_BD_General (MGIO_BD_GENERAL *bd_general)
{
  if (Bio_Read_mint(1,intList)) return (1);
  bd_general->nBndP = intList[0];

  return (0);
}

/****************************************************************************/
/*
   Write_BD_General - writes general information about boundary description

   SYNOPSIS:
   int Write_BD_General (MGIO_BD_GENERAL *bd_general);

   PARAMETERS:
   .  bd_general - information about boundary description

   DESCRIPTION:
   function writes general information about boundary description

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int Write_BD_General (MGIO_BD_GENERAL *bd_general)
{
  intList[0] = bd_general->nBndP;
  if (Bio_Write_mint(1,intList)) return (1);

  return (0);
}

/****************************************************************************/
/*
   Read_PBndDesc - reads BNDPs

   SYNOPSIS:
   int Read_PBndDesc (MGIO_HEAP *theHeap, int n, BNDP **BndPList);

   PARAMETERS:
   .  theHeap - heap
   .  n - nb of BndP to read
   .  BndPList - list of ptrs to BndP

   DESCRIPTION:
   function reads BNDPs

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

#ifdef __MGIO_USE_IN_UG__

int Read_PBndDesc (BVP *theBVP, HEAP *theHeap, int n, BNDP **BndPList)
{
  int i;

  if (theHeap==NULL) return (1);
  for (i=0; i<n; i++)
  {
    BndPList[i] = BNDP_LoadBndP (theBVP,theHeap);
    if (BndPList[i]==NULL) return (1);
  }
  return (0);
}

#endif

/****************************************************************************/
/*
   Write_PBndDesc - write BNDPs

   SYNOPSIS:
   int Write_PBndDesc (int n, BNDP **BndPList);

   PARAMETERS:
   .  n - nb of BndP to write
   .  BndPList - list of ptrs to BndP

   DESCRIPTION:
   function writes BNDPs

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

#ifdef __MGIO_USE_IN_UG__

int Write_PBndDesc (int n, BNDP **BndPList)
{
  int i;

  for (i=0; i<n; i++)
    if (BNDP_SaveBndP (BndPList[i])) return (1);
  return (0);
}

#endif

/****************************************************************************/
/*
   CloseMGFile - close the file

   SYNOPSIS:
   int CloseMGFile ();

   PARAMETERS:

   DESCRIPTION:
   close the file

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int CloseMGFile ()
{
  if (fclose(stream)!=NULL) return (1);
  return (0);
}

/****************************************************************************/
/*
   MGIO_Init - init input/output for mg

   SYNOPSIS:
   int MGIO_Init (void);

   PARAMETERS:

   DESCRIPTION:
   init the i/o of mg

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if read error.

   SEE ALSO:
 */
/****************************************************************************/

int MGIO_Init ()
{

#ifdef __MGIO_USE_IN_UG__

  /* path to grid-dirs */
  mgpathes_set = 0;
  if (ReadSearchingPaths(DEFAULTSFILENAME,"mgpaths")==0)
    mgpathes_set = 1;

#endif

  return (0);
}
