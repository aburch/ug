// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  ugblas.c														*/
/*																			*/
/* Purpose:   basic linear algebra routines                                                             */
/*			  working on the matrix-vector and								*/
/*			  matrix-blockvector structure									*/
/*																			*/
/* Author:	  Henrik Rentz-Reichert                                                                                 */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*																			*/
/*			  blockvector routines from:									*/
/*			  Christian Wrobel                                                                              */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*																			*/
/*			  email: ug@ica3.uni-stuttgart.de					                */
/*																			*/
/* History:   06.03.95 begin, ug version 3.0								*/
/*			  28.09.95 blockvector routines implemented (Christian Wrobel)	*/
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
#include <math.h>
#include <assert.h>

#include "compiler.h"
#include "misc.h"
#include "evm.h"
#include "gm.h"
#include "algebra.h"
#include "devices.h"
#include "general.h"
#include "debug.h"
#ifdef ModelP
#include "pargm.h"
#include "parallel.h"
#endif

#include "np.h"
#include "disctools.h"
#include "ugblas.h"

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

#define VERBOSE_BLAS    10

#define MATARRAYSIZE 512

/* macros to define VEC_SCALAR, VECDATA_DESC and MATDATA_DESC components */
#define DEFINE_VS_CMPS(a)                               register DOUBLE a ## 0,a ## 1,a ## 2
#define DEFINE_VD_CMPS(x)                               register INT x ## 0,x ## 1,x ## 2
#define DEFINE_MD_CMPS(m)                               register INT m ## 00,m ## 01,m ## 02,m ## 10,m ## 11,m ## 12,m ## 20,m ## 21,m ## 22

/* macros to set VEC_SCALAR components */
#define SET_VS_CMP_1(a,A,off,tp)                {a ## 0 = (A)[(off)[tp]];}
#define SET_VS_CMP_2(a,A,off,tp)                {a ## 0 = (A)[(off)[tp]]; a ## 1 = (A)[(off)[tp]+1];}
#define SET_VS_CMP_3(a,A,off,tp)                {a ## 0 = (A)[(off)[tp]]; a ## 1 = (A)[(off)[tp]+1]; a ## 2 = (A)[(off)[tp]+2];}

/* macros to set VECDATA_DESC components */
#define SET_VD_CMP_1(x,v,tp)                    {x ## 0 = VD_CMP_OF_TYPE(v,tp,0);}
#define SET_VD_CMP_2(x,v,tp)                    {x ## 0 = VD_CMP_OF_TYPE(v,tp,0); x ## 1 = VD_CMP_OF_TYPE(v,tp,1);}
#define SET_VD_CMP_3(x,v,tp)                    {x ## 0 = VD_CMP_OF_TYPE(v,tp,0); x ## 1 = VD_CMP_OF_TYPE(v,tp,1); x ## 2 = VD_CMP_OF_TYPE(v,tp,2);}

#define SET_VD_CMP_N(x,v,tp)                    switch (VD_NCMPS_IN_TYPE(v,tp)) {case 1 : SET_VD_CMP_1(x,v,tp); break; \
                                                                               case 2 : SET_VD_CMP_2(x,v,tp); break; \
                                                                               case 3 : SET_VD_CMP_3(x,v,tp); break;}

/* macros to set MATDATA_DESC components */
#define SET_MD_CMP_11(m,M,rt,ct)                {m ## 00 = MD_MCMP_OF_RT_CT(M,rt,ct,0);}
#define SET_MD_CMP_12(m,M,rt,ct)                {m ## 00 = MD_MCMP_OF_RT_CT(M,rt,ct,0); m ## 01 = MD_MCMP_OF_RT_CT(M,rt,ct,1);}
#define SET_MD_CMP_13(m,M,rt,ct)                {m ## 00 = MD_MCMP_OF_RT_CT(M,rt,ct,0); m ## 01 = MD_MCMP_OF_RT_CT(M,rt,ct,1); m ## 02 = MD_MCMP_OF_RT_CT(M,rt,ct,2);}
#define SET_MD_CMP_21(m,M,rt,ct)                {m ## 00 = MD_MCMP_OF_RT_CT(M,rt,ct,0); m ## 10 = MD_MCMP_OF_RT_CT(M,rt,ct,1);}
#define SET_MD_CMP_22(m,M,rt,ct)                {m ## 00 = MD_MCMP_OF_RT_CT(M,rt,ct,0); m ## 01 = MD_MCMP_OF_RT_CT(M,rt,ct,1); \
                                                 m ## 10 = MD_MCMP_OF_RT_CT(M,rt,ct,2); m ## 11 = MD_MCMP_OF_RT_CT(M,rt,ct,3);}
#define SET_MD_CMP_23(m,M,rt,ct)                {m ## 00 = MD_MCMP_OF_RT_CT(M,rt,ct,0); m ## 01 = MD_MCMP_OF_RT_CT(M,rt,ct,1); m ## 02 = MD_MCMP_OF_RT_CT(M,rt,ct,2); \
                                                 m ## 10 = MD_MCMP_OF_RT_CT(M,rt,ct,3); m ## 11 = MD_MCMP_OF_RT_CT(M,rt,ct,4); m ## 12 = MD_MCMP_OF_RT_CT(M,rt,ct,5);}
#define SET_MD_CMP_31(m,M,rt,ct)                {m ## 00 = MD_MCMP_OF_RT_CT(M,rt,ct,0); \
                                                 m ## 10 = MD_MCMP_OF_RT_CT(M,rt,ct,1); \
                                                 m ## 20 = MD_MCMP_OF_RT_CT(M,rt,ct,2);}
#define SET_MD_CMP_32(m,M,rt,ct)                {m ## 00 = MD_MCMP_OF_RT_CT(M,rt,ct,0); m ## 01 = MD_MCMP_OF_RT_CT(M,rt,ct,1); \
                                                 m ## 10 = MD_MCMP_OF_RT_CT(M,rt,ct,2); m ## 11 = MD_MCMP_OF_RT_CT(M,rt,ct,3); \
                                                 m ## 20 = MD_MCMP_OF_RT_CT(M,rt,ct,4); m ## 21 = MD_MCMP_OF_RT_CT(M,rt,ct,5);}
#define SET_MD_CMP_33(m,M,rt,ct)                {m ## 00 = MD_MCMP_OF_RT_CT(M,rt,ct,0); m ## 01 = MD_MCMP_OF_RT_CT(M,rt,ct,1); m ## 02 = MD_MCMP_OF_RT_CT(M,rt,ct,2); \
                                                 m ## 10 = MD_MCMP_OF_RT_CT(M,rt,ct,3); m ## 11 = MD_MCMP_OF_RT_CT(M,rt,ct,4); m ## 12 = MD_MCMP_OF_RT_CT(M,rt,ct,5); \
                                                 m ## 20 = MD_MCMP_OF_RT_CT(M,rt,ct,6); m ## 21 = MD_MCMP_OF_RT_CT(M,rt,ct,7); m ## 22 = MD_MCMP_OF_RT_CT(M,rt,ct,8);}

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

#ifdef ModelP
static VECDATA_DESC *ConsVector;
static MATDATA_DESC *ConsMatrix;
static GRID *ConsGrid;
static INT MaximumInconsMatrices;
static MATRIX *MatArrayLocal[MATARRAYSIZE];
static MATRIX *MatArrayRemote[MATARRAYSIZE];
static INT MaxBlockSize;
static size_t DataSizePerVector;
static size_t DataSizePerMatrix;

#ifdef __TWODIM__
static INT max_vectors_of_type[NVECTYPES] =
{ MAX_CORNERS_OF_ELEM, MAX_EDGES_OF_ELEM, 1};
#endif

#ifdef __THREEDIM__
static INT max_vectors_of_type[NVECTYPES] =
{ MAX_CORNERS_OF_ELEM, MAX_EDGES_OF_ELEM, 1, MAX_SIDES_OF_ELEM};
#endif

#ifdef __BLOCK_VECTOR_DESC__
static const BV_DESC *ConsBvd;
static const BV_DESC_FORMAT *ConsBvdf;
static INT ConsComp;
#endif

#endif

/* RCS string */
const static char RCS_ID("$Header$",UG_RCS_STRING);

REP_ERR_FILE;

/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*D
   VecCheckConsistency - Check wether two VECDATA_DESCs match

   SYNOPSIS:
   INT VecCheckConsistency (const VECDATA_DESC *x, const VECDATA_DESC *y)

   PARAMETERS:
   .  x - vector data descriptor
   .  y - vector data descriptor

   DESCRIPTION:
   This function checks wether the two VECDATA_DESCs match.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_DESC_MISMATCH if the type descriptors does not match
   D*/
/****************************************************************************/

INT VecCheckConsistency (const VECDATA_DESC *x, const VECDATA_DESC *y)
{
  INT vtype;

  for (vtype=0; vtype<NVECTYPES; vtype++)
    if (VD_ISDEF_IN_TYPE(x,vtype))
    {
      /* consistency check: the x-types should include the y-types */
      if (!VD_ISDEF_IN_TYPE(y,vtype))
        REP_ERR_RETURN (NUM_DESC_MISMATCH);

      /* consistency check: the x-nComp should be equal to the y-nComp */
      if (VD_NCMPS_IN_TYPE(x,vtype) != VD_NCMPS_IN_TYPE(y,vtype))
        REP_ERR_RETURN (NUM_DESC_MISMATCH);
    }
  return (NUM_OK);
}

/****************************************************************************/
/*D
   MatmulCheckConsistency - Check the consistency of the data descriptors

   SYNOPSIS:
   INT MatmulCheckConsistency (const VECDATA_DESC *x, const MATDATA_DESC *M,
   const VECDATA_DESC *y )

   PARAMETERS:
   .  x - vector data descriptor
   .  M - matrix data descriptor
   .  y - vector data descriptor

   DESCRIPTION:
   This function checks whether the VECDATA_DESCs and the MATDATA_DESC
   match.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_DESC_MISMATCH if the type descriptors not match
   .n    NUM_BLOCK_TOO_LARGE if the blocks are larger as MAX_SINGLE_VEC_COMP
   D*/
/****************************************************************************/

INT MatmulCheckConsistency (const VECDATA_DESC *x, const MATDATA_DESC *M, const VECDATA_DESC *y)
{
  INT rtype,ctype,maxsmallblock,found;

  /* consistency check */
  maxsmallblock = 0;
  for (rtype=0; rtype<NVECTYPES; rtype++)
    if (VD_ISDEF_IN_TYPE(x,rtype))
    {
      maxsmallblock = MAX(maxsmallblock,VD_NCMPS_IN_TYPE(x,rtype));
      found = FALSE;
      for (ctype=0; ctype<NVECTYPES; ctype++)
        if (MD_ISDEF_IN_RT_CT(M,rtype,ctype))
        {
          found = TRUE;
          if (!VD_ISDEF_IN_TYPE(y,ctype))
            REP_ERR_RETURN (NUM_DESC_MISMATCH);
          maxsmallblock = MAX(maxsmallblock,VD_NCMPS_IN_TYPE(y,ctype));

          /* consistency check: the M-nRow/ColComp should match the nComps of x,y resp. */
          if (MD_ROWS_IN_RT_CT(M,rtype,ctype) != VD_NCMPS_IN_TYPE(x,rtype))
            REP_ERR_RETURN (NUM_DESC_MISMATCH);
          if (MD_COLS_IN_RT_CT(M,rtype,ctype) != VD_NCMPS_IN_TYPE(y,ctype))
            REP_ERR_RETURN (NUM_DESC_MISMATCH);
        }
      if (!found) REP_ERR_RETURN (NUM_DESC_MISMATCH);
    }

  /* check size of the largest small block */
  assert (maxsmallblock <= MAX_SINGLE_VEC_COMP);        /* if too little: increase MAX_SINGLE_VEC_COMP and recompile */
        #ifdef NDEBUG
  /* check also in case NDEBUG is defined (assert off)	*/
  if (maxsmallblock > MAX_SINGLE_VEC_COMP)
    REP_ERR_RETURN (NUM_BLOCK_TOO_LARGE);
        #endif

  return (NUM_OK);
}

/****************************************************************************/
/* naming convention:														*/
/*																			*/
/* all names have the form													*/
/*																			*/
/* ?_function																*/
/*																			*/
/* where ? can be one of the letters:										*/
/*																			*/
/* l	operation working on a grid level									*/
/* s	operation working on all fine grid dof's (surface)                  */
/* a	operation working on all dofs on all levels                                             */
/*																			*/
/* (blockvector routines see below in this file)							*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*D
   l_vector_consistent - builds the sum of the vector values on all copies

   SYNOPSIS:
   INT l_vector_consistent (GRID *g, const VECDATA_DESC *x);

   PARAMETERS:
   .  g - pointer to grid
   .  x - vector data descriptor

   DESCRIPTION:
   This function builds the sum of the vector values for all border vectors.

   RETURN VALUE:
   INT
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
   D*/
/****************************************************************************/

#ifdef ModelP
static int Gather_VectorComp (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  INT i,type;
  const SHORT *Comp;

  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv))
      *((DOUBLE *)data) = VVALUE(pv,VD_SCALCMP(ConsVector));

    return (NUM_OK);
  }

  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
    ((DOUBLE *)data)[i] = VVALUE(pv,Comp[i]);

  return (NUM_OK);
}

static int Scatter_VectorComp (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  INT i,type,vecskip;
  const SHORT *Comp;

  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv))
      if (!VECSKIP(pv))
        VVALUE(pv,VD_SCALCMP(ConsVector)) += *((DOUBLE *)data);

    return (NUM_OK);
  }

  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  vecskip = VECSKIP(pv);
  if (vecskip == 0)
    for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
      VVALUE(pv,Comp[i]) += ((DOUBLE *)data)[i];
  else
    for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
      if (!(vecskip & (1<<i)))
        VVALUE(pv,Comp[i]) += ((DOUBLE *)data)[i];

  return (NUM_OK);
}

INT l_vector_consistent (GRID *g, const VECDATA_DESC *x)
{
  INT tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  DDD_IFAExchange(BorderVectorSymmIF, GRID_ATTR(g), m * sizeof(DOUBLE),
                  Gather_VectorComp, Scatter_VectorComp);
  return (NUM_OK);
}
#endif

/****************************************************************************/
/*D
   a_vector_consistent - builds the sum of the vector values on all copies

   SYNOPSIS:
   INT a_vector_consistent (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - from level
   .  x - vector data descriptor

   DESCRIPTION:
   This function builds the sum of the vector values for all border vectors.

   RETURN VALUE:
   INT
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
   D*/
/****************************************************************************/

#ifdef ModelP
INT a_vector_consistent (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x)
{
  INT level,tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  if ((fl==BOTTOMLEVEL(mg)) && (tl==TOPLEVEL(mg)))
    DDD_IFExchange(BorderVectorSymmIF, m * sizeof(DOUBLE),
                   Gather_VectorComp, Scatter_VectorComp);
  else
    for (level=fl; level<=tl; level++)
      DDD_IFAExchange(BorderVectorSymmIF,
                      GRID_ATTR(GRID_ON_LEVEL(mg,level)),
                      m * sizeof(DOUBLE),
                      Gather_VectorComp, Scatter_VectorComp);

  return (NUM_OK);
}
#endif

/****************************************************************************/
/*D
   l_vector_consistentBS - builds the sum of the vector values within the blockvector on all copies

   SYNOPSIS:
   INT l_vector_consistentBS (GRID *g, const BV_DESC *bvd, const BV_DESC_FORMAT *bvdf, INT x);

   PARAMETERS:
   .  g - pointer to grid
   .  bvd - description of the blockvector
   .  bvdf - format to interpret bvd
   .  x - vector data

   DESCRIPTION:
   This function builds the sum of the vector values within the specified
   blockvector for all master and border vectors; the result is stored in all
   master and border vectors.

   RETURN VALUE:
   INT
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
   D*/
/****************************************************************************/

#if (defined ModelP) && (defined __BLOCK_VECTOR_DESC__)
static int Gather_VectorCompBS (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;

  if( VMATCH(pv,ConsBvd, ConsBvdf) )
  {printf(PFMT "Gather_VectorCompBS: v[%d][%d] = %g\n",me,VINDEX(pv),ConsComp,VVALUE(pv,ConsComp));
   *((DOUBLE *)data) = VVALUE(pv,ConsComp);}
  return (NUM_OK);
}

static int Scatter_VectorCompBS (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;

  if( VMATCH(pv,ConsBvd, ConsBvdf) )
  {
    VVALUE(pv,ConsComp) += *((DOUBLE *)data);
    printf(PFMT "Scatter_VectorCompBS: v[%d][%d] = %g\n",me,VINDEX(pv),ConsComp,VVALUE(pv,ConsComp));
  }

  return (NUM_OK);
}

INT l_vector_consistentBS (GRID *g, const BV_DESC *bvd, const BV_DESC_FORMAT *bvdf, INT x)
{
  ConsBvd = bvd;
  ConsBvdf = bvdf;
  ConsComp = x;

  DDD_IFAExchange(BorderVectorSymmIF, GRID_ATTR(g), sizeof(DOUBLE),
                  Gather_VectorCompBS, Scatter_VectorCompBS);
  return (NUM_OK);
}
#endif

/****************************************************************************/
/*D
   l_ghostvector_consistent - copy values of masters to ghosts

   SYNOPSIS:
   INT l_ghostvector_consistent (GRID *g, const VECDATA_DESC *x);

   PARAMETERS:
   .  g - pointer to grid
   .  x - vector data descriptor

   DESCRIPTION:
   This function copies the vector values of master vectors to ghost vectors.

   RETURN VALUE:
   INT
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
   D*/
/****************************************************************************/

#ifdef ModelP
static int Scatter_GhostVectorComp (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  INT i,type;
  const SHORT *Comp;

  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv))
      VVALUE(pv,VD_SCALCMP(ConsVector)) = *((DOUBLE *)data);

    return (NUM_OK);
  }

  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
    VVALUE(pv,Comp[i]) = ((DOUBLE *)data)[i];

  return (NUM_OK);
}

INT l_ghostvector_consistent (GRID *g, const VECDATA_DESC *x)
{
  INT tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  DDD_IFAOneway(VectorVIF, GRID_ATTR(g), IF_FORWARD, m * sizeof(DOUBLE),
                Gather_VectorComp, Scatter_GhostVectorComp);

  return (NUM_OK);
}
#endif

/****************************************************************************/
/*D
   a_outervector_consistent - makes horizontal ghosts consistent

   SYNOPSIS:
   INT a_outervector_consistent (MULTIGRID *mg, INT fl, INT tl,
   const VECDATA_DESC *x);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - from level
   .  x - vector data descriptor

   DESCRIPTION:
   This function copies the vector values on the master vectors to the
   horizontal ghosts.

   RETURN VALUE:
   INT
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
   D*/
/****************************************************************************/

#ifdef ModelP
INT a_outervector_consistent (MULTIGRID *mg, INT fl, INT tl,
                              const VECDATA_DESC *x)
{
  INT tp,m,level;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  if ((fl==BOTTOMLEVEL(mg)) && (tl==TOPLEVEL(mg)))
    DDD_IFOneway(OuterVectorIF, IF_FORWARD, m * sizeof(DOUBLE),
                 Gather_VectorComp, Scatter_GhostVectorComp);
  else
    for (level=fl; level<=tl; level++)
      DDD_IFAOneway(OuterVectorIF,
                    GRID_ATTR(GRID_ON_LEVEL(mg,level)), IF_FORWARD,
                    m * sizeof(DOUBLE),
                    Gather_VectorComp, Scatter_GhostVectorComp);

  return (NUM_OK);
}
#endif

/****************************************************************************/
/*D
   l_ghostvector_project - copy values of ghosts to masters

   SYNOPSIS:
   INT l_ghostvector_consistent (GRID *g, const VECDATA_DESC *x);

   PARAMETERS:
   .  g - pointer to grid
   .  x - vector data descriptor

   DESCRIPTION:
   This function copies the vector values of master vectors to ghost vectors.

   RETURN VALUE:
   INT
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
   D*/
/****************************************************************************/

#ifdef ModelP
static int Gather_ProjectVectorComp (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  NODE *theNode;
  INT i,type;
  const SHORT *Comp;

  ((INT *)data)[0] = 1;
  if (VOTYPE(pv) == NODEVEC) {
    theNode = SONNODE(VMYNODE(pv));
    if (theNode != NULL)
      if (MASTER(NVECTOR(theNode)))
        ((INT *)data)[0] = 0;
  }
  if (((INT *)data)[0])
    return (NUM_OK);
  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv))
      ((DOUBLE *)data)[1] = VVALUE(pv,VD_SCALCMP(ConsVector));
    return (NUM_OK);
  }
  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
    ((DOUBLE *)data)[i+1] = VVALUE(pv,Comp[i]);

  return (NUM_OK);
}

static int Scatter_ProjectVectorComp (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  INT i,type;
  const SHORT *Comp;

  if (((INT *)data)[0])
    return (NUM_OK);
  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv))
      VVALUE(pv,VD_SCALCMP(ConsVector)) = ((DOUBLE *)data)[1];

    return (NUM_OK);
  }
  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
    VVALUE(pv,Comp[i]) = ((DOUBLE *)data)[i+1];

  return (NUM_OK);
}

INT l_ghostvector_project (GRID *g, const VECDATA_DESC *x)
{
  INT tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));
  m++;

  DDD_IFAOneway(VectorVAllIF, GRID_ATTR(g), IF_FORWARD, m * sizeof(DOUBLE),
                Gather_ProjectVectorComp, Scatter_ProjectVectorComp);

  return (NUM_OK);
}
#endif

/****************************************************************************/
/*D
   l_vector_collect - collects the vector values of all copies

   SYNOPSIS:
   INT l_vector_collect (GRID *g, const VECDATA_DESC *x);

   PARAMETERS:
   .  g - pointer to grid
   .  x - vector data descriptor

   DESCRIPTION:
   This function collects the sum of the vector values for all border vectors
   to the master vector.

   RETURN VALUE:
   INT
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
   D*/
/****************************************************************************/

#ifdef ModelP
static int Gather_VectorCompCollect (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  INT vc,i,type;
  const SHORT *Comp;

  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv)) {
      vc = VD_SCALCMP(ConsVector);
      *((DOUBLE *)data) = VVALUE(pv,vc);
      VVALUE(pv,vc) = 0.0;
    }
    return (NUM_OK);
  }

  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++) {
    ((DOUBLE *)data)[i] = VVALUE(pv,Comp[i]);
    VVALUE(pv,Comp[i]) = 0.0;
  }

  return (NUM_OK);
}

INT l_vector_collect (GRID *g, const VECDATA_DESC *x)
{
  INT tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  DDD_IFAOneway(BorderVectorIF, GRID_ATTR(g), IF_FORWARD, m * sizeof(DOUBLE),
                Gather_VectorCompCollect, Scatter_VectorComp);

  return (NUM_OK);
}
#endif

/****************************************************************************/
/*D
   a_vector_collect - collects the vector values of all copies

   SYNOPSIS:
   INT a_vector_collect (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - from level
   .  x - vector data descriptor

   DESCRIPTION:
   This function collects the sum of the vector values for all border vectors
   to the master vector.

   RETURN VALUE:
   INT
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
   D*/
/****************************************************************************/

#ifdef ModelP
INT a_vector_collect (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x)
{
  INT level,tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  if ((fl==BOTTOMLEVEL(mg)) && (tl==TOPLEVEL(mg)))
    DDD_IFOneway(BorderVectorIF, IF_FORWARD, m * sizeof(DOUBLE),
                 Gather_VectorCompCollect, Scatter_VectorComp);
  else
    for (level=fl; level<=tl; level++)
      DDD_IFAOneway(BorderVectorIF,
                    GRID_ATTR(GRID_ON_LEVEL(mg,level)),
                    IF_FORWARD, m * sizeof(DOUBLE),
                    Gather_VectorCompCollect, Scatter_VectorComp);

  return (NUM_OK);
}
#endif

/****************************************************************************/
/*D
   a_vector_vecskip - checks vecskip flags

   SYNOPSIS:
   INT a_vector_vecskip (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - from level
   .  x - vector data descriptor

   DESCRIPTION:
   This function checks the vecskip flags and exchanges Dirichlet values.

   RETURN VALUE:
   INT
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
   D*/
/****************************************************************************/

#ifdef ModelP
static int Gather_VectorVecskip (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  INT i,type;
  const SHORT *Comp;

  ((DOUBLE *) data)[0] = VECSKIP(pv);
  if (VECSKIP(pv) == 0) return (NUM_OK);
  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv))
      ((DOUBLE *)data)[1] = VVALUE(pv,VD_SCALCMP(ConsVector));
    return (NUM_OK);
  }

  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
    ((DOUBLE *)data)[i+1] = VVALUE(pv,Comp[i]);

  return (NUM_OK);
}

static int Scatter_VectorVecskip (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  INT i,type;
  unsigned INT vecskip;
  const SHORT *Comp;

  vecskip = ((DOUBLE *) data)[0];
  if (vecskip == 0) return (NUM_OK);

  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv))
      if (vecskip) {
        if (VECSKIP(pv))
          VVALUE(pv,VD_SCALCMP(ConsVector)) = MAX(VVALUE(pv,VD_SCALCMP(ConsVector)),((DOUBLE *)data)[1]);
        else {
          VVALUE(pv,VD_SCALCMP(ConsVector)) = ((DOUBLE *)data)[1];
          VECSKIP(pv) = 1;
        }
      }
    return (NUM_OK);
  }
  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
    if ((vecskip & (1<<i))) {
      if ((VECSKIP(pv) & (1<<i)))
        VVALUE(pv,Comp[i]) = MAX(VVALUE(pv,Comp[i]),((DOUBLE *)data)[i+1]);
      else {
        VVALUE(pv,Comp[i]) = ((DOUBLE *)data)[i+1];
        VECSKIP(pv) |= (1<<i);
      }
    }

  return (NUM_OK);
}

INT a_vector_vecskip (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x)
{
  INT level,tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  m++;
  if ((fl==BOTTOMLEVEL(mg)) && (tl==TOPLEVEL(mg)))
    DDD_IFExchange(BorderVectorSymmIF, m * sizeof(DOUBLE),
                   Gather_VectorVecskip, Scatter_VectorVecskip);
  else
    for (level=fl; level<=tl; level++)
      DDD_IFAExchange(BorderVectorSymmIF,
                      GRID_ATTR(GRID_ON_LEVEL(mg,level)),
                      m * sizeof(DOUBLE),
                      Gather_VectorVecskip, Scatter_VectorVecskip);

  return (NUM_OK);
}
#endif

/****************************************************************************/
/*D
   l_ghostvector_collect - collects the vector values of all copies

   SYNOPSIS:
   INT l_ghostvector_collect (GRID *g, const VECDATA_DESC *x);

   PARAMETERS:
   .  g - pointer to grid
   .  x - vector data descriptor

   DESCRIPTION:
   This function collects the sum of the vector values for all ghost vectors
   to the master vector.

   RETURN VALUE:
   INT
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
   D*/
/****************************************************************************/

#ifdef ModelP
INT l_ghostvector_collect (GRID *g, const VECDATA_DESC *x)
{
  INT tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  DDD_IFAOneway(VectorVIF, GRID_ATTR(g), IF_BACKWARD, m * sizeof(DOUBLE),
                Gather_VectorCompCollect, Scatter_VectorComp);

  return (NUM_OK);
}
#endif

/****************************************************************************/
/*D
   l_ghostmatrix_collect - collects ghostmatrix entries for Galerkin assembling

   SYNOPSIS:
   INT l_ghostmatrix_collect (GRID *g, const MATDATA_DESC *A);

   PARAMETERS:
   .  g - pointer to grid
   .  A - matrix data descriptor

   DESCRIPTION:
   This function collects the matrix entries of ghost elements.
   It is called in 'AssembleGalerkinByMatrix'.

   RETURN VALUE:
   INT
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
   D*/
/****************************************************************************/

#ifdef ModelP
static int Gather_MatrixCollect (DDD_OBJ obj, void *data)
{
  ELEMENT *pe = (ELEMENT *)obj;
  DOUBLE *mptr[MAX_NODAL_VALUES*MAX_NODAL_VALUES];
  INT i,m;

  m = GetElementMPtrs(pe,ConsMatrix,mptr);
  if (m < 0)
    for (i=0; i<DataSizePerMatrix; i++)
      ((DOUBLE *)data)[i] = 0.0;
  else
    for (i=0; i<MIN(DataSizePerMatrix,m*m); i++) {
      ((DOUBLE *)data)[i] = *mptr[i];
      *mptr[i] = 0.0;
    }

  return (NUM_OK);
}

static int Scatter_MatrixCollect (DDD_OBJ obj, void *data)
{
  ELEMENT *pe = (ELEMENT *)obj;
  DOUBLE *mptr[MAX_NODAL_VALUES*MAX_NODAL_VALUES];
  INT i,m;

  m = GetElementMPtrs(pe,ConsMatrix,mptr);
  if (m < 0)
    return (NUM_ERROR);
  for (i=0; i<MIN(DataSizePerMatrix,m*m); i++)
    *mptr[i] += ((DOUBLE *)data)[i];

  return (NUM_OK);
}

INT l_ghostmatrix_collect (GRID *g, const MATDATA_DESC *A)
{
  INT rtp,ctp,m;

  ConsMatrix = (MATDATA_DESC *)A;
  m = 0;
  for (rtp=0; rtp<NVECTYPES; rtp++)
    m += MD_NCMPS_IN_RT_CT(ConsMatrix,rtp,rtp) * max_vectors_of_type[rtp];
  m = MIN(m,MAX_NODAL_VALUES);
  DataSizePerMatrix = m * m;

  DDD_IFAOneway(ElementVIF, GRID_ATTR(g), IF_BACKWARD,
                DataSizePerMatrix * sizeof(DOUBLE),
                Gather_MatrixCollect, Scatter_MatrixCollect);

  return (NUM_OK);
}
#endif

/****************************************************************************/
/*D
   l_amgmatrix_collect - collects ghostmatrix entries for AMG method

   SYNOPSIS:
   INT l_amgmatrix_collect (GRID *g, const MATDATA_DESC *A);

   PARAMETERS:
   .  g - pointer to grid
   .  A - matrix data descriptor

   DESCRIPTION:
   This function collects the matrix entries of vertical ghosts on the
   first AMG-level stored on one processor.
   It is called in 'AMGAgglomerate'.

   RETURN VALUE:
   INT
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
   D*/
/****************************************************************************/

#ifdef ModelP

static int Gather_AMGMatrixCollect (DDD_OBJ obj, void *data)
{
  VECTOR  *pv = (VECTOR *)obj;
  MATRIX  *m;
  DOUBLE  *msgbuf = (DOUBLE *)           data;
  INT     *maxgid = (INT *)    (((char *)data)+DataSizePerVector);
  DDD_GID *gidbuf = (DDD_GID *)(((char *)data)+DataSizePerVector+sizeof(INT));
  int i, *proclist,mc,vtype,mtype,masc;
  const SHORT *Comp;

  *maxgid = 0;

  if (VSTART(pv) == NULL) {
    return (NUM_OK);
  }

  if (MD_IS_SCALAR(ConsMatrix)) {
    if (MD_SCAL_RTYPEMASK(ConsMatrix)  & VDATATYPE(pv)) {
      if (VECSKIP(pv) != 0) return (NUM_OK);
      mc = MD_SCALCMP(ConsMatrix);
      masc =MD_SCAL_CTYPEMASK(ConsMatrix);
      for (m=VSTART(pv); m!=NULL; m=MNEXT(m))
      {
        *msgbuf = MVALUE(m,mc);
        msgbuf++;

        gidbuf[*maxgid] = DDD_InfoGlobalId(PARHDR(MDEST(m)));
        (*maxgid)++;
      }
      m=VSTART(pv);
      MVALUE(m,mc) = 1.0;
      for (m=MNEXT(m); m!=NULL; m=MNEXT(m))
        MVALUE(m,mc) = 0.0;
      return (NUM_OK);
    }
  }

  vtype = VTYPE(pv);
  for (m=(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
    mtype = MTP(vtype,MDESTTYPE(m));
    Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
    for (i=0; i<MD_COLS_IN_MTYPE(ConsMatrix,mtype)
         *MD_ROWS_IN_MTYPE(ConsMatrix,mtype); i++)
      msgbuf[i] = MVALUE(m,Comp[i]);
    msgbuf+=MaxBlockSize;

    gidbuf[*maxgid] = GID(MDEST(m));
    (*maxgid)++;
  }
  for (m=(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
    mtype = MTP(vtype,MDESTTYPE(m));
    Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
    for (i=0; i<MD_COLS_IN_MTYPE(ConsMatrix,mtype)
         *MD_ROWS_IN_MTYPE(ConsMatrix,mtype); i++)
      MVALUE(m,Comp[i]) = 0.0;
  }

  return (NUM_OK);
}

static int Scatter_AMGMatrixCollect (DDD_OBJ obj, void *data)
{
  VECTOR  *pv = (VECTOR *)obj;
  MATRIX  *m;
  DOUBLE  *msgbuf = (DOUBLE *)           data;
  INT     *maxgid = (INT *)    (((char *)data)+DataSizePerVector);
  DDD_GID *gidbuf = (DDD_GID *)(((char *)data)+DataSizePerVector+sizeof(INT));
  INT igid = 0;
  int i,j,k, *proclist,mc,vtype,mtype,ncomp,rcomp,vecskip,masc;
  const SHORT *Comp;


  if (VSTART(pv) == NULL) return (NUM_OK);
  if (MD_IS_SCALAR(ConsMatrix)) {
    printf("%3d: MD_IS_SCALAR(ConsMatrix)\n",me);
    if (MD_SCAL_RTYPEMASK(ConsMatrix)  & VDATATYPE(pv))
    {
      if (VECSKIP(pv) != 0) return (NUM_OK);
      mc = MD_SCALCMP(ConsMatrix);
      masc =MD_SCAL_CTYPEMASK(ConsMatrix);
      for (m=VSTART(pv); m!=NULL; m=MNEXT(m)) {
        DDD_GID dest = DDD_InfoGlobalId(PARHDR(MDEST(m)));

        if (igid<*maxgid && (gidbuf[igid]==dest)) {
          MVALUE(m,mc) += *msgbuf;
          msgbuf++;
          igid++;
        }
      }
    }
    return (NUM_OK);
  }

  vtype = VTYPE(pv);
  vecskip = VECSKIP(pv);
  rcomp = MD_ROWS_IN_MTYPE(ConsMatrix,MTP(vtype,vtype));
  if (vecskip == 0)
    for (m=(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
      DDD_GID dest = GID(MDEST(m));

      while (igid<*maxgid && (gidbuf[igid]<dest))
      {
        msgbuf+=MaxBlockSize;
        igid++;
      }

      if (igid<*maxgid && (gidbuf[igid]==dest))
      {
        mtype = MTP(vtype,MDESTTYPE(m));
        Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
        for (j=0; j<MD_COLS_IN_MTYPE(ConsMatrix,mtype)*rcomp; j++)
          MVALUE(m,Comp[j]) += msgbuf[j];
        msgbuf+=MaxBlockSize;
        igid++;
      }
    }
  else
    for (m=(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
      DDD_GID dest = DDD_InfoGlobalId(PARHDR(MDEST(m)));

      while (igid<*maxgid && (gidbuf[igid]<dest))
      {
        msgbuf+=MaxBlockSize;
        igid++;
      }

      if (igid<*maxgid && (gidbuf[igid]==dest))
      {
        mtype = MTP(vtype,MDESTTYPE(m));
        ncomp = MD_COLS_IN_MTYPE(ConsMatrix,mtype);
        Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
        for (k=0; k<rcomp; k++)
          if (!(vecskip & (1<<k)))
            for (j=k*ncomp; j<(k+1)*ncomp; j++)
              MVALUE(m,Comp[j]) += msgbuf[j];
        msgbuf+=MaxBlockSize;
        igid++;
      }
    }

  IFDEBUG(np,2)
  igid = 0;
  msgbuf = (DOUBLE *)data;
  for (m=(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
    DDD_GID dest = DDD_InfoGlobalId(PARHDR(MDEST(m)));

    while (igid<*maxgid && (gidbuf[igid]<dest))  {
      msgbuf+=MaxBlockSize;
      igid++;
    }

    if (igid<*maxgid && (gidbuf[igid]==dest)) {
      printf("%d: %d->%d:",me,GID(pv),GID(MDEST(m)));
      mtype = MTP(vtype,MDESTTYPE(m));
      ncomp = MD_COLS_IN_MTYPE(ConsMatrix,mtype);
      Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
      for (k=0; k<rcomp; k++)
        for (j=k*ncomp; j<(k+1)*ncomp; j++)
          printf(" %f",MVALUE(m,Comp[j]));
      msgbuf+=MaxBlockSize;
      igid++;
      printf("\n");
    }
  }
  ENDDEBUG

  return (NUM_OK);
}

static int sort_MatArray (const void *e1, const void *e2)
{
  MATRIX  *m1 = *((MATRIX **)e1);
  MATRIX  *m2 = *((MATRIX **)e2);
  DDD_GID g1 = DDD_InfoGlobalId(PARHDR(MDEST(m1)));
  DDD_GID g2 = DDD_InfoGlobalId(PARHDR(MDEST(m2)));

  if (g1<g2) return(-1);
  if (g1>g2) return(1);
  return (NUM_OK);
}


static int CountAndSortMatrices (DDD_OBJ obj)
{
  VECTOR *pv = (VECTOR *)obj;
  MATRIX *m;
  int n, j;

  /* sort MATRIX-list according to gid of destination vector */
  if (VSTART(pv) == NULL)
    return(0);
  n = 0;
  ASSERT(MDEST(VSTART(pv))==pv);

  for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m))
    MatArrayRemote[n++] = m;
  if (n>1) {
    qsort(MatArrayRemote,MIN(n,MATARRAYSIZE),
          sizeof(MATRIX *),sort_MatArray);
    m=VSTART(pv);
    for(j=0; j<n; j++) {
      MNEXT(m) = MatArrayRemote[j];
      m = MNEXT(m);
    }
    MNEXT(m)=NULL;
  }
  n++;
  if (PRIO(pv) == PrioVGhost)
    if (MaximumInconsMatrices < n)
      MaximumInconsMatrices = n;

  return(0);
}

INT l_amgmatrix_collect (GRID *g, const MATDATA_DESC *A)
{
  INT mt;
  size_t sizePerVector;

  PRINTDEBUG(np,2,("%3d: entering l_amgmatrix_collect...\n",me));
  PRINTDEBUG(np,2,("%3d: Gridlevel %d\n",me,GLEVEL(g)));

  ConsMatrix = (MATDATA_DESC *)A;
  MaxBlockSize = 0;
  for (mt=0; mt<NMATTYPES; mt++)
    MaxBlockSize = MAX(MaxBlockSize,MD_COLS_IN_MTYPE(ConsMatrix,mt)
                       *MD_ROWS_IN_MTYPE(ConsMatrix,mt));
  MaximumInconsMatrices=0;
  DDD_IFAExecLocal(VectorVIF, GRID_ATTR(g), CountAndSortMatrices);
  MaximumInconsMatrices = UG_GlobalMaxINT(MaximumInconsMatrices);
  DataSizePerVector = MaximumInconsMatrices * MaxBlockSize * sizeof(DOUBLE);
  DataSizePerVector = CEIL(DataSizePerVector);

  PRINTDEBUG(np,2,("%3d: MaximumInconsMatrices: %d\n",me,MaximumInconsMatrices));
  PRINTDEBUG(np,2,("%3d: MaxBlockSize: %d\n",me,MaxBlockSize));
  PRINTDEBUG(np,2,("%3d: DataSizePerVector: %d\n",me,DataSizePerVector));

  /* overall data sent per vector is its matrix entry data plus
     the number of valid entries plus a table of DDD-GIDs of
          destination vectors */
  sizePerVector = DataSizePerVector + sizeof(INT)
                  + MaximumInconsMatrices*sizeof(DDD_GID);
  sizePerVector = CEIL(sizePerVector);

  PRINTDEBUG(np,2,("%3d: sizePerVector: %d\n",me,sizePerVector));

  DDD_IFAOneway(VectorVIF, GRID_ATTR(g), IF_BACKWARD, sizePerVector,
                Gather_AMGMatrixCollect, Scatter_AMGMatrixCollect);

  PRINTDEBUG(np,2,("%3d: exiting l_amgmatrix_collect...\n",me));

  return (NUM_OK);
}
#endif

/****************************************************************************/
/*D
   l_vector_meanvalue - averages the vector values of all copies

   SYNOPSIS:
   INT l_vector_meanvalue (GRID *g, const VECDATA_DESC *x);

   PARAMETERS:
   .  g - pointer to grid
   .  x - vector data descriptor

   DESCRIPTION:
   This function builds the mean value of all vector values on border vectors.

   RETURN VALUE:
   INT
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
   D*/
/****************************************************************************/

#ifdef ModelP
int DDD_InfoPrioCopies (DDD_HDR hdr)
{
  INT i,n;
  int *proclist;

  if (DDD_InfoNCopies(hdr) == 0)
    return(0);

  proclist = DDD_InfoProcList(hdr);
  n = 0;
  for(i=2; proclist[i]>=0; i+=2)
    if (!GHOSTPRIO(proclist[i+1]))
      n++;

  return(n);
}

static INT l_vector_average (GRID *g, const VECDATA_DESC *x)
{
  VECTOR *v;
  DOUBLE fac;
  INT vc,i,type,mask,n,m,vecskip;
  const SHORT *Comp;

  if (VD_IS_SCALAR(x)) {
    mask = VD_SCALTYPEMASK(x);
    vc = VD_SCALCMP(x);
    for (v=FIRSTVECTOR(g); v!= NULL; v=SUCCVC(v))
      if ((VECSKIP(v) == 0) && (mask & VDATATYPE(v))) {
        m = DDD_InfoPrioCopies(PARHDR(v));
        if (m > 0)
          VVALUE(v,vc) *= 1.0 / (m+1.0);
      }
  }
  else
    for (v=FIRSTVECTOR(g); v!= NULL; v=SUCCVC(v)) {
      type = VTYPE(v);
      n = VD_NCMPS_IN_TYPE(x,type);
      if (n == 0) continue;
      vecskip = VECSKIP(v);
      Comp = VD_CMPPTR_OF_TYPE(x,type);
      m = DDD_InfoPrioCopies(PARHDR(v));
      if (m == 0) continue;
      fac = 1.0 / (m + 1.0);
      if (vecskip == 0)
        for (i=0; i<n; i++)
          VVALUE(v,Comp[i]) *= fac;
      else
        for (i=0; i<n; i++)
          if (!(vecskip & (1<<i)))
            VVALUE(v,Comp[i]) *= fac;
    }

  return(NUM_OK);
}

INT l_vector_meanvalue (GRID *g, const VECDATA_DESC *x)
{
  INT tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  DDD_IFAExchange(BorderVectorSymmIF, GRID_ATTR(g), m * sizeof(DOUBLE),
                  Gather_VectorComp, Scatter_VectorComp);

  if (l_vector_average(g,x) != NUM_OK)
    REP_ERR_RETURN(NUM_ERROR);

  return (NUM_OK);
}
#endif

/****************************************************************************/
/*D
   a_vector_meanvalue - averages the vector values of all copies

   SYNOPSIS:
   INT a_vector_meanvalue (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - from level
   .  x - vector data descriptor

   DESCRIPTION:
   This function builds the mean value of all vector values on border vectors.

   RETURN VALUE:
   INT
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
   D*/
/****************************************************************************/

#ifdef ModelP
INT a_vector_meanvalue (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x)
{
  INT level,tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  if ((fl==BOTTOMLEVEL(mg)) && (tl==TOPLEVEL(mg)))
    DDD_IFExchange(BorderVectorSymmIF, m * sizeof(DOUBLE),
                   Gather_VectorComp, Scatter_VectorComp);
  else
    for (level=fl; level<=tl; level++)
      DDD_IFAExchange(BorderVectorSymmIF,
                      GRID_ATTR(GRID_ON_LEVEL(mg,level)),
                      m * sizeof(DOUBLE),
                      Gather_VectorComp, Scatter_VectorComp);

  for (level=fl; level<=tl; level++)
    if (l_vector_average(GRID_ON_LEVEL(mg,level),x) != NUM_OK)
      REP_ERR_RETURN(NUM_ERROR);

  return (NUM_OK);
}
#endif

/****************************************************************************/
/*D
   l_matrix_consistent - builds the sum of the matrix values on all copies

   SYNOPSIS:
   INT l_matrix_consistent (GRID *g, const MATDATA_DESC *M, INT mode);

   PARAMETERS:
   .  g - pointer to grid
   .  M - matrix data descriptor
   .  mode - consistence of the diagonal (MAT_DIAG_CONS), the complete row for
          all master vectors (MAT_MASTER_CONS) or all (MAT_CONS)

   DESCRIPTION:
   This function builds the sum of the matrix values for
   the matrix list of all border vectors.

   RETURN VALUE:
   INT
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
   D*/
/****************************************************************************/

#ifdef ModelP
static int Gather_DiagMatrixComp (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  MATRIX *m;
  INT mc,i,vtype,mtype;
  const SHORT *Comp;

  if (MD_IS_SCALAR(ConsMatrix)) {
    if (MD_SCAL_RTYPEMASK(ConsMatrix) & VDATATYPE(pv))
      *((DOUBLE *)data) = MVALUE(VSTART(pv),MD_SCALCMP(ConsMatrix));
    return (NUM_OK);
  }

  vtype = VTYPE(pv);
  mtype = MTP(vtype,vtype);
  Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
  m = VSTART(pv);
  for (i=0; i<MD_COLS_IN_MTYPE(ConsMatrix,mtype)
       *MD_ROWS_IN_MTYPE(ConsMatrix,mtype); i++)
    ((DOUBLE *)data)[i] = MVALUE(m,Comp[i]);

  return (NUM_OK);
}

static int Scatter_DiagMatrixComp (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  MATRIX *m;
  INT mc,i,j,vtype,mtype,ncomp,vecskip;
  const SHORT *Comp;

  if (MD_IS_SCALAR(ConsMatrix)) {
    if (MD_SCAL_RTYPEMASK(ConsMatrix) & VDATATYPE(pv))
      if (!VECSKIP(pv))
        MVALUE(VSTART(pv),MD_SCALCMP(ConsMatrix)) += *((DOUBLE *)data);
    return (NUM_OK);
  }

  vtype = VTYPE(pv);
  mtype = MTP(vtype,vtype);
  Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
  m = VSTART(pv);
  vecskip = VECSKIP(pv);
  if (vecskip == 0)
    for (i=0; i<MD_COLS_IN_MTYPE(ConsMatrix,mtype)
         *MD_ROWS_IN_MTYPE(ConsMatrix,mtype); i++)
      MVALUE(m,Comp[i]) += ((DOUBLE *)data)[i];
  else {
    ncomp = MD_COLS_IN_MTYPE(ConsMatrix,mtype);
    for (i=0; i<MD_ROWS_IN_MTYPE(ConsMatrix,mtype); i++)
      if (!(vecskip & (1<<i)))
        for (j=i*ncomp; j<(i+1)*ncomp; j++)
          MVALUE(m,Comp[j]) += ((DOUBLE *)data)[j];
  }

  return (NUM_OK);
}

static int Scatter_GhostDiagMatrixComp (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  MATRIX *m;
  INT mc,i,j,vtype,mtype,ncomp,vecskip;
  const SHORT *Comp;

  m = VSTART(pv);
  if (m == NULL)
    m = CreateExtraConnection(ConsGrid,pv,pv);
  if (m == NULL)
    return(1);

  if (MD_IS_SCALAR(ConsMatrix)) {
    if (MD_SCAL_RTYPEMASK(ConsMatrix) & VDATATYPE(pv))
      MVALUE(m,MD_SCALCMP(ConsMatrix)) = *((DOUBLE *)data);
    return (NUM_OK);
  }

  vtype = VTYPE(pv);
  mtype = MTP(vtype,vtype);
  Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
  for (i=0; i<MD_COLS_IN_MTYPE(ConsMatrix,mtype)
       *MD_ROWS_IN_MTYPE(ConsMatrix,mtype); i++)
    MVALUE(m,Comp[i]) = ((DOUBLE *)data)[i];

  return (NUM_OK);
}

static int Gather_OffDiagMatrixComp (DDD_OBJ obj, void *data,
                                     DDD_PROC proc, DDD_PRIO prio)
{
  VECTOR  *pv = (VECTOR *)obj;
  MATRIX  *m;
  DOUBLE  *msgbuf = (DOUBLE *)           data;
  INT     *maxgid = (INT *)    (((char *)data)+DataSizePerVector);
  DDD_GID *gidbuf = (DDD_GID *)(((char *)data)+DataSizePerVector+sizeof(INT));
  int i, *proclist,mc,vtype,mtype,masc;
  const SHORT *Comp;

  *maxgid = 0;

  if (VSTART(pv) == NULL) return (NUM_OK);
  if (MD_IS_SCALAR(ConsMatrix)) {
    if (MD_SCAL_RTYPEMASK(ConsMatrix)  & VDATATYPE(pv)) {
      if (VECSKIP(pv) != 0) return (NUM_OK);
      mc = MD_SCALCMP(ConsMatrix);
      masc =MD_SCAL_CTYPEMASK(ConsMatrix);
      for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
        if (XFERMATX(m)==0) break;
        if (masc  & VDATATYPE(MDEST(m))) {
          proclist = DDD_InfoProcList(PARHDR(MDEST(m)));
          for(i=2; proclist[i]>=0 && ((DDD_PROC)proclist[i])!=proc; i+=2)
            ;
          if (((DDD_PROC)proclist[i])==proc &&
              (!GHOSTPRIO(proclist[i+1])))
          {
            *msgbuf = MVALUE(m,mc);
            msgbuf++;

            gidbuf[*maxgid] = DDD_InfoGlobalId(PARHDR(MDEST(m)));
            (*maxgid)++;
          }
        }
      }
    }
    return (NUM_OK);
  }

  vtype = VTYPE(pv);
  for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
    if (XFERMATX(m)==0) break;
    proclist = DDD_InfoProcList(PARHDR(MDEST(m)));
    for(i=2; proclist[i]>=0 && ((DDD_PROC)proclist[i])!=proc; i+=2)
      ;
    if (((DDD_PROC)proclist[i])==proc &&
        (!GHOSTPRIO(proclist[i+1])))
    {
      mtype = MTP(vtype,MDESTTYPE(m));
      Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
      for (i=0; i<MD_COLS_IN_MTYPE(ConsMatrix,mtype)
           *MD_ROWS_IN_MTYPE(ConsMatrix,mtype); i++)
        msgbuf[i] = MVALUE(m,Comp[i]);
      msgbuf+=MaxBlockSize;

      gidbuf[*maxgid] = DDD_InfoGlobalId(PARHDR(MDEST(m)));
      (*maxgid)++;
    }
  }

  return (NUM_OK);
}

static int Gather_OffDiagMatrixCompCollect (DDD_OBJ obj, void *data,
                                            DDD_PROC proc, DDD_PRIO prio)
{
  VECTOR  *pv = (VECTOR *)obj;
  MATRIX  *m;
  DOUBLE  *msgbuf = (DOUBLE *)           data;
  INT     *maxgid = (INT *)    (((char *)data)+DataSizePerVector);
  DDD_GID *gidbuf = (DDD_GID *)(((char *)data)+DataSizePerVector+sizeof(INT));
  int i, *proclist,mc,vtype,mtype,masc;
  const SHORT *Comp;

  *maxgid = 0;

  if (VSTART(pv) == NULL) return (NUM_OK);
  if (MD_IS_SCALAR(ConsMatrix)) {
    if (MD_SCAL_RTYPEMASK(ConsMatrix)  & VDATATYPE(pv)) {
      if (VECSKIP(pv) != 0) return (NUM_OK);
      mc = MD_SCALCMP(ConsMatrix);
      masc =MD_SCAL_CTYPEMASK(ConsMatrix);
      for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m))
      {
        if (XFERMATX(m)==0) break;

        if (masc  & VDATATYPE(MDEST(m))) {
          proclist = DDD_InfoProcList(PARHDR(MDEST(m)));
          for(i=2; proclist[i]>=0 && ((DDD_PROC)proclist[i])!=proc;
              i+=2)
            ;
          if (((DDD_PROC)proclist[i])==proc &&
              (!GHOSTPRIO(proclist[i+1])))
          {
            *msgbuf = MVALUE(m,mc);
            msgbuf++;

            gidbuf[*maxgid] = DDD_InfoGlobalId(PARHDR(MDEST(m)));
            (*maxgid)++;
          }
        }
      }
      for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m))
        if (masc  & VDATATYPE(MDEST(m)))
          MVALUE(m,mc) = 0.0;
    }
    return (NUM_OK);
  }

  vtype = VTYPE(pv);
  for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
    if (XFERMATX(m)==0) break;
    proclist = DDD_InfoProcList(PARHDR(MDEST(m)));
    for(i=2; proclist[i]>=0 && ((DDD_PROC)proclist[i])!=proc; i+=2)
      ;
    if (((DDD_PROC)proclist[i])==proc &&
        (!GHOSTPRIO(proclist[i+1])))
    {
      mtype = MTP(vtype,MDESTTYPE(m));
      Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
      for (i=0; i<MD_COLS_IN_MTYPE(ConsMatrix,mtype)
           *MD_ROWS_IN_MTYPE(ConsMatrix,mtype); i++)
        msgbuf[i] = MVALUE(m,Comp[i]);
      msgbuf+=MaxBlockSize;

      gidbuf[*maxgid] = DDD_InfoGlobalId(PARHDR(MDEST(m)));
      (*maxgid)++;
    }
  }
  for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
    mtype = MTP(vtype,MDESTTYPE(m));
    Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
    for (i=0; i<MD_COLS_IN_MTYPE(ConsMatrix,mtype)
         *MD_ROWS_IN_MTYPE(ConsMatrix,mtype); i++)
      MVALUE(m,Comp[i]) = 0.0;
  }


  return (NUM_OK);
}

static int Scatter_OffDiagMatrixComp (DDD_OBJ obj, void *data,
                                      DDD_PROC proc, DDD_PRIO prio)
{
  VECTOR  *pv = (VECTOR *)obj;
  MATRIX  *m;
  DOUBLE  *msgbuf = (DOUBLE *)           data;
  INT     *maxgid = (INT *)    (((char *)data)+DataSizePerVector);
  DDD_GID *gidbuf = (DDD_GID *)(((char *)data)+DataSizePerVector+sizeof(INT));
  INT igid = 0;
  int i,j,k, *proclist,mc,vtype,mtype,ncomp,rcomp,vecskip,masc;
  const SHORT *Comp;

  PRINTDEBUG(np,1,("%d: Scatter_OffDiagMatrixComp %d: maxgid %d\n",
                   me,GID(pv),*maxgid));

  if (VSTART(pv) == NULL) return (NUM_OK);
  if (MD_IS_SCALAR(ConsMatrix)) {
    if (MD_SCAL_RTYPEMASK(ConsMatrix)  & VDATATYPE(pv))
    {
      if (VECSKIP(pv) != 0) return (NUM_OK);
      mc = MD_SCALCMP(ConsMatrix);
      masc =MD_SCAL_CTYPEMASK(ConsMatrix);
      for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m))
      {
        if (XFERMATX(m)==0) break;
        if (masc  & VDATATYPE(MDEST(m))) {
          proclist = DDD_InfoProcList(PARHDR(MDEST(m)));
          for(i=2; proclist[i]>=0 &&
              ((DDD_PROC)proclist[i])!=proc; i+=2)
            ;
          if (((DDD_PROC)proclist[i])==proc &&
              (!GHOSTPRIO(proclist[i+1])))
          {
            DDD_GID dest = DDD_InfoGlobalId(PARHDR(MDEST(m)));

            while (igid<*maxgid && (gidbuf[igid]<dest))
            {
              msgbuf++;
              igid++;
            }

            if (igid<*maxgid && (gidbuf[igid]==dest))
            {
              MVALUE(m,mc) += *msgbuf;
              msgbuf++;
              igid++;
            }
          }
        }
      }
    }
    return (NUM_OK);
  }

  vtype = VTYPE(pv);
  vecskip = VECSKIP(pv);
  rcomp = MD_ROWS_IN_MTYPE(ConsMatrix,MTP(vtype,vtype));
  if (vecskip == 0)
    for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
      if (XFERMATX(m)==0) break;
      proclist = DDD_InfoProcList(PARHDR(MDEST(m)));
      for (i=2; proclist[i]>=0 && ((DDD_PROC)proclist[i])!=proc; i+=2)
        ;
      if (((DDD_PROC)proclist[i])==proc &&
          (!GHOSTPRIO(proclist[i+1])))
      {
        DDD_GID dest = DDD_InfoGlobalId(PARHDR(MDEST(m)));

        while (igid<*maxgid && (gidbuf[igid]<dest))
        {
          msgbuf+=MaxBlockSize;
          igid++;
        }

        if (igid<*maxgid && (gidbuf[igid]==dest))
        {
          mtype = MTP(vtype,MDESTTYPE(m));
          Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
          for (j=0; j<MD_COLS_IN_MTYPE(ConsMatrix,mtype)*rcomp; j++)
            MVALUE(m,Comp[j]) += msgbuf[j];
          msgbuf+=MaxBlockSize;
          igid++;
        }
      }
    }
  else
    for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
      if (XFERMATX(m)==0) break;
      proclist = DDD_InfoProcList(PARHDR(MDEST(m)));
      for(i=2; proclist[i]>=0 && ((DDD_PROC)proclist[i])!=proc; i+=2)
        ;
      if (((DDD_PROC)proclist[i])==proc &&
          (!GHOSTPRIO(proclist[i+1])))
      {
        DDD_GID dest = DDD_InfoGlobalId(PARHDR(MDEST(m)));

        while (igid<*maxgid && (gidbuf[igid]<dest))
        {
          msgbuf+=MaxBlockSize;
          igid++;
        }

        if (igid<*maxgid && (gidbuf[igid]==dest))
        {
          mtype = MTP(vtype,MDESTTYPE(m));
          ncomp = MD_COLS_IN_MTYPE(ConsMatrix,mtype);
          Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
          for (k=0; k<rcomp; k++)
            if (!(vecskip & (1<<k)))
              for (j=k*ncomp; j<(k+1)*ncomp; j++)
                MVALUE(m,Comp[j]) += msgbuf[j];
          msgbuf+=MaxBlockSize;
          igid++;
        }
      }
    }

  IFDEBUG(np,1)
  igid = 0;
  msgbuf = (DOUBLE *)data;
  for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
    if (XFERMATX(m)==0) break;
    proclist = DDD_InfoProcList(PARHDR(MDEST(m)));
    for(i=2; proclist[i]>=0 && ((DDD_PROC)proclist[i])!=proc; i+=2)
      ;
    if (((DDD_PROC)proclist[i])==proc &&
        (!GHOSTPRIO(proclist[i+1])))
    {
      DDD_GID dest = DDD_InfoGlobalId(PARHDR(MDEST(m)));

      while (igid<*maxgid && (gidbuf[igid]<dest))
      {
        msgbuf+=MaxBlockSize;
        igid++;
      }

      if (igid<*maxgid && (gidbuf[igid]==dest))
      {
        printf("%d: %d->%d:",me,GID(pv),GID(MDEST(m)));
        mtype = MTP(vtype,MDESTTYPE(m));
        ncomp = MD_COLS_IN_MTYPE(ConsMatrix,mtype);
        Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
        for (k=0; k<rcomp; k++)
          for (j=k*ncomp; j<(k+1)*ncomp; j++)
            printf(" %f",MVALUE(m,Comp[j]));
        msgbuf+=MaxBlockSize;
        igid++;
        printf("\n");
      }
    }
  }
  ENDDEBUG

  return (NUM_OK);
}

static int ClearOffDiagCompOfCopies (GRID *theGrid, const MATDATA_DESC *M)
{
  VECTOR *v;
  MATRIX *m;
  INT i,j,k,mc,vtype,mtype,ncomp,rcomp,vecskip,masc;
  const SHORT *Comp;

  if (MD_IS_SCALAR(M)) {
    for (v=FIRSTVECTOR(theGrid); v!=NULL; v=PREDVC(v)) {
      if (VSTART(v) == NULL) continue;
      if (!MASTER(v)) continue;
      if (MD_SCAL_RTYPEMASK(M)  & VDATATYPE(v)) {
        if (VECSKIP(v) != 0) continue;
        mc = MD_SCALCMP(M);
        masc =MD_SCAL_CTYPEMASK(M);
        for (m=MNEXT(VSTART(v)); m!=NULL; m=MNEXT(m))
          if (masc  & VDATATYPE(MDEST(m)))
            MVALUE(m,mc) = 0.0;
      }
    }
    return (NUM_OK);
  }

  for (v=FIRSTVECTOR(theGrid); v!=NULL; v=PREDVC(v)) {
    if (VSTART(v) == NULL) continue;
    if (!MASTER(v)) continue;
    vtype = VTYPE(v);
    vecskip = VECSKIP(v);
    rcomp = MD_ROWS_IN_MTYPE(M,MTP(vtype,vtype));
    for (m=MNEXT(VSTART(v)); m!=NULL; m=MNEXT(m)) {
      mtype = MTP(vtype,MDESTTYPE(m));
      Comp = MD_MCMPPTR_OF_MTYPE(M,mtype);
      for (j=0; j<MD_COLS_IN_MTYPE(M,mtype)*rcomp; j++)
        MVALUE(m,Comp[j]) = 0.0;
    }
  }

  return (NUM_OK);
}

static int CountAndSortInconsMatrices (DDD_OBJ obj)
{
  VECTOR *pv = (VECTOR *)obj;
  MATRIX *m;
  int nLocal, nRemote, j;

  /* sort MATRIX-list according to gid of destination vector */
  nLocal=nRemote=0;
  if (VSTART(pv)!=NULL)
  {
    ASSERT(MDEST(VSTART(pv))==pv);

    for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m))
    {
      int i, *proclist = DDD_InfoProcList(PARHDR(MDEST(m)));
      for(i=0; proclist[i]>=0 && GHOSTPRIO(proclist[i+1]); i+=2)
        ;
      ASSERT(MDEST(m)!=pv);

      if (proclist[i]<0)
      {
        /* MDEST has only copies with PrioGhost (if any) */
        MatArrayLocal[nLocal++] = m;
      }
      else
      {
        /* MDEST has copies != PrioGhost */
        MatArrayRemote[nRemote++] = m;
      }
    }
  }
  if (nRemote>0)
  {
    if (nRemote>1)
      qsort(MatArrayRemote,MIN(nRemote,MATARRAYSIZE),sizeof(MATRIX *),sort_MatArray);

    m=VSTART(pv);
    for(j=0; j<nRemote; j++)
    {
      MNEXT(m) = MatArrayRemote[j];
      m = MNEXT(m);
      SETXFERMATX(m, 1);
    }
    for(j=0; j<nLocal; j++)
    {
      MNEXT(m) = MatArrayLocal[j];
      m = MNEXT(m);
      SETXFERMATX(m, 0);
    }
    MNEXT(m)=NULL;
  }
  else
  {
    if (VSTART(pv) != NULL)
      if (MNEXT(VSTART(pv))!=NULL)
        SETXFERMATX(MNEXT(VSTART(pv)), 0);
  }

  /* TODO: MaximumInconsMatrices ist eigentlich <nRemote, naemlich
          das maximum der teilmenge aus der matrixliste mit einer
          Kopie von MDEST auf processor proc, hier vernachlaessigt. */
  if (MaximumInconsMatrices<nRemote)
    MaximumInconsMatrices = nRemote;
}


/* TODO: perhaps it would make sense to have two routines,
        one for diagonal matrix entries only and the other for
        diag. and off-diag. matrix entries.
 */

INT l_matrix_consistent (GRID *g, const MATDATA_DESC *M, INT mode)
{
  INT mt;
  size_t sizePerVector;

  ConsMatrix = (MATDATA_DESC *)M;
  MaxBlockSize = 0;
  for (mt=0; mt<NMATTYPES; mt++)
    MaxBlockSize = MAX(MaxBlockSize,MD_COLS_IN_MTYPE(ConsMatrix,mt)
                       *MD_ROWS_IN_MTYPE(ConsMatrix,mt));

  PRINTDEBUG(np,1,("%2d: l_matrix_consistent mode\n",me,mode));

  /* TODO: make consistency of diags and off-diags in one communication! */

  DDD_IFAExchange(BorderVectorSymmIF, GRID_ATTR(g),
                  MaxBlockSize*sizeof(DOUBLE),
                  Gather_DiagMatrixComp, Scatter_DiagMatrixComp);

  if (mode == MAT_DIAG_CONS) return (NUM_OK);

  if (mode == MAT_GHOST_DIAG_CONS) {
    ConsGrid = g;
    DDD_IFAOneway(VectorVIF, GRID_ATTR(g), IF_FORWARD,
                  MaxBlockSize * sizeof(DOUBLE),
                  Gather_DiagMatrixComp, Scatter_GhostDiagMatrixComp);
    return (NUM_OK);
  }

  /* now make off-diagonal entries consistent */
  MaximumInconsMatrices=0;
  DDD_IFAExecLocal(BorderVectorSymmIF, GRID_ATTR(g), CountAndSortInconsMatrices);
  MaximumInconsMatrices = UG_GlobalMaxINT(MaximumInconsMatrices);
  DataSizePerVector = MaximumInconsMatrices * MaxBlockSize * sizeof(DOUBLE);
  DataSizePerVector = CEIL(DataSizePerVector);

  /* overall data sent per vector is its matrix entry data plus
     the number of valid entries plus a table of DDD-GIDs of
          destination vectors */
  sizePerVector = DataSizePerVector +
                  sizeof(INT) + MaximumInconsMatrices*sizeof(DDD_GID);
  sizePerVector = CEIL(sizePerVector);

  if (mode == MAT_CONS) {
    PRINTDEBUG(np,1,("%d: MAT_CONS\n",me));
    DDD_IFAExchangeX(BorderVectorSymmIF, GRID_ATTR(g), sizePerVector,
                     Gather_OffDiagMatrixComp, Scatter_OffDiagMatrixComp);
  }
  else if (mode == MAT_MASTER_CONS)
  {
    DDD_IFAOnewayX(BorderVectorIF, GRID_ATTR(g),IF_FORWARD, sizePerVector,
                   Gather_OffDiagMatrixCompCollect,
                   Scatter_OffDiagMatrixComp);
  }

  return (NUM_OK);
}
#endif /* ModelP */

/****************************************************************************/
/*																			*/
/*		blas level 1 routines												*/
/*																			*/
/****************************************************************************/

#ifdef __MWCW__
#pragma mark blas_level_0
#endif

/****************************************************************************/
/*D
   dset - set the given components of a vector to a given value

   SYNOPSIS:
   INT dset (MULTIGRID *mg, INT fl, INT tl, INT mode, VECDATA_DESC *x, DOUBLE a);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  x - vector data descriptor
   .  a - the DOUBLE value

   DESCRIPTION:
   This function sets the given components of a vector to a given value.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dsetBS
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   dsetBS - set one component of a vector to a given value

   SYNOPSIS:
   INT dsetBS (const BLOCKVECTOR *bv, INT xc, DOUBLE a);

   PARAMETERS:
   .  bv - BLOCKVECTOR specifying the vector list
   .  xc - component in the VECTOR
   .  a - the DOUBLE value

   DESCRIPTION:
   This function sets all components of a vector to a given value.

   RETURN VALUE:
   INT
   .n    NUM_OK
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dset
   D*/
/****************************************************************************/

#define T_FUNCNAME      dset
#define T_ARGS          ,DOUBLE a
#define T_MOD_SCAL      VVALUE(v,xc) = a;
#define T_MOD_VECTOR_1  VVALUE(v,cx0) = a;
#define T_MOD_VECTOR_2  VVALUE(v,cx1) = a;
#define T_MOD_VECTOR_3  VVALUE(v,cx2) = a;
#define T_MOD_VECTOR_N  for (i=0; i<ncomp; i++)                                 \
    VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i))=a;

#include "vecfunc.ct"

/****************************************************************************/
/*D
   dcopy - copy a vector

   SYNOPSIS:
   INT dcopy (MULTIGRID *mg, INT fl, INT tl, INT mode, VECDATA_DESC *x,
   VECDATA_DESC *y);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  x - destination vector data descriptor
   .  y - source vector data descriptor

   DESCRIPTION:
   This function copies a vector to another: `x := y`.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dcopyBS
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   dcopyBS - copy a vector

   SYNOPSIS:
   INT dcopyBS (const BLOCKVECTOR *bv, INT xc, INT yc);

   PARAMETERS:
   .  bv - BLOCKVECTOR specifying the vector list
   .  xc - component in the destination VECTOR
   .  yc - component in the source VECTOR

   DESCRIPTION:
   This function copies a vector to another: `x := y`.

   RETURN VALUE:
   INT
   .n    NUM_OK
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dcopy
   D*/
/****************************************************************************/

#define T_FUNCNAME      dcopy
#define T_ARGS          ,const VECDATA_DESC *y
#define T_ARGS_BV       ,INT yc
#define T_USE_Y
#define T_MOD_SCAL      VVALUE(v,xc) = VVALUE(v,yc);
#define T_MOD_VECTOR_1  VVALUE(v,cx0) = VVALUE(v,cy0);
#define T_MOD_VECTOR_2  VVALUE(v,cx1) = VVALUE(v,cy1);
#define T_MOD_VECTOR_3  VVALUE(v,cx2) = VVALUE(v,cy2);
#define T_MOD_VECTOR_N  for (i=0; i<ncomp; i++)                                 \
    VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i))=              \
      VVALUE(v,VD_CMP_OF_TYPE(y,vtype,i));

#include "vecfunc.ct"

/****************************************************************************/
/*D
   dscal - scaling x with a

   SYNOPSIS:
   INT dscal (MULTIGRID *mg, INT fl, INT tl, INT mode,
   VECDATA_DESC *x, DOUBLE a);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  x - destination vector data descriptor
   .  a - the scaling factor

   DESCRIPTION:
   This function calculates `x := a * x`.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dscalx, dscalBS
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   dscalBS - scaling x with a

   SYNOPSIS:
   INT dscalBS (const BLOCKVECTOR *bv, INT xc, DOUBLE a);

   PARAMETERS:
   .  bv - BLOCKVECTOR specifying the vector list
   .  xc - component in the VECTOR
   .  a - the scaling factor

   DESCRIPTION:
   This function calculates `x := a * x`.

   RETURN VALUE:
   INT
   .n    NUM_OK
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dscal
   D*/
/****************************************************************************/

#define T_FUNCNAME      dscal
#define T_ARGS          ,DOUBLE a
#define T_MOD_SCAL      VVALUE(v,xc) *= a;
#define T_MOD_VECTOR_1  VVALUE(v,cx0) *= a;
#define T_MOD_VECTOR_2  VVALUE(v,cx1) *= a;
#define T_MOD_VECTOR_3  VVALUE(v,cx2) *= a;
#define T_MOD_VECTOR_N  for (i=0; i<ncomp; i++)                              \
    VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i)) *= a;

#include "vecfunc.ct"

/****************************************************************************/
/*D
   dscalx - scaling x with a

   SYNOPSIS:
   INT dscalx (MULTIGRID *mg, INT fl, INT tl, INT mode,
   VECDATA_DESC *x, VEC_SCALAR *a);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  x - destination vector data descriptor
   .  a - DOUBLE value per component

   DESCRIPTION:
   This function calculates `x := a * x`.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dscal, dscalBS
   D*/
/****************************************************************************/

#define T_FUNCNAME      dscalx
#define T_ARGS          ,const VEC_SCALAR a
#define T_CONFIG        const SHORT *aoff = VD_OFFSETPTR(x);                  \
  DEFINE_VS_CMPS(a); const DOUBLE *value;
#define T_PREP_1        SET_VS_CMP_1(a,a,aoff,vtype);
#define T_MOD_VECTOR_1  VVALUE(v,cx0) *= a0;
#define T_PREP_2        SET_VS_CMP_2(a,a,aoff,vtype);
#define T_MOD_VECTOR_2  VVALUE(v,cx1) *= a1;
#define T_PREP_3        SET_VS_CMP_3(a,a,aoff,vtype);
#define T_MOD_VECTOR_3  VVALUE(v,cx2) *= a2;
#define T_PREP_N        value = a+aoff[vtype];
#define T_MOD_VECTOR_N  for (i=0; i<ncomp; i++)                              \
    VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i)) *= value[i];
#define T_NO_BV_FUNC

#include "vecfunc.ct"

/****************************************************************************/
/*D
   dadd - x plus y

   SYNOPSIS:
   INT dadd (MULTIGRID *mg, INT fl, INT tl, INT mode, VECDATA_DESC *x,
   VECDATA_DESC *y);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  x - vector data descriptor
   .  y - vector data descriptor

   DESCRIPTION:
   This function calculates `x := x + y`.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   daddBS, dsub, dminusadd
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   daddBS - x plus y

   SYNOPSIS:
   INT daddBS (const BLOCKVECTOR *bv, INT xc, INT yc);

   PARAMETERS:
   .  bv - BLOCKVECTOR specifying the vector list
   .  xc - component in the destination VECTOR
   .  yc - component in the source VECTOR

   DESCRIPTION:
   This function calculates `x := x + y`.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dadd, dsubBS, dminusaddBS
   D*/
/****************************************************************************/

#define T_FUNCNAME      dadd
#define T_ARGS          ,const VECDATA_DESC *y
#define T_ARGS_BV       ,INT yc
#define T_USE_Y
#define T_MOD_SCAL      VVALUE(v,xc) += VVALUE(v,yc);
#define T_MOD_VECTOR_1  VVALUE(v,cx0) += VVALUE(v,cy0);
#define T_MOD_VECTOR_2  VVALUE(v,cx1) += VVALUE(v,cy1);
#define T_MOD_VECTOR_3  VVALUE(v,cx2) += VVALUE(v,cy2);
#define T_MOD_VECTOR_N  for (i=0; i<ncomp; i++)                               \
    VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i))               \
      += VVALUE(v,VD_CMP_OF_TYPE(y,vtype,i));

#include "vecfunc.ct"

/****************************************************************************/
/*D
   dsub - x minus y

   SYNOPSIS:
   INT dsub (MULTIGRID *mg, INT fl, INT tl, INT mode, VECDATA_DESC *x,
   VECDATA_DESC *y);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  x - vector data descriptor
   .  y - vector data descriptor

   DESCRIPTION:
   This function calculates `x := x - y`.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dsubBS, dadd, dminusadd
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   dsubBS - x minus y

   SYNOPSIS:
   INT dsubBS (const BLOCKVECTOR *bv, INT xc, INT yc);

   PARAMETERS:
   .  bv - BLOCKVECTOR specifying the vector list
   .  xc - component in the destination VECTOR
   .  yc - component in the source VECTOR

   DESCRIPTION:
   This function calculates `x := x - y`.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dsub, daddBS, dminusaddBS
   D*/
/****************************************************************************/

#define T_FUNCNAME      dsub
#define T_ARGS          ,const VECDATA_DESC *y
#define T_ARGS_BV       ,INT yc
#define T_USE_Y
#define T_MOD_SCAL      VVALUE(v,xc) -= VVALUE(v,yc);
#define T_MOD_VECTOR_1  VVALUE(v,cx0) -= VVALUE(v,cy0);
#define T_MOD_VECTOR_2  VVALUE(v,cx1) -= VVALUE(v,cy1);
#define T_MOD_VECTOR_3  VVALUE(v,cx2) -= VVALUE(v,cy2);
#define T_MOD_VECTOR_N  for (i=0; i<ncomp; i++)                               \
    VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i))               \
      -= VVALUE(v,VD_CMP_OF_TYPE(y,vtype,i));

#include "vecfunc.ct"

/****************************************************************************/
/*D
   dminusadd - x := -x + y

   SYNOPSIS:
   INT dminusadd (MULTIGRID *mg, INT fl, INT tl, INT mode, VECDATA_DESC *x,
   VECDATA_DESC *y);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  x - vector data descriptor
   .  y - vector data descriptor

   DESCRIPTION:
   This function calculates `x := -x + y`.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dminusaddBS, dadd, dsub
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   dminusaddBS - x := -x + y

   SYNOPSIS:
   INT dminusaddBS (const BLOCKVECTOR *bv, INT xc, INT yc);

   PARAMETERS:
   .  bv - BLOCKVECTOR specifying the vector list
   .  xc - component in the destination VECTOR
   .  yc - component in the source VECTOR

   DESCRIPTION:
   This function calculates `x := -x + y`.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dminusadd, daddBS, dsubBS
   D*/
/****************************************************************************/

#define T_FUNCNAME      dminusadd
#define T_ARGS          ,const VECDATA_DESC *y
#define T_ARGS_BV       ,INT yc
#define T_USE_Y
#define T_MOD_SCAL      VVALUE(v,xc) = VVALUE(v,yc) - VVALUE(v,xc);
#define T_MOD_VECTOR_1  VVALUE(v,cx0) = VVALUE(v,cy0) - VVALUE(v,cx0);
#define T_MOD_VECTOR_2  VVALUE(v,cx1) = VVALUE(v,cy1) - VVALUE(v,cx1);
#define T_MOD_VECTOR_3  VVALUE(v,cx2) = VVALUE(v,cy2) - VVALUE(v,cx2);
#define T_MOD_VECTOR_N  for (i=0; i<ncomp; i++)                               \
    VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i))               \
      = VVALUE(v,VD_CMP_OF_TYPE(y,vtype,i)) - VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i));

#include "vecfunc.ct"

/****************************************************************************/
/*D
   daxpyx - x plus a times y

   SYNOPSIS:
   INT daxpyx (MULTIGRID *mg, INT fl, INT tl, INT mode, VECDATA_DESC *x,
   const VEC_SCALAR a, VECDATA_DESC *y);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  x - vector data descriptor
   .  a - DOUBLE value for every component of 'x'
   .  y - vector data descriptor

   DESCRIPTION:
   This function calculates `x := x + ay`.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   daxpyBS, daxpy
   D*/
/****************************************************************************/

#define T_FUNCNAME      daxpyx
#define T_ARGS          ,const VEC_SCALAR a,const VECDATA_DESC *y
#define T_USE_Y
#define T_CONFIG        const SHORT *aoff = VD_OFFSETPTR(x); const DOUBLE *value;
#define T_MOD_SCAL      VVALUE(v,xc) += a[aoff[VTYPE(v)]] * VVALUE(v,yc);
#define T_PREP_SWITCH   value = a+aoff[vtype];
#define T_MOD_VECTOR_1  VVALUE(v,cx0) += value[0] * VVALUE(v,cy0);
#define T_MOD_VECTOR_2  VVALUE(v,cx1) += value[1] * VVALUE(v,cy1);
#define T_MOD_VECTOR_3  VVALUE(v,cx2) += value[2] * VVALUE(v,cy2);
#define T_MOD_VECTOR_N  for (i=0; i<ncomp; i++)                               \
    VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i))               \
      += value[i] * VVALUE(v,VD_CMP_OF_TYPE(y,vtype,i));
#define T_NO_BV_FUNC

#include "vecfunc.ct"

/****************************************************************************/
/*D
   daxpy - x plus a times y

   SYNOPSIS:
   INT daxpy (MULTIGRID *mg, INT fl, INT tl, INT mode, VECDATA_DESC *x,
   DOUBLE a, VECDATA_DESC *y);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  x - vector data descriptor
   .  a - scaling factor
   .  y - vector data descriptor

   DESCRIPTION:
   This function calculates `x := x + ay`.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   daxpyBS, daxpyx
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   daxpyBS - x plus a times y

   SYNOPSIS:
   INT daxpyBS (const BLOCKVECTOR *bv, INT xc, DOUBLE a, INT yc);

   PARAMETERS:
   .  bv - BLOCKVECTOR specifying the vector list
   .  xc - component in the destination VECTOR
   .  a - scaling factor
   .  yc - component in the source VECTOR

   DESCRIPTION:
   This function calculates `x := x + ay`.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   daxpy
   D*/
/****************************************************************************/

#define T_FUNCNAME      daxpy
#define T_ARGS          ,DOUBLE a,const VECDATA_DESC *y
#define T_ARGS_BV       ,DOUBLE a,INT yc
#define T_USE_Y
#define T_MOD_SCAL      VVALUE(v,xc) += a * VVALUE(v,yc);
#define T_MOD_VECTOR_1  VVALUE(v,cx0) += a * VVALUE(v,cy0);
#define T_MOD_VECTOR_2  VVALUE(v,cx1) += a * VVALUE(v,cy1);
#define T_MOD_VECTOR_3  VVALUE(v,cx2) += a * VVALUE(v,cy2);
#define T_MOD_VECTOR_N  for (i=0; i<ncomp; i++)                               \
    VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i))               \
      += a * VVALUE(v,VD_CMP_OF_TYPE(y,vtype,i));

#include "vecfunc.ct"

/****************************************************************************/
/*D
   ddotx - scalar product of two vectors

   SYNOPSIS:
   INT ddotx (MULTIGRID *mg, INT fl, INT tl, INT mode,
   VECDATA_DESC *x, VECDATA_DESC *y, VEC_SCALAR a);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  x - vector data descriptor
   .  y - vector data descriptor
   .  a - DOUBLE value for every component of 'x'

   DESCRIPTION:
   This function computes the scalar product of two vectors.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   ddotBS, dnrm2, ddot, ddotw
   D*/
/****************************************************************************/

static INT UG_GlobalSumNDOUBLE_X (INT ncomp, DOUBLE *a)
{
        #ifdef ModelP
        #ifdef Debug
  INT i;
  DOUBLE a1[MAX_VEC_COMP+1];

  for (i=0; i<ncomp; i++)
    a1[i] = a[i];
  a1[ncomp] = (DOUBLE) rep_err_count;
  UG_GlobalSumNDOUBLE(ncomp+1,a1);
  if (a1[ncomp] > 0.0)
    return(1);
  for (i=0; i<ncomp; i++)
    a[i] = a1[i];
        #else
  UG_GlobalSumNDOUBLE(ncomp,a);
        #endif
        #endif
  return(0);
}

#define T_FUNCNAME      ddotx
#define T_ARGS          ,const VECDATA_DESC *y,VEC_SCALAR a
#define T_USE_Y
#define T_CONFIG        const SHORT *aoff = VD_OFFSETPTR(x); DOUBLE *value;   \
  for (i=0; i<VD_NCOMP(x); i++) a[i] = 0.0;
#define T_MOD_SCAL      a[aoff[VTYPE(v)]] += VVALUE(v,xc) * VVALUE(v,yc);
#define T_PREP_SWITCH   value = a+aoff[vtype];
#define T_MOD_VECTOR_1  value[0] += VVALUE(v,cx0) * VVALUE(v,cy0);
#define T_MOD_VECTOR_2  value[1] += VVALUE(v,cx1) * VVALUE(v,cy1);
#define T_MOD_VECTOR_3  value[2] += VVALUE(v,cx2) * VVALUE(v,cy2);
#define T_MOD_VECTOR_N  for (i=0; i<ncomp; i++)                               \
    value[i] += VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i)) * \
                VVALUE(v,VD_CMP_OF_TYPE(y,vtype,i));
#define T_POST_PAR      if (UG_GlobalSumNDOUBLE_X(VD_NCOMP(x),a))             \
    REP_ERR_RETURN(NUM_ERROR);
#define T_NO_BV_FUNC

#include "vecfunc.ct"

/****************************************************************************/
/*D
   ddotw - weighted scalar product of two vectors

   SYNOPSIS:
   INT ddotw (MULTIGRID *mg, INT fl, INT tl, INT mode,
   VECDATA_DESC *x, VECDATA_DESC *y, const VEC_SCALAR w, DOUBLE *s);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  x - vector data descriptor
   .  y - vector data descriptor
   .  w - weight factors
   .  a - DOUBLE value for every component of 'x'

   DESCRIPTION:
   This function computes the weighted scalar product of two vectors.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   ddotBS, dnrm2, ddot, ddotx
   D*/
/****************************************************************************/

#define T_FUNCNAME      ddotw
#define T_ARGS          ,const VECDATA_DESC *y,const VEC_SCALAR w,DOUBLE *s
#define T_USE_Y
#define T_CONFIG        const SHORT *aoff = VD_OFFSETPTR(x); DOUBLE *value;   \
  VEC_SCALAR a;                                                                             \
  for (i=0; i<VD_NCOMP(x); i++) a[i] = 0.0;
#define T_MOD_SCAL      a[aoff[VTYPE(v)]] += VVALUE(v,xc) * VVALUE(v,yc);
#define T_PREP_SWITCH   value = a+aoff[vtype];
#define T_MOD_VECTOR_1  value[0] += VVALUE(v,cx0) * VVALUE(v,cy0);
#define T_MOD_VECTOR_2  value[1] += VVALUE(v,cx1) * VVALUE(v,cy1);
#define T_MOD_VECTOR_3  value[2] += VVALUE(v,cx2) * VVALUE(v,cy2);
#define T_MOD_VECTOR_N  for (i=0; i<ncomp; i++)                               \
    value[i] += VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i)) * \
                VVALUE(v,VD_CMP_OF_TYPE(y,vtype,i));
#define T_POST_PAR      if (UG_GlobalSumNDOUBLE_X(VD_NCOMP(x),a))             \
    REP_ERR_RETURN(NUM_ERROR);
#define T_POST          *s = 0.0; for (i=0; i<VD_NCOMP(x); i++) *s += w[i]*a[i];
#define T_NO_BV_FUNC

#include "vecfunc.ct"

/****************************************************************************/
/*D
   ddot - scalar product of two vectors

   SYNOPSIS:
   INT ddot (MULTIGRID *mg, INT fl, INT tl, INT mode,
   const VECDATA_DESC *x, const VECDATA_DESC *y, DOUBLE *a);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  x - vector data descriptor
   .  y - vector data descriptor
   .  a - pointer to result

   DESCRIPTION:
   This function computes the scalar product of two vectors.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   ddotBS, dnrm2, ddotx, ddotw
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   ddotBS - scalar product of two vectors

   SYNOPSIS:
   INT ddotBS (const BLOCKVECTOR *bv, INT xc, INT yc, DOUBLE *a);

   PARAMETERS:
   .  bv - BLOCKVECTOR specifying the vector list
   .  xc - component in the destination VECTOR
   .  yc - component in the source VECTOR
   .  a - pointer to result

   DESCRIPTION:
   This function computes the scalar product of two vectors.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   ddot, dnrm2BS
   D*/
/****************************************************************************/

#define T_FUNCNAME      ddot
#define T_ARGS          ,const VECDATA_DESC *y,DOUBLE *a
#define T_ARGS_BV       ,INT yc,DOUBLE *a
#define T_USE_Y
#define T_CONFIG        register DOUBLE sum = 0.0;
#define T_MOD_SCAL      sum += VVALUE(v,xc) * VVALUE(v,yc);
#define T_MOD_VECTOR_1  sum += VVALUE(v,cx0) * VVALUE(v,cy0);
#define T_MOD_VECTOR_2  sum += VVALUE(v,cx1) * VVALUE(v,cy1);
#define T_MOD_VECTOR_3  sum += VVALUE(v,cx2) * VVALUE(v,cy2);
#define T_MOD_VECTOR_N  for (i=0; i<ncomp; i++)                              \
    sum += VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i)) *     \
           VVALUE(v,VD_CMP_OF_TYPE(y,vtype,i));
#define T_POST_PAR      *a=sum;if (UG_GlobalSumNDOUBLE_X(1,a))               \
    REP_ERR_RETURN(NUM_ERROR);sum=*a;
#define T_POST                  *a=sum;

#include "vecfunc.ct"

/****************************************************************************/
/*D
   dnrm2x - euclidian norm of a vector

   SYNOPSIS:
   INT dnrm2x (MULTIGRID *mg, INT fl, INT tl, INT mode, const VECDATA_DESC *x,
   VEC_SCALAR a);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  x - vector data descriptor
   .  a - DOUBLE value for every component of 'x'

   DESCRIPTION:
   This function computes the euclidian norm of a vector and stores it to a
   VEC_SCALAR.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dnrm2
   D*/
/****************************************************************************/

#define T_FUNCNAME      dnrm2x
#define T_ARGS          ,VEC_SCALAR a
#define T_CONFIG        const SHORT *aoff = VD_OFFSETPTR(x); DOUBLE *value;   \
  register DOUBLE s;                                                                        \
  for (i=0; i<VD_NCOMP(x); i++) a[i] = 0.0;
#define T_MOD_SCAL      s = VVALUE(v,xc); a[aoff[VTYPE(v)]] += s*s;
#define T_PREP_SWITCH   value = a+aoff[vtype];
#define T_MOD_VECTOR_1  s = VVALUE(v,cx0); value[0] += s*s;
#define T_MOD_VECTOR_2  s = VVALUE(v,cx1); value[1] += s*s;
#define T_MOD_VECTOR_3  s = VVALUE(v,cx2); value[2] += s*s;
#define T_MOD_VECTOR_N  for (i=0; i<ncomp; i++) {                             \
    s = VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i));           \
    value[i] += s*s; }
#define T_POST_PAR      if (UG_GlobalSumNDOUBLE_X(VD_NCOMP(x),a))             \
    REP_ERR_RETURN(NUM_ERROR);
#define T_POST          for (i=0; i<VD_NCOMP(x); i++) a[i] = SQRT(a[i]);
#define T_NO_BV_FUNC

#include "vecfunc.ct"

/****************************************************************************/
/*D
   dnrm2 - euclidian norm of a vector

   SYNOPSIS:
   INT dnrm2 (MULTIGRID *mg, INT fl, INT tl, INT mode, const VECDATA_DESC *x,
   DOUBLE *a);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  x - vector data descriptor
   .  a - pointer to result

   DESCRIPTION:
   This function computes the euclidian norm of a vector and stores it to
   a DOUBLE.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dnrm2BS, ddot
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   dnrm2BS - euclidian norm of a vector

   SYNOPSIS:
   INT dnrm2BS (const BLOCKVECTOR *bv, INT xc, DOUBLE *a);

   PARAMETERS:
   .  bv - BLOCKVECTOR specifying the vector list
   .  xc - component in the destination VECTOR
   .  a - pointer to result

   DESCRIPTION:
   This function computes the euclidian norm of a vector and stores it to
   a DOUBLE.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dnrm2, ddotBS
   D*/
/****************************************************************************/

#define T_FUNCNAME      dnrm2
#define T_ARGS          ,DOUBLE *a
#define T_CONFIG        register DOUBLE s, sum = 0.0;
#define T_MOD_SCAL      s = VVALUE(v,xc); sum += s*s;
#define T_MOD_VECTOR_1  s = VVALUE(v,cx0); sum += s*s;
#define T_MOD_VECTOR_2  s = VVALUE(v,cx1); sum += s*s;
#define T_MOD_VECTOR_3  s = VVALUE(v,cx2); sum += s*s;
#define T_MOD_VECTOR_N  for (i=0; i<ncomp; i++) {                             \
    s = VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i));           \
    sum += s*s; }
#define T_POST_PAR      *a=sum;if (UG_GlobalSumNDOUBLE_X(1,a))                \
    REP_ERR_RETURN(NUM_ERROR);sum=*a;
#define T_POST                  *a = SQRT(sum);

#include "vecfunc.ct"

/****************************************************************************/
/*																			*/
/*		blas level 2 routines												*/
/*																			*/
/****************************************************************************/

#ifdef __MWCW__
#pragma mark blas_level_1
#endif

/****************************************************************************/
/*D
   dmatset - initialize a matrix with a given value

   SYNOPSIS:
   INT dmatset (MULTIGRID *mg, INT fl, INT tl, INT mode, const MATDATA_DESC *M,
   DOUBLE a);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  M - matrix data descriptor
   .  a - DOUBLE value

   DESCRIPTION:
   This function sets all matrix entries to `a`.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dmatsetBS
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   dmatsetBS - initialize a matrix with a given value

   SYNOPSIS:
   INT dmatsetBS (const BLOCKVECTOR *bv_row, const BV_DESC *bvd_col,
   const BV_DESC_FORMAT *bvdf, INT mc, DOUBLE a);

   PARAMETERS:
   .  bv_row - BLOCKVECTOR specifying the row vector list
   .  bvd_col - BLOCKVECTOR_DESCRIPTOR specifying the column vector list
   .  bvdf - format to interpret bvd_col
   .  mc - component in the MATRIX
   .  a - DOUBLE value

   DESCRIPTION:
   This function sets all matrix entries to `a`.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dmatset
   D*/
/****************************************************************************/

#define T_FUNCNAME     dmatset
#define T_ARGS         ,const MATDATA_DESC *M,DOUBLE a
#define T_ARGS_BV      ,INT mc,DOUBLE a
#define T_MOD_SCAL     MVALUE(mat,mc)=a;
#define T_PREP_SWITCH  INT mcomp;
#define T_MOD_11       MVALUE(mat,m00)=a;
#define T_MOD_12       MVALUE(mat,m00)=a; MVALUE(mat,m01)=a;
#define T_MOD_13       MVALUE(mat,m00)=a; MVALUE(mat,m01)=a; MVALUE(mat,m02)=a;
#define T_MOD_21       MVALUE(mat,m00)=a;                                      \
  MVALUE(mat,m10)=a;
#define T_MOD_22       MVALUE(mat,m00)=a; MVALUE(mat,m01)=a;                   \
  MVALUE(mat,m10)=a; MVALUE(mat,m11)=a;
#define T_MOD_23       MVALUE(mat,m00)=a; MVALUE(mat,m01)=a; MVALUE(mat,m02)=a;\
  MVALUE(mat,m10)=a; MVALUE(mat,m11)=a; MVALUE(mat,m12)=a;
#define T_MOD_31       MVALUE(mat,m00)=a;                                      \
  MVALUE(mat,m10)=a;                                      \
  MVALUE(mat,m20)=a;
#define T_MOD_32       MVALUE(mat,m00)=a; MVALUE(mat,m01)=a;                   \
  MVALUE(mat,m10)=a; MVALUE(mat,m11)=a;                   \
  MVALUE(mat,m20)=a; MVALUE(mat,m21)=a;
#define T_MOD_33       MVALUE(mat,m00)=a; MVALUE(mat,m01)=a; MVALUE(mat,m02)=a;\
  MVALUE(mat,m10)=a; MVALUE(mat,m11)=a; MVALUE(mat,m12)=a;\
  MVALUE(mat,m20)=a; MVALUE(mat,m21)=a; MVALUE(mat,m22)=a;
#define T_PREP_N       mcomp = nr * nc;
#define T_MOD_N        for (i=0; i<mcomp; i++)                                 \
    MVALUE(mat,MD_MCMP_OF_RT_CT(M,rtype,ctype,i)) = a;

#include "matfunc.ct"

/****************************************************************************/
/*D
   dmatcopy - copy a matrix

   SYNOPSIS:
   INT dmatcopy (MULTIGRID *mg, INT fl, INT tl, INT mode, const MATDATA_DESC *M,
   const MATDATA_DESC *N);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  M - destination matrix data descriptor
   .  N - source matrix data descriptor

   DESCRIPTION:
   This function set all 'M := N'.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dmatcopyBS
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   dmatcopyBS - copy a matrix

   SYNOPSIS:
   INT dmatcopyBS (const BLOCKVECTOR *bv_row, const BV_DESC *bvd_col,
   const BV_DESC_FORMAT *bvdf, INT mc, INT nc);

   PARAMETERS:
   .  bv_row - BLOCKVECTOR specifying the row vector list
   .  bvd_col - BLOCKVECTOR_DESCRIPTOR specifying the column vector list
   .  bvdf - format to interpret bvd_col
   .  mc - component in the destination MATRIX
   .  nc - component in the source MATRIX

   DESCRIPTION:
   This function set all 'M := N'.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dmatcopy
   D*/
/****************************************************************************/

#define T_FUNCNAME     dmatcopy
#define T_ARGS         ,const MATDATA_DESC *M,const MATDATA_DESC *N
#define T_ARGS_BV      ,INT mc,INT nc
#define T_PREP_SCAL    register SHORT nc = MD_SCALCMP(N);
#define T_MOD_SCAL     MVALUE(mat,mc)=MVALUE(mat,nc);
#define T_PREP_SWITCH  INT mcomp;DEFINE_MD_CMPS(n);
#define T_PREP_11      SET_MD_CMP_11(n,N,rtype,ctype);
#define T_MOD_11       MVALUE(mat,m00)=MVALUE(mat,n00);
#define T_PREP_12      SET_MD_CMP_12(n,N,rtype,ctype);
#define T_MOD_12       MVALUE(mat,m00)=MVALUE(mat,n00);    \
  MVALUE(mat,m01)=MVALUE(mat,n01);
#define T_PREP_13      SET_MD_CMP_13(n,N,rtype,ctype);
#define T_MOD_13       MVALUE(mat,m00)=MVALUE(mat,n00);    \
  MVALUE(mat,m01)=MVALUE(mat,n01);    \
  MVALUE(mat,m02)=MVALUE(mat,n02);
#define T_PREP_21      SET_MD_CMP_21(n,N,rtype,ctype);
#define T_MOD_21       MVALUE(mat,m00)=MVALUE(mat,n00);    \
  MVALUE(mat,m10)=MVALUE(mat,n10);
#define T_PREP_22      SET_MD_CMP_22(n,N,rtype,ctype);
#define T_MOD_22       MVALUE(mat,m00)=MVALUE(mat,n00);    \
  MVALUE(mat,m01)=MVALUE(mat,n01);    \
  MVALUE(mat,m10)=MVALUE(mat,n10);    \
  MVALUE(mat,m11)=MVALUE(mat,n11);
#define T_PREP_23      SET_MD_CMP_23(n,N,rtype,ctype);
#define T_MOD_23       MVALUE(mat,m00)=MVALUE(mat,n00);    \
  MVALUE(mat,m01)=MVALUE(mat,n01);    \
  MVALUE(mat,m02)=MVALUE(mat,n02);    \
  MVALUE(mat,m10)=MVALUE(mat,n10);    \
  MVALUE(mat,m11)=MVALUE(mat,n11);    \
  MVALUE(mat,m12)=MVALUE(mat,n12);
#define T_PREP_31      SET_MD_CMP_31(n,N,rtype,ctype);
#define T_MOD_31       MVALUE(mat,m00)=MVALUE(mat,n00);    \
  MVALUE(mat,m10)=MVALUE(mat,n10);    \
  MVALUE(mat,m20)=MVALUE(mat,n20);
#define T_PREP_32      SET_MD_CMP_32(n,N,rtype,ctype);
#define T_MOD_32       MVALUE(mat,m00)=MVALUE(mat,n00);    \
  MVALUE(mat,m01)=MVALUE(mat,n01);    \
  MVALUE(mat,m10)=MVALUE(mat,n10);    \
  MVALUE(mat,m11)=MVALUE(mat,n11);    \
  MVALUE(mat,m20)=MVALUE(mat,n20);    \
  MVALUE(mat,m21)=MVALUE(mat,n21);
#define T_PREP_33      SET_MD_CMP_33(n,N,rtype,ctype);
#define T_MOD_33       MVALUE(mat,m00)=MVALUE(mat,n00);    \
  MVALUE(mat,m01)=MVALUE(mat,n01);    \
  MVALUE(mat,m02)=MVALUE(mat,n02);    \
  MVALUE(mat,m10)=MVALUE(mat,n10);    \
  MVALUE(mat,m11)=MVALUE(mat,n11);    \
  MVALUE(mat,m12)=MVALUE(mat,n12);    \
  MVALUE(mat,m20)=MVALUE(mat,n20);    \
  MVALUE(mat,m21)=MVALUE(mat,n21);    \
  MVALUE(mat,m22)=MVALUE(mat,n22);
#define T_PREP_N       mcomp = nr * nc;
#define T_MOD_N        for (i=0; i<mcomp; i++)                                 \
    MVALUE(mat,MD_MCMP_OF_RT_CT(M,rtype,ctype,i)) =     \
      MVALUE(mat,MD_MCMP_OF_RT_CT(N,rtype,ctype,i));

#include "matfunc.ct"

/****************************************************************************/
/*D
   dmatadd - add a matrix

   SYNOPSIS:
   INT dmatadd (MULTIGRID *mg, INT fl, INT tl, INT mode, const MATDATA_DESC *M,
   const MATDATA_DESC *N);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  M - destination matrix data descriptor
   .  N - source matrix data descriptor

   DESCRIPTION:
   This function sets 'M := M + N'.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dmataddBS, dmatmul
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   dmataddBS - add a matrix

   SYNOPSIS:
   INT dmataddBS (const BLOCKVECTOR *bv_row, const BV_DESC *bvd_col,
   const BV_DESC_FORMAT *bvdf, INT mc, INT nc);

   PARAMETERS:
   .  bv_row - BLOCKVECTOR specifying the row vector list
   .  bvd_col - BLOCKVECTOR_DESCRIPTOR specifying the column vector list
   .  bvdf - format to interpret bvd_col
   .  mc - component in the destination MATRIX
   .  nc - component in the source MATRIX


   DESCRIPTION:
   This function sets 'M := M + N'.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dmatadd, dmatmulBS
   D*/
/****************************************************************************/

#define T_FUNCNAME     dmatadd
#define T_ARGS         ,const MATDATA_DESC *M,const MATDATA_DESC *N
#define T_ARGS_BV      ,INT mc,INT nc
#define T_PREP_SCAL    register SHORT nc = MD_SCALCMP(N);
#define T_MOD_SCAL     MVALUE(mat,mc)+=MVALUE(mat,nc);
#define T_PREP_SWITCH  INT mcomp;DEFINE_MD_CMPS(n);
#define T_PREP_11      SET_MD_CMP_11(n,N,rtype,ctype);
#define T_MOD_11       MVALUE(mat,m00)+=MVALUE(mat,n00);
#define T_PREP_12      SET_MD_CMP_12(n,N,rtype,ctype);
#define T_MOD_12       MVALUE(mat,m00)+=MVALUE(mat,n00);    \
  MVALUE(mat,m01)+=MVALUE(mat,n01);
#define T_PREP_13      SET_MD_CMP_13(n,N,rtype,ctype);
#define T_MOD_13       MVALUE(mat,m00)+=MVALUE(mat,n00);    \
  MVALUE(mat,m01)+=MVALUE(mat,n01);    \
  MVALUE(mat,m02)+=MVALUE(mat,n02);
#define T_PREP_21      SET_MD_CMP_21(n,N,rtype,ctype);
#define T_MOD_21       MVALUE(mat,m00)+=MVALUE(mat,n00);    \
  MVALUE(mat,m10)+=MVALUE(mat,n10);
#define T_PREP_22      SET_MD_CMP_22(n,N,rtype,ctype);
#define T_MOD_22       MVALUE(mat,m00)+=MVALUE(mat,n00);    \
  MVALUE(mat,m01)+=MVALUE(mat,n01);    \
  MVALUE(mat,m10)+=MVALUE(mat,n10);    \
  MVALUE(mat,m11)+=MVALUE(mat,n11);
#define T_PREP_23      SET_MD_CMP_23(n,N,rtype,ctype);
#define T_MOD_23       MVALUE(mat,m00)+=MVALUE(mat,n00);    \
  MVALUE(mat,m01)+=MVALUE(mat,n01);    \
  MVALUE(mat,m02)+=MVALUE(mat,n02);    \
  MVALUE(mat,m10)+=MVALUE(mat,n10);    \
  MVALUE(mat,m11)+=MVALUE(mat,n11);    \
  MVALUE(mat,m12)+=MVALUE(mat,n12);
#define T_PREP_31      SET_MD_CMP_31(n,N,rtype,ctype);
#define T_MOD_31       MVALUE(mat,m00)+=MVALUE(mat,n00);    \
  MVALUE(mat,m10)+=MVALUE(mat,n10);    \
  MVALUE(mat,m20)+=MVALUE(mat,n20);
#define T_PREP_32      SET_MD_CMP_32(n,N,rtype,ctype);
#define T_MOD_32       MVALUE(mat,m00)+=MVALUE(mat,n00);    \
  MVALUE(mat,m01)+=MVALUE(mat,n01);    \
  MVALUE(mat,m10)+=MVALUE(mat,n10);    \
  MVALUE(mat,m11)+=MVALUE(mat,n11);    \
  MVALUE(mat,m20)+=MVALUE(mat,n20);    \
  MVALUE(mat,m21)+=MVALUE(mat,n21);
#define T_PREP_33      SET_MD_CMP_33(n,N,rtype,ctype);
#define T_MOD_33       MVALUE(mat,m00)+=MVALUE(mat,n00);    \
  MVALUE(mat,m01)+=MVALUE(mat,n01);    \
  MVALUE(mat,m02)+=MVALUE(mat,n02);    \
  MVALUE(mat,m10)+=MVALUE(mat,n10);    \
  MVALUE(mat,m11)+=MVALUE(mat,n11);    \
  MVALUE(mat,m12)+=MVALUE(mat,n12);    \
  MVALUE(mat,m20)+=MVALUE(mat,n20);    \
  MVALUE(mat,m21)+=MVALUE(mat,n21);    \
  MVALUE(mat,m22)+=MVALUE(mat,n22);
#define T_PREP_N       mcomp = nr * nc;
#define T_MOD_N        for (i=0; i<mcomp; i++)                                 \
    MVALUE(mat,MD_MCMP_OF_RT_CT(M,rtype,ctype,i)) +=    \
      MVALUE(mat,MD_MCMP_OF_RT_CT(N,rtype,ctype,i));

#include "matfunc.ct"

/****************************************************************************/
/*D
   dmatmul - matrix vector product

   SYNOPSIS:
   INT dmatmul (MULTIGRID *mg, INT fl, INT tl, INT mode, const VECDATA_DESC *x,
   const MATDATA_DESC *M, const VECDATA_DESC *y);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  mode - ALL_VECTORS or ON_SURFACE
   .  x - vector descriptor
   .  M - matrix data descriptor
   .  y - vector descriptor

   DESCRIPTION:
   This function computes `x = M * y`.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dmatmulBS, dmatmul_add, dmatmul_minus
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   dmatmulBS - matrix vector product

   SYNOPSIS:
   INT dmatmulBS (const BLOCKVECTOR *bv_row, const BV_DESC *bvd_col,
   const BV_DESC_FORMAT *bvdf, INT xc, INT mc, INT yc);

   PARAMETERS:
   .  bv_row - BLOCKVECTOR specifying the row vector list
   .  bvd_col - BLOCKVECTOR_DESCRIPTOR specifying the column vector list
   .  bvdf - format to interpret bvd_col
   .  xc - component in the result VECTOR
   .  mc - component in the MATRIX
   .  yc - component in the source VECTOR

   DESCRIPTION:
   This function computes `x = M * y`.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dmatmul, dmatmul_addBS, dmatmul_minusBS, d2matmulBS, d3matmulBS
   D*/
/****************************************************************************/

INT dmatmul1        (MULTIGRID *mg, INT fl, INT tl, INT mode, const VECDATA_DESC *x,
                     const MATDATA_DESC *M, const VECDATA_DESC *y)
{
  dset(mg,fl,tl,mode,x,0.0);
  return(dmatmul_add(mg,fl,tl,mode,x,M,y));
}

#define T_FUNCNAME     dmatmul
#define T_ARGS         ,const VECDATA_DESC *x,const MATDATA_DESC *M,const VECDATA_DESC *y
#define T_ARGS_BV      ,INT xc,INT mc,INT yc
#define T_USE_X
#define T_USE_Y
#define T_USE_MATMUL
#define T_CONFIG           DOUBLE s[MAX_SINGLE_VEC_COMP],sum;DEFINE_VS_CMPS(s);
#define T_CONFIG_BV        register DOUBLE sum;
#define T_LOOP_SCAL    sum = 0.0;
#define T_MOD_SCAL     sum += MVALUE(mat,mc) * VVALUE(w,yc);
#define T_POST_SCAL    VVALUE(v,xc) = sum;
#define T_CLEAR_X
#define T_LOOP_11          s0 = 0.0;
#define T_POST_11          VVALUE(v,cx0) += s0;
#define T_LOOP_12          s0 = 0.0;
#define T_POST_12          VVALUE(v,cx0) += s0;
#define T_LOOP_13          s0 = 0.0;
#define T_POST_13          VVALUE(v,cx0) += s0;
#define T_LOOP_21          s0 = s1 = 0.0;
#define T_POST_21      VVALUE(v,cx0) += s0; VVALUE(v,cx1) += s1;
#define T_LOOP_22          s0 = s1 = 0.0;
#define T_POST_22      VVALUE(v,cx0) += s0; VVALUE(v,cx1) += s1;
#define T_LOOP_23          s0 = s1 = 0.0;
#define T_POST_23      VVALUE(v,cx0) += s0; VVALUE(v,cx1) += s1;
#define T_LOOP_31          s0 = s1 = s2 = 0.0;
#define T_POST_31          VVALUE(v,cx0) +=s0;VVALUE(v,cx1) +=s1;VVALUE(v,cx2) +=s2;
#define T_LOOP_32          s0 = s1 = s2 = 0.0;
#define T_POST_32          VVALUE(v,cx0) +=s0;VVALUE(v,cx1) +=s1;VVALUE(v,cx2) +=s2;
#define T_LOOP_33          s0 = s1 = s2 = 0.0;
#define T_POST_33          VVALUE(v,cx0) +=s0;VVALUE(v,cx1) +=s1;VVALUE(v,cx2) +=s2;
#define T_LOOP_N           for (i=0; i<nr; i++) s[i] = 0.0;
#define T_MOD_N        for (i=0; i<nr; i++)                                   \
    for (j=0; j<nc; j++)                               \
      s[i] += MVALUE(mat,MD_MCMP_OF_RT_CT(M,rtype,ctype,i*nc+j)) *  \
              VVALUE(w,VD_CMP_OF_TYPE(y,ctype,j));
#define T_POST_N           for (i=0; i<nr; i++)                                   \
    VVALUE(v,VD_CMP_OF_TYPE(x,rtype,i)) += s[i];

#include "matfunc.ct"

/****************************************************************************/
/*D
   dmatmul_add - add matrix vector product

   SYNOPSIS:
   INT dmatmul_add (MULTIGRID *mg, INT fl, INT tl, INT mode, const VECDATA_DESC *x,
   const MATDATA_DESC *M, const VECDATA_DESC *y);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  x - vector descriptor
   .  M - matrix data descriptor
   .  y - vector descriptor

   DESCRIPTION:
   This function computes `x = x + M * y`.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dmatmul_addBS, dmatmul_minus, dmatmul
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   dmatmul_addBS - add matrix vector product

   SYNOPSIS:
   INT dmatmul_addBS (const BLOCKVECTOR *bv_row, const BV_DESC *bvd_col,
   const BV_DESC_FORMAT *bvdf, INT xc, INT mc, INT yc);

   PARAMETERS:
   .  bv_row - BLOCKVECTOR specifying the row vector list
   .  bvd_col - BLOCKVECTOR_DESCRIPTOR specifying the column vector list
   .  bvdf - format to interpret bvd_col
   .  xc - component in the result VECTOR
   .  mc - component in the MATRIX
   .  yc - component in the source VECTOR

   DESCRIPTION:
   This function computes `x = x + M * y`.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dmatmul_add, dmatmul_minusBS, dmatmulBS, d2matmulBS, d3matmulBS
   D*/
/****************************************************************************/

#define T_FUNCNAME     dmatmul_add
#define T_ARGS         ,const VECDATA_DESC *x,const MATDATA_DESC *M,const VECDATA_DESC *y
#define T_ARGS_BV      ,INT xc,INT mc,INT yc
#define T_USE_X
#define T_USE_Y
#define T_USE_MATMUL
#define T_CONFIG           DOUBLE s[MAX_SINGLE_VEC_COMP],sum;DEFINE_VS_CMPS(s);
#define T_CONFIG_BV    register DOUBLE sum;
#define T_LOOP_SCAL    sum = 0.0;
#define T_MOD_SCAL     sum += MVALUE(mat,mc) * VVALUE(w,yc);
#define T_POST_SCAL    VVALUE(v,xc) += sum;
#define T_LOOP_11          s0 = 0.0;
#define T_POST_11          VVALUE(v,cx0) += s0;
#define T_LOOP_12          s0 = 0.0;
#define T_POST_12          VVALUE(v,cx0) += s0;
#define T_LOOP_13          s0 = 0.0;
#define T_POST_13          VVALUE(v,cx0) += s0;
#define T_LOOP_21          s0 = s1 = 0.0;
#define T_POST_21      VVALUE(v,cx0) += s0; VVALUE(v,cx1) += s1;
#define T_LOOP_22          s0 = s1 = 0.0;
#define T_POST_22      VVALUE(v,cx0) += s0; VVALUE(v,cx1) += s1;
#define T_LOOP_23          s0 = s1 = 0.0;
#define T_POST_23      VVALUE(v,cx0) += s0; VVALUE(v,cx1) += s1;
#define T_LOOP_31          s0 = s1 = s2 = 0.0;
#define T_POST_31          VVALUE(v,cx0) +=s0;VVALUE(v,cx1) +=s1;VVALUE(v,cx2) +=s2;
#define T_LOOP_32          s0 = s1 = s2 = 0.0;
#define T_POST_32          VVALUE(v,cx0) +=s0;VVALUE(v,cx1) +=s1;VVALUE(v,cx2) +=s2;
#define T_LOOP_33          s0 = s1 = s2 = 0.0;
#define T_POST_33          VVALUE(v,cx0) +=s0;VVALUE(v,cx1) +=s1;VVALUE(v,cx2) +=s2;
#define T_LOOP_N           for (i=0; i<nr; i++) s[i] = 0.0;
#define T_MOD_N        for (i=0; i<nr; i++)                                   \
    for (j=0; j<nc; j++)                               \
      s[i] += MVALUE(mat,MD_MCMP_OF_RT_CT(M,rtype,ctype,i*nc+j)) *  \
              VVALUE(w,VD_CMP_OF_TYPE(y,ctype,j));
#define T_POST_N           for (i=0; i<nr; i++)                                   \
    VVALUE(v,VD_CMP_OF_TYPE(x,rtype,i)) += s[i];

#include "matfunc.ct"

/****************************************************************************/
/*D
   dmatmul_minus - subtract matrix vector product

   SYNOPSIS:
   INT dmatmul_minus (MULTIGRID *mg, INT fl, INT tl, INT mode,
   const VECDATA_DESC *x, const MATDATA_DESC *M, const VECDATA_DESC *y);

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  mode - ALL_VECTORS or ON_SURFACE
   .  x - vector descriptor
   .  M - matrix data descriptor
   .  y - vector descriptor

   DESCRIPTION:
   This function computes `x = x - M * y`.

   It runs from level fl to tl.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dmatmul_minusBS, dmatmul_add, dmatmul
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   dmatmul_minusBS - subtract matrix vector product

   SYNOPSIS:
   INT dmatmul_minusBS (const BLOCKVECTOR *bv_row, const BV_DESC *bvd_col,
   const BV_DESC_FORMAT *bvdf, INT xc, INT mc, INT yc);

   PARAMETERS:
   .  bv_row - BLOCKVECTOR specifying the row vector list
   .  bvd_col - BLOCKVECTOR_DESCRIPTOR specifying the column vector list
   .  bvdf - format to interpret bvd_col
   .  xc - component in the result VECTOR
   .  mc - component in the MATRIX
   .  yc - component in the source VECTOR

   DESCRIPTION:
   This function computes `x = x - M * y`.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured

   SEE ALSO:
   dmatmul_minus, dmatmul_addBS, dmatmulBS, d2matmulBS, d3matmulBS
   D*/
/****************************************************************************/

#define T_FUNCNAME     dmatmul_minus
#define T_ARGS         ,const VECDATA_DESC *x,const MATDATA_DESC *M,const VECDATA_DESC *y
#define T_ARGS_BV      ,INT xc,INT mc,INT yc
#define T_USE_X
#define T_USE_Y
#define T_USE_MATMUL
#define T_CONFIG           DOUBLE s[MAX_SINGLE_VEC_COMP],sum;DEFINE_VS_CMPS(s);
#define T_CONFIG_BV    register DOUBLE sum;
#define T_LOOP_SCAL    sum = 0.0;
#define T_MOD_SCAL     sum += MVALUE(mat,mc) * VVALUE(w,yc);
#define T_POST_SCAL    VVALUE(v,xc) -= sum;
#define T_LOOP_11          s0 = 0.0;
#define T_POST_11          VVALUE(v,cx0) -= s0;
#define T_LOOP_12          s0 = 0.0;
#define T_POST_12          VVALUE(v,cx0) -= s0;
#define T_LOOP_13          s0 = 0.0;
#define T_POST_13          VVALUE(v,cx0) -= s0;
#define T_LOOP_21          s0 = s1 = 0.0;
#define T_POST_21      VVALUE(v,cx0) -= s0; VVALUE(v,cx1) -= s1;
#define T_LOOP_22          s0 = s1 = 0.0;
#define T_POST_22      VVALUE(v,cx0) -= s0; VVALUE(v,cx1) -= s1;
#define T_LOOP_23          s0 = s1 = 0.0;
#define T_POST_23      VVALUE(v,cx0) -= s0; VVALUE(v,cx1) -= s1;
#define T_LOOP_31          s0 = s1 = s2 = 0.0;
#define T_POST_31          VVALUE(v,cx0) -=s0;VVALUE(v,cx1) -=s1;VVALUE(v,cx2) -=s2;
#define T_LOOP_32          s0 = s1 = s2 = 0.0;
#define T_POST_32          VVALUE(v,cx0) -=s0;VVALUE(v,cx1) -=s1;VVALUE(v,cx2) -=s2;
#define T_LOOP_33          s0 = s1 = s2 = 0.0;
#define T_POST_33          VVALUE(v,cx0) -=s0;VVALUE(v,cx1) -=s1;VVALUE(v,cx2) -=s2;
#define T_LOOP_N           for (i=0; i<nr; i++) s[i] = 0.0;
#define T_MOD_N        for (i=0; i<nr; i++)                                   \
    for (j=0; j<nc; j++)                               \
      s[i] += MVALUE(mat,MD_MCMP_OF_RT_CT(M,rtype,ctype,i*nc+j)) *  \
              VVALUE(w,VD_CMP_OF_TYPE(y,ctype,j));
#define T_POST_N           for (i=0; i<nr; i++)                                   \
    VVALUE(v,VD_CMP_OF_TYPE(x,rtype,i)) -= s[i];

#include "matfunc.ct"

/****************************************************************************/
/*D
   l_dsetrandom - set all components of a vector randomly

   SYNOPSIS:
   INT l_dsetrandom (GRID *g, const VECDATA_DESC *x, INT xclass, DOUBLE a);

   PARAMETERS:
   .  g - pointer to grid
   .  x - vector data descriptor
   .  xclass - vector class
   .  a - the maximal random value

   DESCRIPTION:
   This function sets all components of a vector on one grid level
   to random value.

   RETURN VALUE:
   INT
   .n    NUM_OK
   D*/
/****************************************************************************/

INT l_dsetrandom (GRID *g, const VECDATA_DESC *x, INT xclass, DOUBLE a)
{
  VECTOR *first_v;
  register VECTOR *v;
  register SHORT i;
  register SHORT ncomp;
  register INT vtype;
  DOUBLE scale;
  DEFINE_VD_CMPS(cx);

  if (a<=0.0) REP_ERR_RETURN (NUM_ERROR);
  scale = a/(DOUBLE)RAND_MAX;

  first_v = FIRSTVECTOR(g);

  for (vtype=0; vtype<NVECTYPES; vtype++)
    if (VD_ISDEF_IN_TYPE(x,vtype))
      switch (VD_NCMPS_IN_TYPE(x,vtype))
      {
      case 1 :
        SET_VD_CMP_1(cx,x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        VVALUE(v,cx0) = scale*(DOUBLE)rand();
        break;

      case 2 :
        SET_VD_CMP_2(cx,x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        {
          VVALUE(v,cx0) = scale*(DOUBLE)rand(); VVALUE(v,cx1) = scale*(DOUBLE)rand();
        }
        break;

      case 3 :
        SET_VD_CMP_3(cx,x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        {
          VVALUE(v,cx0) = scale*(DOUBLE)rand(); VVALUE(v,cx1) = scale*(DOUBLE)rand(); VVALUE(v,cx2) = scale*(DOUBLE)rand();
        }
        break;

      default :
        ncomp = VD_NCMPS_IN_TYPE(x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        for (i=0; i<ncomp; i++)
          VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i)) = scale*(DOUBLE)rand();
      }

        #ifdef ModelP
  if (l_vector_consistent(g,x))
    return (NUM_ERROR);
        #endif

  return (NUM_OK);
}

INT l_dsetrandom2 (GRID *g, const VECDATA_DESC *x, INT xclass, DOUBLE from, DOUBLE to, INT skip)
{
  VECTOR *first_v;
  register VECTOR *v;
  register SHORT i;
  register SHORT ncomp;
  register INT vtype;
  DOUBLE scale;
  DEFINE_VD_CMPS(cx);
  INT vskip;

  if (from>=to) REP_ERR_RETURN (NUM_ERROR);
  scale = (to -from)/(DOUBLE)RAND_MAX;

  first_v = FIRSTVECTOR(g);

  for (vtype=0; vtype<NVECTYPES; vtype++)
    if (VD_ISDEF_IN_TYPE(x,vtype))
      switch (VD_NCMPS_IN_TYPE(x,vtype))
      {
      case 1 :
        SET_VD_CMP_1(cx,x,vtype);
        if (skip)
        {
          L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
          {
            vskip = VECSKIP(v);
            if (!(vskip&(1<<0))) VVALUE(v,cx0) = from + scale*(DOUBLE)rand();
            else VVALUE(v,cx0) = 0.0;
          }
        }
        else
        {
          L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
          VVALUE(v,cx0) = from + scale*(DOUBLE)rand();
        }
        break;

      case 2 :
        SET_VD_CMP_2(cx,x,vtype);
        if (skip)
        {
          L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
          {
            vskip = VECSKIP(v);
            if (!(vskip&(1<<0))) VVALUE(v,cx0) = from + scale*(DOUBLE)rand();
            else VVALUE(v,cx0) = 0.0;
            if (!(vskip&(1<<1))) VVALUE(v,cx1) = from + scale*(DOUBLE)rand();
            else VVALUE(v,cx1) = 0.0;
          }
        }
        else
        {
          L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
          {
            VVALUE(v,cx0) = from + scale*(DOUBLE)rand(); VVALUE(v,cx1) = from + scale*(DOUBLE)rand();
          }
        }
        break;

      case 3 :
        SET_VD_CMP_3(cx,x,vtype);
        if (skip)
        {
          L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
          {
            vskip = VECSKIP(v);
            if (!(vskip&(1<<0))) VVALUE(v,cx0) = from + scale*(DOUBLE)rand();
            else VVALUE(v,cx0) = 0.0;
            if (!(vskip&(1<<1))) VVALUE(v,cx1) = from + scale*(DOUBLE)rand();
            else VVALUE(v,cx1) = 0.0;
            if (!(vskip&(1<<2))) VVALUE(v,cx2) = from + scale*(DOUBLE)rand();
            else VVALUE(v,cx2) = 0.0;
          }
        }
        else
        {
          L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
          {
            VVALUE(v,cx0) = from + scale*(DOUBLE)rand(); VVALUE(v,cx1) = from + scale*(DOUBLE)rand(); VVALUE(v,cx2) = from + scale*(DOUBLE)rand();
          }
        }
        break;

      default :
        ncomp = VD_NCMPS_IN_TYPE(x,vtype);
        if (skip)
        {
          L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
          {
            vskip = VECSKIP(v);
            for (i=0; i<ncomp; i++)
            {
              if (!(vskip&(1<<i)))
                VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i)) = from + scale*(DOUBLE)rand();
              else
                VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i)) = 0.0;
            }
          }
        }
        else
        {
          L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
          for (i=0; i<ncomp; i++)
            VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i)) = from + scale*(DOUBLE)rand();
        }
      }

        #ifdef ModelP
  if (l_vector_consistent(g,x))
    return (NUM_ERROR);
        #endif

  return (NUM_OK);
}

/****************************************************************************/
/*D
   l_dsetnonskip - set all !skip components of a vector to a given value

   SYNOPSIS:
   INT l_dsetnonskip (GRID *g, const VECDATA_DESC *x, INT xclass, DOUBLE a);

   PARAMETERS:
   .  g - pointer to grid
   .  x - vector data descriptor
   .  xclass - vector class
   .  a - the DOUBLE value

   DESCRIPTION:
   This function sets on one grid level all components of a vector for which
   the skip flag is not set to a given value.

   RETURN VALUE:
   INT
   .n    NUM_OK
   D*/
/****************************************************************************/

INT l_dsetnonskip (GRID *g, const VECDATA_DESC *x, INT xclass, DOUBLE a)
{
  VECTOR *first_v;
  register VECTOR *v;
  register SHORT i;
  register SHORT ncomp;
  register INT vtype,vskip;
  DEFINE_VD_CMPS(cx);

  first_v = FIRSTVECTOR(g);

  for (vtype=0; vtype<NVECTYPES; vtype++)
    if (VD_ISDEF_IN_TYPE(x,vtype))
      switch (VD_NCMPS_IN_TYPE(x,vtype))
      {
      case 1 :
        SET_VD_CMP_1(cx,x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        if (!(VECSKIP(v)&(1<<0)))
          VVALUE(v,cx0) = a;
        break;

      case 2 :
        SET_VD_CMP_2(cx,x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        {
          vskip = VECSKIP(v); if (!(vskip&(1<<0))) VVALUE(v,cx0) = a;if (!(vskip&(1<<1))) VVALUE(v,cx1) = a;
        }
        break;

      case 3 :
        SET_VD_CMP_3(cx,x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        {
          vskip = VECSKIP(v); if (!(vskip&(1<<0))) VVALUE(v,cx0) = a;if (!(vskip&(1<<1))) VVALUE(v,cx1) = a;if (!(vskip&(1<<2))) VVALUE(v,cx2) = a;
        }
        break;

      default :
        ncomp = VD_NCMPS_IN_TYPE(x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        {
          vskip = VECSKIP(v);
          for (i=0; i<ncomp; i++)
            if (!(vskip&(1<<i)))
              VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i)) = a;
        }
      }

  return (NUM_OK);
}

/****************************************************************************/
/*D
   a_dsetnonskip - set all !skip components of a vector to a given value

   SYNOPSIS:
   INT a_dsetnonskip (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x,
             INT xclass, DOUBLE a)

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  x - vector data descriptor
   .  xclass - vector class
   .  a - the DOUBLE value

   DESCRIPTION:
   This function sets on one grid level all components of a vector for which
   the skip flag is not set to a given value.
   It runs from level fl to level tl.

   RETURN VALUE:
   INT
   .n    NUM_OK
   D*/
/****************************************************************************/

INT a_dsetnonskip (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x, INT xclass, DOUBLE a)
{
  register VECTOR *v;
  register SHORT i;
  register SHORT ncomp;
  register INT vtype,vskip;
  INT lev;
  DEFINE_VD_CMPS(cx);

  for (vtype=0; vtype<NVECTYPES; vtype++)
    if (VD_ISDEF_IN_TYPE(x,vtype))
      switch (VD_NCMPS_IN_TYPE(x,vtype))
      {
      case 1 :
        SET_VD_CMP_1(cx,x,vtype);
        A_VLOOP__TYPE_CLASS(lev,fl,tl,v,mg,vtype,xclass)
        if (!(VECSKIP(v)&(1<<0)))
          VVALUE(v,cx0) = a;
        break;

      case 2 :
        SET_VD_CMP_2(cx,x,vtype);
        A_VLOOP__TYPE_CLASS(lev,fl,tl,v,mg,vtype,xclass)
        {
          vskip = VECSKIP(v); if (!(vskip&(1<<0))) VVALUE(v,cx0) = a;if (!(vskip&(1<<1))) VVALUE(v,cx1) = a;
        }
        break;

      case 3 :
        SET_VD_CMP_3(cx,x,vtype);
        A_VLOOP__TYPE_CLASS(lev,fl,tl,v,mg,vtype,xclass)
        {
          vskip = VECSKIP(v); if (!(vskip&(1<<0))) VVALUE(v,cx0) = a;if (!(vskip&(1<<1))) VVALUE(v,cx1) = a;if (!(vskip&(1<<2))) VVALUE(v,cx2) = a;
        }
        break;

      default :
        ncomp = VD_NCMPS_IN_TYPE(x,vtype);
        A_VLOOP__TYPE_CLASS(lev,fl,tl,v,mg,vtype,xclass)
        {
          vskip = VECSKIP(v);
          for (i=0; i<ncomp; i++)
            if (!(vskip&(1<<i)))
              VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i)) = a;
        }
      }

  return (NUM_OK);
}
/****************************************************************************/
/*D
   s_dsetnonskip - set all !skip components of a vector to a given value

   SYNOPSIS:
   INT s_dsetnonskip (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x,
   DOUBLE a)

   PARAMETERS:
   .  mg - pointer to multigrid
   .  fl - from level
   .  tl - to level
   .  x - vector data descriptor
   .  a - the DOUBLE value

   DESCRIPTION:
   This function sets on one grid level all components of a vector for which
   the skip flag is not set to a given value.
   It runs the surface of the grid, c. f. 'FINE_GRID_DOF' in 'gm.h'.

   RETURN VALUE:
   INT
   .n    NUM_OK
   D*/
/****************************************************************************/

INT s_dsetnonskip (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x, DOUBLE a)
{
  register VECTOR *v;
  register SHORT i;
  register SHORT ncomp;
  register INT vtype,vskip;
  INT lev;
  DEFINE_VD_CMPS(cx);

  for (vtype=0; vtype<NVECTYPES; vtype++)
    if (VD_ISDEF_IN_TYPE(x,vtype))
      switch (VD_NCMPS_IN_TYPE(x,vtype))
      {
      case 1 :
        SET_VD_CMP_1(cx,x,vtype);
        S_BELOW_VLOOP__TYPE(lev,fl,tl,v,mg,vtype)
        if (!(VECSKIP(v)&(1<<0)))
          VVALUE(v,cx0) = a;
        S_FINE_VLOOP__TYPE(tl,v,mg,vtype)
        if (!(VECSKIP(v)&(1<<0)))
          VVALUE(v,cx0) = a;
        break;

      case 2 :
        SET_VD_CMP_2(cx,x,vtype);
        S_BELOW_VLOOP__TYPE(lev,fl,tl,v,mg,vtype)
        {
          vskip = VECSKIP(v); if (!(vskip&(1<<0))) VVALUE(v,cx0) = a;if (!(vskip&(1<<1))) VVALUE(v,cx1) = a;
        }
        S_FINE_VLOOP__TYPE(tl,v,mg,vtype)
        {
          vskip = VECSKIP(v); if (!(vskip&(1<<0))) VVALUE(v,cx0) = a;if (!(vskip&(1<<1))) VVALUE(v,cx1) = a;
        }
        break;

      case 3 :
        SET_VD_CMP_3(cx,x,vtype);
        S_BELOW_VLOOP__TYPE(lev,fl,tl,v,mg,vtype)
        {
          vskip = VECSKIP(v); if (!(vskip&(1<<0))) VVALUE(v,cx0) = a;if (!(vskip&(1<<1))) VVALUE(v,cx1) = a;if (!(vskip&(1<<2))) VVALUE(v,cx2) = a;
        }
        S_FINE_VLOOP__TYPE(tl,v,mg,vtype)
        {
          vskip = VECSKIP(v); if (!(vskip&(1<<0))) VVALUE(v,cx0) = a;if (!(vskip&(1<<1))) VVALUE(v,cx1) = a;if (!(vskip&(1<<2))) VVALUE(v,cx2) = a;
        }
        break;

      default :
        ncomp = VD_NCMPS_IN_TYPE(x,vtype);
        S_BELOW_VLOOP__TYPE(lev,fl,tl,v,mg,vtype)
        {
          vskip = VECSKIP(v);
          for (i=0; i<ncomp; i++)
            if (!(vskip&(1<<i)))
              VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i)) = a;
        }
        S_FINE_VLOOP__TYPE(tl,v,mg,vtype)
        {
          vskip = VECSKIP(v);
          for (i=0; i<ncomp; i++)
            if (!(vskip&(1<<i)))
              VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i)) = a;
        }
      }

  return (NUM_OK);
}

/****************************************************************************/
/*D
   l_dsetskip - set all skip components of a vector to a given value

   SYNOPSIS:
   INT l_dsetskip (GRID *g, const VECDATA_DESC *x, INT xclass, DOUBLE a);

   PARAMETERS:
   .  g - pointer to grid
   .  x - vector data descriptor
   .  xclass - vector class
   .  a - the DOUBLE value

   DESCRIPTION:
   This function sets on one grid level all components of a vector for which
   the skip flag is not set to a given value.

   RETURN VALUE:
   INT
   .n    NUM_OK
   D*/
/****************************************************************************/

INT l_dsetskip (GRID *g, const VECDATA_DESC *x, INT xclass, DOUBLE a)
{
  VECTOR *first_v;
  register VECTOR *v;
  register SHORT i;
  register SHORT ncomp;
  register INT vtype,vskip;
  DEFINE_VD_CMPS(cx);

  first_v = FIRSTVECTOR(g);

  for (vtype=0; vtype<NVECTYPES; vtype++)
    if (VD_ISDEF_IN_TYPE(x,vtype))
      switch (VD_NCMPS_IN_TYPE(x,vtype))
      {
      case 1 :
        SET_VD_CMP_1(cx,x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        if ((VECSKIP(v)&(1<<0)))
          VVALUE(v,cx0) = a;
        break;

      case 2 :
        SET_VD_CMP_2(cx,x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        {
          vskip = VECSKIP(v); if ((vskip&(1<<0))) VVALUE(v,cx0) = a;if ((vskip&(1<<1))) VVALUE(v,cx1) = a;
        }
        break;

      case 3 :
        SET_VD_CMP_3(cx,x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        {
          vskip = VECSKIP(v); if ((vskip&(1<<0))) VVALUE(v,cx0) = a;if ((vskip&(1<<1))) VVALUE(v,cx1) = a;if ((vskip&(1<<2))) VVALUE(v,cx2) = a;
        }
        break;

      default :
        ncomp = VD_NCMPS_IN_TYPE(x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        {
          vskip = VECSKIP(v);
          for (i=0; i<ncomp; i++)
            if ((vskip&(1<<i)))
              VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i)) = a;
        }
      }

  return (NUM_OK);
}

/****************************************************************************/
/*D
   l_dsetfunc - set all components of a vector to a given function value

   SYNOPSIS:
   INT l_dsetfunc (GRID *g, const VECDATA_DESC *x, INT xclass,
   SetFuncProcPtr SetFunc);

   PARAMETERS:
   .  g - pointer to grid
   .  x - destination vector data descriptor
   .  xclass - vector class
   .  SetFunc - pointer to a function

   DESCRIPTION:
   This function sets all components of a vector to a given function value
   of the type

   'typedef INT (*SetFuncProcPtr) (const DOUBLE_VECTOR Global, SHORT vtype,'
   'DOUBLE *val);'

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_ERROR if the function could not be evaluated for a VECTOR
   .n    if NDEBUG is defined:
   .n    NUM_BLOCK_TOO_LARGE if the blocks are larger as MAX_SINGLE_VEC_COMP
   D*/
/****************************************************************************/

INT l_dsetfunc (GRID *g, const VECDATA_DESC *x, INT xclass, SetFuncProcPtr SetFunc)
{
  VECTOR *first_v;
  DOUBLE val[MAX_SINGLE_VEC_COMP];
  DOUBLE_VECTOR Point;
  INT maxsmallblock;
  register VECTOR *v;
  register SHORT i;
  register SHORT ncomp;
  register INT vtype;
  DEFINE_VD_CMPS(cx);

#ifndef NDEBUG
  /* check maximal block size */
  maxsmallblock = 0;
  for (vtype=0; vtype<NVECTYPES; vtype++)
    if (VD_ISDEF_IN_TYPE(x,vtype))
      maxsmallblock = MAX(maxsmallblock,VD_NCMPS_IN_TYPE(x,vtype));

  /* check size of the largest small block */
  assert (maxsmallblock <= MAX_SINGLE_VEC_COMP);        /* if too little: increase MAX_SINGLE_VEC_COMP and recompile */
#endif
#ifdef NDEBUG
  /* check also in case NDEBUG is defined (assert off)	*/
  if (maxsmallblock > MAX_SINGLE_VEC_COMP)
    REP_ERR_RETURN (NUM_BLOCK_TOO_LARGE);
#endif

  first_v = FIRSTVECTOR(g);

  for (vtype=0; vtype<NVECTYPES; vtype++)
    if (VD_ISDEF_IN_TYPE(x,vtype))
      switch (VD_NCMPS_IN_TYPE(x,vtype))
      {
      case 1 :
        SET_VD_CMP_1(cx,x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        {
          if (VectorPosition(v,Point)) REP_ERR_RETURN (NUM_ERROR);
          if (SetFunc(Point,vtype,val)!=0) REP_ERR_RETURN (NUM_ERROR);
          VVALUE(v,cx0) = val[0];
        }
        break;

      case 2 :
        SET_VD_CMP_2(cx,x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        {
          if (VectorPosition(v,Point)) REP_ERR_RETURN (NUM_ERROR);
          if (SetFunc(Point,vtype,val)!=0) REP_ERR_RETURN (NUM_ERROR);
          VVALUE(v,cx0) = val[0];
          VVALUE(v,cx1) = val[1];
        }
        break;

      case 3 :
        SET_VD_CMP_3(cx,x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        {
          if (VectorPosition(v,Point)) REP_ERR_RETURN (NUM_ERROR);
          if (SetFunc(Point,vtype,val)!=0) REP_ERR_RETURN (NUM_ERROR);
          VVALUE(v,cx0) = val[0]; VVALUE(v,cx1) = val[1]; VVALUE(v,cx2) = val[2];
        }
        break;

      default :
        ncomp = VD_NCMPS_IN_TYPE(x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        {
          if (VectorPosition(v,Point)) REP_ERR_RETURN (NUM_ERROR);
          if (SetFunc(Point,vtype,val)!=0) REP_ERR_RETURN (NUM_ERROR);
          for (i=0; i<ncomp; i++)
            VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i)) = val[i];
        }
      }

  return (NUM_OK);
}

INT l_dcopy_SB (BLOCKVECTOR *theBV, const VECDATA_DESC *x, INT xclass, const VECDATA_DESC *y)
{
  VECTOR *first_v,*end_v;
  register VECTOR *v;
  register SHORT i;
  register SHORT ncomp;
  register INT vtype,err;
  DEFINE_VD_CMPS(cx);
  DEFINE_VD_CMPS(cy);

#ifndef NDEBUG
  /* check consistency */
  if ((err = VecCheckConsistency(x,y))!=NUM_OK)
    REP_ERR_RETURN(err);
#endif

  first_v = BVFIRSTVECTOR(theBV);
  end_v = BVENDVECTOR(theBV);

  for (vtype=0; vtype<NVECTYPES; vtype++)
    if (VD_ISDEF_IN_TYPE(x,vtype))
      switch (VD_NCMPS_IN_TYPE(x,vtype))
      {
      case 1 :
        SET_VD_CMP_1(cx,x,vtype);
        SET_VD_CMP_1(cy,y,vtype);
        L_VLOOP__TYPE_CLASS2(v,first_v,end_v,vtype,xclass)
        VVALUE(v,cx0) = VVALUE(v,cy0);
        break;

      case 2 :
        SET_VD_CMP_2(cx,x,vtype);
        SET_VD_CMP_2(cy,y,vtype);
        L_VLOOP__TYPE_CLASS2(v,first_v,end_v,vtype,xclass)
        {
          VVALUE(v,cx0) = VVALUE(v,cy0); VVALUE(v,cx1) = VVALUE(v,cy1);
        }
        break;

      case 3 :
        SET_VD_CMP_3(cx,x,vtype);
        SET_VD_CMP_3(cy,y,vtype);
        L_VLOOP__TYPE_CLASS2(v,first_v,end_v,vtype,xclass)
        {
          VVALUE(v,cx0) = VVALUE(v,cy0); VVALUE(v,cx1) = VVALUE(v,cy1); VVALUE(v,cx2) = VVALUE(v,cy2);
        }
        break;

      default :
        ncomp = VD_NCMPS_IN_TYPE(x,vtype);
        L_VLOOP__TYPE_CLASS2(v,first_v,end_v,vtype,xclass)
        for (i=0; i<ncomp; i++)
          VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i)) = VVALUE(v,VD_CMP_OF_TYPE(y,vtype,i));
      }

  return (NUM_OK);
}

INT l_dscale_SB (BLOCKVECTOR *theBV, const VECDATA_DESC *x, INT xclass, const DOUBLE *a)
{
  VECTOR *first_v,*end_v;
  const DOUBLE *value;
  register VECTOR *v;
  register SHORT i;
  register SHORT ncomp;
  register INT vtype;
  const SHORT *aoff;
  DEFINE_VS_CMPS(a);
  DEFINE_VD_CMPS(cx);

  aoff = VD_OFFSETPTR(x);
  first_v = BVFIRSTVECTOR(theBV);
  end_v = BVENDVECTOR(theBV);

  for (vtype=0; vtype<NVECTYPES; vtype++)
    if (VD_ISDEF_IN_TYPE(x,vtype))
      switch (VD_NCMPS_IN_TYPE(x,vtype))
      {
      case 1 :
        SET_VD_CMP_1(cx,x,vtype);
        SET_VS_CMP_1(a,a,aoff,vtype);
        L_VLOOP__TYPE_CLASS2(v,first_v,end_v,vtype,xclass)
        VVALUE(v,cx0) *= a0;
        break;

      case 2 :
        SET_VD_CMP_2(cx,x,vtype);
        SET_VS_CMP_2(a,a,aoff,vtype);
        L_VLOOP__TYPE_CLASS2(v,first_v,end_v,vtype,xclass)
        {
          VVALUE(v,cx0) *= a0; VVALUE(v,cx1) *= a1;
        }
        break;

      case 3 :
        SET_VD_CMP_3(cx,x,vtype);
        SET_VS_CMP_3(a,a,aoff,vtype);
        L_VLOOP__TYPE_CLASS2(v,first_v,end_v,vtype,xclass)
        {
          VVALUE(v,cx0) *= a0; VVALUE(v,cx1) *= a1; VVALUE(v,cx2) *= a2;
        }
        break;

      default :
        ncomp = VD_NCMPS_IN_TYPE(x,vtype);
        value = a+aoff[vtype];
        L_VLOOP__TYPE_CLASS2(v,first_v,end_v,vtype,xclass)
        for (i=0; i<ncomp; i++)
          VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i)) *= value[i];
      }

  return (NUM_OK);
}

INT l_daxpy_SB (BLOCKVECTOR *theBV, const VECDATA_DESC *x, INT xclass, const DOUBLE *a, const VECDATA_DESC *y)
{
  VECTOR *first_v,*end_v;
  const DOUBLE *value;
  register VECTOR *v;
  register SHORT i;
  register SHORT ncomp;
  register INT vtype,err;
  const SHORT *aoff;
  DEFINE_VS_CMPS(a);
  DEFINE_VD_CMPS(cx);
  DEFINE_VD_CMPS(cy);

#ifndef NDEBUG
  /* check consistency */
  if ((err = VecCheckConsistency(x,y))!=NUM_OK)
    REP_ERR_RETURN(err);
#endif

  aoff = VD_OFFSETPTR(x);
  first_v = BVFIRSTVECTOR(theBV);
  end_v = BVENDVECTOR(theBV);

  for (vtype=0; vtype<NVECTYPES; vtype++)
    if (VD_ISDEF_IN_TYPE(x,vtype))
      switch (VD_NCMPS_IN_TYPE(x,vtype))
      {
      case 1 :
        SET_VD_CMP_1(cx,x,vtype);
        SET_VD_CMP_1(cy,y,vtype);
        SET_VS_CMP_1(a,a,aoff,vtype);
        L_VLOOP__TYPE_CLASS2(v,first_v,end_v,vtype,xclass)
        VVALUE(v,cx0) += a0*VVALUE(v,cy0);
        break;

      case 2 :
        SET_VD_CMP_2(cx,x,vtype);
        SET_VD_CMP_2(cy,y,vtype);
        SET_VS_CMP_2(a,a,aoff,vtype);
        L_VLOOP__TYPE_CLASS2(v,first_v,end_v,vtype,xclass)
        {
          VVALUE(v,cx0) += a0*VVALUE(v,cy0); VVALUE(v,cx1) += a1*VVALUE(v,cy1);
        }
        break;

      case 3 :
        SET_VD_CMP_3(cx,x,vtype);
        SET_VD_CMP_3(cy,y,vtype);
        SET_VS_CMP_3(a,a,aoff,vtype);
        L_VLOOP__TYPE_CLASS2(v,first_v,end_v,vtype,xclass)
        {
          VVALUE(v,cx0) += a0*VVALUE(v,cy0); VVALUE(v,cx1) += a1*VVALUE(v,cy1); VVALUE(v,cx2) += a2*VVALUE(v,cy2);
        }
        break;

      default :
        ncomp = VD_NCMPS_IN_TYPE(x,vtype);
        value = a+aoff[vtype];
        L_VLOOP__TYPE_CLASS2(v,first_v,end_v,vtype,xclass)
        for (i=0; i<ncomp; i++)
          VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i)) += value[i]*VVALUE(v,VD_CMP_OF_TYPE(y,vtype,i));
      }

  return (NUM_OK);
}

/****************************************************************************/
/*D
   l_mean - mean of a vector

   SYNOPSIS:
   INT l_mean (const GRID *g, const VECDATA_DESC *x, INT xclass,
   DOUBLE *sp);

   PARAMETERS:
   .  g - pointer to grid
   .  x - vector data descriptor
   .  xclass - vector class
   .  sp - DOUBLE value for every component of 'x'

   DESCRIPTION:
   This function computes the mean of a vector on one grid level.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    if NDEBUG is not defined:
   .n    error code from 'VecCheckConsistency'
   D*/
/****************************************************************************/

INT l_mean (const GRID *g, const VECDATA_DESC *x, INT xclass, DOUBLE *sp)
{
  DOUBLE *value;
  VECTOR *v,*first_v;
  register SHORT i;
  register SHORT ncomp;
  INT vtype;
  const SHORT *spoff;
  DEFINE_VD_CMPS(cx);

  spoff = VD_OFFSETPTR(x);

  /* clear sp */
  for (vtype=0; vtype<NVECTYPES; vtype++)
    if (VD_ISDEF_IN_TYPE(x,vtype))
      for (i=0; i<VD_NCMPS_IN_TYPE(x,vtype); i++)
        sp[spoff[vtype]+i] = 0.0;

  first_v = FIRSTVECTOR(g);

  for (vtype=0; vtype<NVECTYPES; vtype++)
    if (VD_ISDEF_IN_TYPE(x,vtype))
    {
      value = sp+spoff[vtype];

      switch (VD_NCMPS_IN_TYPE(x,vtype))
      {
      case 1 :
        SET_VD_CMP_1(cx,x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        value[0] += VVALUE(v,cx0);
        break;

      case 2 :
        SET_VD_CMP_2(cx,x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        {
          value[0] += VVALUE(v,cx0); value[1] += VVALUE(v,cx1);
        }
        break;

      case 3 :
        SET_VD_CMP_3(cx,x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        {
          value[0] += VVALUE(v,cx0); value[1] += VVALUE(v,cx1); value[2] += VVALUE(v,cx2);
        }
        break;

      default :
        ncomp = VD_NCMPS_IN_TYPE(x,vtype);
        L_VLOOP__TYPE_CLASS(v,first_v,vtype,xclass)
        for (i=0; i<ncomp; i++)
          value[i] += VVALUE(v,VD_CMP_OF_TYPE(x,vtype,i));
      }
    }

  return (NUM_OK);
}

INT l_dmatset_SB (BLOCKVECTOR *dest, BLOCKVECTOR *source,const MATDATA_DESC *M, DOUBLE a)
{
  register VECTOR *v,*first_v, *end_v;
  register MATRIX *m;
  INT rtype,ctype,first_index,last_index;
  register SHORT i;
  register SHORT nr;
  DEFINE_MD_CMPS(m);

  first_v = BVFIRSTVECTOR(dest);
  end_v = BVENDVECTOR(dest);
  first_index = VINDEX(BVFIRSTVECTOR(source));
  last_index  = VINDEX(BVLASTVECTOR(source));

  for (rtype=0; rtype<NVECTYPES; rtype++)
    for (ctype=0; ctype<NVECTYPES; ctype++)
      if (MD_ISDEF_IN_RT_CT(M,rtype,ctype))
        switch (MAT_RCKIND(M,rtype,ctype))
        {
        case R1C1 :
          SET_MD_CMP_11(m,M,rtype,ctype);
          L_MLOOP__RCTYPE2(v,first_v,end_v,m,rtype,ctype)
          if (MDESTINDEX(m)>=first_index && MDESTINDEX(m)<=last_index)
            MVALUE(m,m00) = a;
          break;

        case R1C2 :
          SET_MD_CMP_12(m,M,rtype,ctype);
          L_MLOOP__RCTYPE2(v,first_v,end_v,m,rtype,ctype)
          if (MDESTINDEX(m)>=first_index && MDESTINDEX(m)<=last_index)
          {MVALUE(m,m00) = a; MVALUE(m,m01) = a;}
          break;

        case R1C3 :
          SET_MD_CMP_13(m,M,rtype,ctype);
          L_MLOOP__RCTYPE2(v,first_v,end_v,m,rtype,ctype)
          if (MDESTINDEX(m)>=first_index && MDESTINDEX(m)<=last_index)
          {MVALUE(m,m00) = a; MVALUE(m,m01) = a; MVALUE(m,m02) = a;}
          break;

        case R2C1 :
          SET_MD_CMP_21(m,M,rtype,ctype);
          L_MLOOP__RCTYPE2(v,first_v,end_v,m,rtype,ctype)
          if (MDESTINDEX(m)>=first_index && MDESTINDEX(m)<=last_index)
          {MVALUE(m,m00) = a;
           MVALUE(m,m10) = a;}
          break;

        case R2C2 :
          SET_MD_CMP_22(m,M,rtype,ctype);
          L_MLOOP__RCTYPE2(v,first_v,end_v,m,rtype,ctype)
          if (MDESTINDEX(m)>=first_index && MDESTINDEX(m)<=last_index)
          {MVALUE(m,m00) = a; MVALUE(m,m01) = a;
           MVALUE(m,m10) = a; MVALUE(m,m11) = a;}
          break;

        case R2C3 :
          SET_MD_CMP_23(m,M,rtype,ctype);
          L_MLOOP__RCTYPE2(v,first_v,end_v,m,rtype,ctype)
          if (MDESTINDEX(m)>=first_index && MDESTINDEX(m)<=last_index)
          {MVALUE(m,m00) = a; MVALUE(m,m01) = a; MVALUE(m,m02) = a;
           MVALUE(m,m10) = a; MVALUE(m,m11) = a; MVALUE(m,m12) = a;}
          break;

        case R3C1 :
          SET_MD_CMP_31(m,M,rtype,ctype);
          L_MLOOP__RCTYPE2(v,first_v,end_v,m,rtype,ctype)
          if (MDESTINDEX(m)>=first_index && MDESTINDEX(m)<=last_index)
          {MVALUE(m,m00) = a;
           MVALUE(m,m10) = a;
           MVALUE(m,m20) = a;}
          break;

        case R3C2 :
          SET_MD_CMP_32(m,M,rtype,ctype);
          L_MLOOP__RCTYPE2(v,first_v,end_v,m,rtype,ctype)
          if (MDESTINDEX(m)>=first_index && MDESTINDEX(m)<=last_index)
          {MVALUE(m,m00) = a; MVALUE(m,m01) = a;
           MVALUE(m,m10) = a; MVALUE(m,m11) = a;
           MVALUE(m,m20) = a; MVALUE(m,m21) = a;}
          break;

        case R3C3 :
          SET_MD_CMP_33(m,M,rtype,ctype);
          L_MLOOP__RCTYPE2(v,first_v,end_v,m,rtype,ctype)
          if (MDESTINDEX(m)>=first_index && MDESTINDEX(m)<=last_index)
          {MVALUE(m,m00) = a; MVALUE(m,m01) = a; MVALUE(m,m02) = a;
           MVALUE(m,m10) = a; MVALUE(m,m11) = a; MVALUE(m,m12) = a;
           MVALUE(m,m20) = a; MVALUE(m,m21) = a; MVALUE(m,m22) = a;}
          break;

        default :
          nr   = MD_ROWS_IN_RT_CT(M,rtype,ctype) * MD_COLS_IN_RT_CT(M,rtype,ctype);
          L_MLOOP__RCTYPE2(v,first_v,end_v,m,rtype,ctype)
          if (MDESTINDEX(m)>=first_index && MDESTINDEX(m)<=last_index)
            for (i=0; i<nr; i++)
              MVALUE(m,MD_MCMP_OF_RT_CT(M,rtype,ctype,i)) = a;
        }

  return(NUM_OK);
}

/****************************************************************************/
/*D
   l_dmattranspose - transpose a matrix

   SYNOPSIS:
   INT l_dmattranspose (GRID *g, const MATDATA_DESC *M1, const MATDATA_DESC *M2);

   PARAMETERS:
   .  g - pointer to grid
   .  M1 - destination matrix data descriptor (transpose matrix)
   .  M2 - source matrix data descriptor

   DESCRIPTION:
   This function copies a matrix `M1 = M2-transpose` on one grid level.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    if NDEBUG is not defined:
   .n    NUM_DESC_MISMATCH if the type descriptors not match.
   D*/
/****************************************************************************/

INT l_dmattranspose (GRID *g, const MATDATA_DESC *M1, const MATDATA_DESC *M2)
{
  register VECTOR *v,*first_v;
  register MATRIX *m;
  INT rtype,ctype;
  register SHORT i;
  register SHORT nr;
  DEFINE_MD_CMPS(m);
  DEFINE_MD_CMPS(mc);

#ifndef NDEBUG
  for (rtype=0; rtype<NVECTYPES; rtype++)
    for (ctype=0; ctype<NVECTYPES; ctype++)
      if (MD_ISDEF_IN_RT_CT(M1,rtype,ctype))
      {
        /* consistency check: the M1-types should include the M2-types */
        if (!MD_ISDEF_IN_RT_CT(M2,rtype,ctype))
          REP_ERR_RETURN (NUM_DESC_MISMATCH);

        /* consistency check: the M1-nRow/ColComp should be equal to the M2-nRow/ColComp */
        if (MD_ROWS_IN_RT_CT(M1,rtype,ctype) != MD_ROWS_IN_RT_CT(M2,rtype,ctype))
          REP_ERR_RETURN (NUM_DESC_MISMATCH);
        if (MD_COLS_IN_RT_CT(M1,rtype,ctype) != MD_COLS_IN_RT_CT(M2,rtype,ctype))
          REP_ERR_RETURN (NUM_DESC_MISMATCH);
      }
#endif

  first_v = FIRSTVECTOR(g);

  for (rtype=0; rtype<NVECTYPES; rtype++)
    for (ctype=0; ctype<NVECTYPES; ctype++)
      if (MD_ISDEF_IN_RT_CT(M1,rtype,ctype))
        switch (MAT_RCKIND(M1,rtype,ctype))
        {
        case R1C1 :
          SET_MD_CMP_11(m,M1,rtype,ctype);
          SET_MD_CMP_11(mc,M2,rtype,ctype);
          L_MLOOP__RCTYPE(v,first_v,m,rtype,ctype)
          MVALUE(m,m00) = MVALUE(MADJ(m),mc00);
          break;

        case R1C2 :
          SET_MD_CMP_12(m,M1,rtype,ctype);
          SET_MD_CMP_12(mc,M2,rtype,ctype);
          L_MLOOP__RCTYPE(v,first_v,m,rtype,ctype)
          {
            MVALUE(m,m00) = MVALUE(MADJ(m),mc00); MVALUE(m,m01) = MVALUE(MADJ(m),mc01);
          }
          break;

        case R1C3 :
          SET_MD_CMP_13(m,M1,rtype,ctype);
          SET_MD_CMP_13(mc,M2,rtype,ctype);
          L_MLOOP__RCTYPE(v,first_v,m,rtype,ctype)
          {
            MVALUE(m,m00) = MVALUE(MADJ(m),mc00); MVALUE(m,m01) = MVALUE(MADJ(m),mc01); MVALUE(m,m02) = MVALUE(MADJ(m),mc02);
          }
          break;

        case R2C1 :
          SET_MD_CMP_21(m,M1,rtype,ctype);
          SET_MD_CMP_21(mc,M2,rtype,ctype);
          L_MLOOP__RCTYPE(v,first_v,m,rtype,ctype)
          {
            MVALUE(m,m00) = MVALUE(MADJ(m),mc00);
            MVALUE(m,m10) = MVALUE(MADJ(m),mc10);
          }
          break;

        case R2C2 :
          SET_MD_CMP_22(m,M1,rtype,ctype);
          SET_MD_CMP_22(mc,M2,rtype,ctype);
          L_MLOOP__RCTYPE(v,first_v,m,rtype,ctype)
          {
            MVALUE(m,m00) = MVALUE(MADJ(m),mc00); MVALUE(m,m01) = MVALUE(MADJ(m),mc01);
            MVALUE(m,m10) = MVALUE(MADJ(m),mc10); MVALUE(m,m11) = MVALUE(MADJ(m),mc11);
          }
          break;

        case R2C3 :
          SET_MD_CMP_23(m,M1,rtype,ctype);
          SET_MD_CMP_23(mc,M2,rtype,ctype);
          L_MLOOP__RCTYPE(v,first_v,m,rtype,ctype)
          {
            MVALUE(m,m00) = MVALUE(MADJ(m),mc00); MVALUE(m,m01) = MVALUE(MADJ(m),mc01); MVALUE(m,m02) = MVALUE(MADJ(m),mc02);
            MVALUE(m,m10) = MVALUE(MADJ(m),mc10); MVALUE(m,m11) = MVALUE(MADJ(m),mc11); MVALUE(m,m12) = MVALUE(MADJ(m),mc12);
          }
          break;

        case R3C1 :
          SET_MD_CMP_31(m,M1,rtype,ctype);
          SET_MD_CMP_31(mc,M2,rtype,ctype);
          L_MLOOP__RCTYPE(v,first_v,m,rtype,ctype)
          {
            MVALUE(m,m00) = MVALUE(MADJ(m),mc00);
            MVALUE(m,m10) = MVALUE(MADJ(m),mc10);
            MVALUE(m,m20) = MVALUE(MADJ(m),mc20);
          }
          break;

        case R3C2 :
          SET_MD_CMP_32(m,M1,rtype,ctype);
          SET_MD_CMP_32(mc,M2,rtype,ctype);
          L_MLOOP__RCTYPE(v,first_v,m,rtype,ctype)
          {
            MVALUE(m,m00) = MVALUE(MADJ(m),mc00); MVALUE(m,m01) = MVALUE(MADJ(m),mc01);
            MVALUE(m,m10) = MVALUE(MADJ(m),mc10); MVALUE(m,m11) = MVALUE(MADJ(m),mc11);
            MVALUE(m,m20) = MVALUE(MADJ(m),mc20); MVALUE(m,m21) = MVALUE(MADJ(m),mc21);
          }
          break;

        case R3C3 :
          SET_MD_CMP_33(m,M1,rtype,ctype);
          SET_MD_CMP_33(mc,M2,rtype,ctype);
          L_MLOOP__RCTYPE(v,first_v,m,rtype,ctype)
          {
            MVALUE(m,m00) = MVALUE(MADJ(m),mc00); MVALUE(m,m01) = MVALUE(MADJ(m),mc01); MVALUE(m,m02) = MVALUE(MADJ(m),mc02);
            MVALUE(m,m10) = MVALUE(MADJ(m),mc10); MVALUE(m,m11) = MVALUE(MADJ(m),mc11); MVALUE(m,m12) = MVALUE(MADJ(m),mc12);
            MVALUE(m,m20) = MVALUE(MADJ(m),mc20); MVALUE(m,m21) = MVALUE(MADJ(m),mc21); MVALUE(m,m22) = MVALUE(MADJ(m),mc22);
          }
          break;

        default :
          nr = MD_ROWS_IN_RT_CT(M1,rtype,ctype) * MD_COLS_IN_RT_CT(M1,rtype,ctype);
          L_MLOOP__RCTYPE(v,first_v,m,rtype,ctype)
          for (i=0; i<nr; i++)
            MVALUE(m,MD_MCMP_OF_RT_CT(M1,rtype,ctype,i)) =
              MVALUE(MADJ(m),MD_MCMP_OF_RT_CT(M2,rtype,ctype,i));
        }

  return (NUM_OK);
}

INT l_dmatmul_SB (BLOCKVECTOR *theBVX, const VECDATA_DESC *x, INT xclass, const MATDATA_DESC *M, BLOCKVECTOR *theBVY, const VECDATA_DESC *y, INT yclass)
{
  register VECTOR *v,*w,*first_v,*end_v;
  register MATRIX *mat;
  INT err,xmask,ymask,first_index,last_index;
  register SHORT xc,yc,mc;
  DOUBLE sum;

#ifndef NDEBUG
  if ((err=MatmulCheckConsistency(x,M,y))!=NUM_OK)
    REP_ERR_RETURN (err);
#endif

  first_v = BVFIRSTVECTOR(theBVX);
  end_v = BVENDVECTOR(theBVX);
  first_index = VINDEX(BVFIRSTVECTOR(theBVY));
  last_index = VINDEX(BVLASTVECTOR(theBVY));

  if (MD_IS_SCALAR(M))
  {
    xc    = VD_SCALCMP(x);
    mc    = MD_SCALCMP(M);
    yc    = VD_SCALCMP(y);
    xmask = VD_SCALTYPEMASK(x);
    ymask = VD_SCALTYPEMASK(y);

    for (v=first_v; v!= end_v; v=SUCCVC(v))
    {
      if ( (VDATATYPE(v)&xmask) && (VCLASS(v)>=xclass) )
      {
        sum = 0.0;
        for (mat=VSTART(v); mat!=NULL; mat = MNEXT(mat))
        {
          w = MDEST(mat);
          if ( (VDATATYPE(w)&ymask) && (VCLASS(w)>=yclass) && (VINDEX(w)>=first_index) && (VINDEX(w)<=last_index))
            sum += MVALUE(mat,mc) * VVALUE(w,yc);
        }
        VVALUE(v,xc) += sum;
      }
    }

    return (NUM_OK);
  }

  return (NUM_ERROR);
}

INT l_dtpmatmul_SB (BLOCKVECTOR *theBVX, const VECDATA_DESC *x, INT xclass, const MATDATA_DESC *M, BLOCKVECTOR *theBVY, const VECDATA_DESC *y, INT yclass)
{
  register VECTOR *v,*w,*first_v,*end_v;
  register MATRIX *mat;
  INT err,xmask,ymask,first_index,last_index;
  register SHORT xc,yc,mc;
  DOUBLE sum;

#ifndef NDEBUG
  if ((err=MatmulCheckConsistency(x,M,y))!=NUM_OK)
    REP_ERR_RETURN (err);
#endif

  first_v = BVFIRSTVECTOR(theBVX);
  end_v = BVENDVECTOR(theBVX);
  first_index = VINDEX(BVFIRSTVECTOR(theBVY));
  last_index = VINDEX(BVLASTVECTOR(theBVY));

  if (MD_IS_SCALAR(M))
  {
    xc    = VD_SCALCMP(x);
    mc    = MD_SCALCMP(M);
    yc    = VD_SCALCMP(y);
    xmask = VD_SCALTYPEMASK(x);
    ymask = VD_SCALTYPEMASK(y);

    for (v=first_v; v!= end_v; v=SUCCVC(v))
    {
      if ( (VDATATYPE(v)&xmask) && (VCLASS(v)>=xclass) )
      {
        sum = 0.0;
        for (mat=VSTART(v); mat!=NULL; mat = MNEXT(mat))
        {
          w = MDEST(mat);
          if ( (VDATATYPE(w)&ymask) && (VCLASS(w)>=yclass) && (VINDEX(w)>=first_index) && (VINDEX(w)<=last_index))
            sum += MVALUE(MADJ(mat),mc) * VVALUE(w,yc);
        }
        VVALUE(v,xc) += sum;
      }
    }

    return (NUM_OK);
  }

  return (NUM_ERROR);
}

INT l_dmatmul_set_SB (BLOCKVECTOR *theBVX, const VECDATA_DESC *x, INT xclass, const MATDATA_DESC *M, BLOCKVECTOR *theBVY, const VECDATA_DESC *y, INT yclass)
{
  register VECTOR *v,*w,*first_v,*end_v;
  register MATRIX *mat;
  INT err,xmask,ymask,first_index,last_index;
  register SHORT xc,yc,mc;
  DOUBLE sum;

#ifndef NDEBUG
  if ((err=MatmulCheckConsistency(x,M,y))!=NUM_OK)
    REP_ERR_RETURN (err);
#endif

  first_v = BVFIRSTVECTOR(theBVX);
  end_v = BVENDVECTOR(theBVX);
  first_index = VINDEX(BVFIRSTVECTOR(theBVY));
  last_index = VINDEX(BVLASTVECTOR(theBVY));

  if (MD_IS_SCALAR(M) && VD_IS_SCALAR(y) && VD_IS_SCALAR(x))
  {
    xc    = VD_SCALCMP(x);
    mc    = MD_SCALCMP(M);
    yc    = VD_SCALCMP(y);
    xmask = VD_SCALTYPEMASK(x);
    ymask = VD_SCALTYPEMASK(y);

    for (v=first_v; v!= end_v; v=SUCCVC(v))
    {
      if ( (VDATATYPE(v)&xmask) && (VCLASS(v)>=xclass) )
      {
        sum = 0.0;
        for (mat=VSTART(v); mat!=NULL; mat = MNEXT(mat))
        {
          w = MDEST(mat);
          if ( (VDATATYPE(w)&ymask) && (VCLASS(w)>=yclass) && (VINDEX(w)>=first_index) && (VINDEX(w)<=last_index))
            sum += MVALUE(mat,mc) * VVALUE(w,yc);
        }
        VVALUE(v,xc) = sum;
      }
    }

    return (NUM_OK);
  }

  return (NUM_ERROR);
}

INT l_dtpmatmul_set_SB (BLOCKVECTOR *theBVX, const VECDATA_DESC *x, INT xclass, const MATDATA_DESC *M, BLOCKVECTOR *theBVY, const VECDATA_DESC *y, INT yclass)
{
  register VECTOR *v,*w,*first_v,*end_v;
  register MATRIX *mat;
  INT err,xmask,ymask,first_index,last_index;
  register SHORT xc,yc,mc;
  DOUBLE sum;

#ifndef NDEBUG
  if ((err=MatmulCheckConsistency(x,M,y))!=NUM_OK)
    REP_ERR_RETURN (err);
#endif

  first_v = BVFIRSTVECTOR(theBVX);
  end_v = BVENDVECTOR(theBVX);
  first_index = VINDEX(BVFIRSTVECTOR(theBVY));
  last_index = VINDEX(BVLASTVECTOR(theBVY));

  if (MD_IS_SCALAR(M) && VD_IS_SCALAR(y) && VD_IS_SCALAR(x))
  {
    xc    = VD_SCALCMP(x);
    mc    = MD_SCALCMP(M);
    yc    = VD_SCALCMP(y);
    xmask = VD_SCALTYPEMASK(x);
    ymask = VD_SCALTYPEMASK(y);

    for (v=first_v; v!= end_v; v=SUCCVC(v))
    {
      if ( (VDATATYPE(v)&xmask) && (VCLASS(v)>=xclass) )
      {
        sum = 0.0;
        for (mat=VSTART(v); mat!=NULL; mat = MNEXT(mat))
        {
          w = MDEST(mat);
          if ( (VDATATYPE(w)&ymask) && (VCLASS(w)>=yclass) && (VINDEX(w)>=first_index) && (VINDEX(w)<=last_index))
            sum += MVALUE(MADJ(mat),mc) * VVALUE(w,yc);
        }
        VVALUE(v,xc) = sum;
      }
    }

    return (NUM_OK);
  }

  return (NUM_ERROR);
}

INT l_dmatmul_minus_SB (BLOCKVECTOR *theBVX, const VECDATA_DESC *x, INT xclass, const MATDATA_DESC *M, BLOCKVECTOR *theBVY, const VECDATA_DESC *y, INT yclass)
{
  register VECTOR *v,*w,*first_v,*end_v;
  register MATRIX *mat;
  INT err,xmask,ymask,first_index,last_index;
  register SHORT xc,yc,mc;
  DOUBLE sum;

#ifndef NDEBUG
  if ((err=MatmulCheckConsistency(x,M,y))!=NUM_OK)
    REP_ERR_RETURN (err);
#endif

  first_v = BVFIRSTVECTOR(theBVX);
  end_v = BVENDVECTOR(theBVX);
  first_index = VINDEX(BVFIRSTVECTOR(theBVY));
  last_index = VINDEX(BVLASTVECTOR(theBVY));

  if (MD_IS_SCALAR(M) && VD_IS_SCALAR(y) && VD_IS_SCALAR(x))
  {
    xc    = VD_SCALCMP(x);
    mc    = MD_SCALCMP(M);
    yc    = VD_SCALCMP(y);
    xmask = VD_SCALTYPEMASK(x);
    ymask = VD_SCALTYPEMASK(y);

    for (v=first_v; v!= end_v; v=SUCCVC(v))
    {
      if ( (VDATATYPE(v)&xmask) && (VCLASS(v)>=xclass) )
      {
        sum = 0.0;
        for (mat=VSTART(v); mat!=NULL; mat = MNEXT(mat))
        {
          w = MDEST(mat);
          if ( (VDATATYPE(w)&ymask) && (VCLASS(w)>=yclass) && (VINDEX(w)>=first_index) && (VINDEX(w)<=last_index))
            sum += MVALUE(mat,mc) * VVALUE(w,yc);
        }
        VVALUE(v,xc) -= sum;
      }
    }

    return (NUM_OK);
  }

  return (NUM_ERROR);
}

INT s_dtpmatmul_set (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x, const MATDATA_DESC *M, const VECDATA_DESC *y, INT yclass)
{
  register VECTOR *v,*w;
  register MATRIX *mat;
  INT rtype,ctype,err,xmask,ymask,lev;
  register SHORT i,j,xc,yc,mc;
  register SHORT nr,nc;
  DOUBLE s[MAX_SINGLE_VEC_COMP],sum;
  DEFINE_VD_CMPS(cx);

#ifndef NDEBUG
  if ((err=MatmulCheckConsistency(x,M,y))!=NUM_OK)
    REP_ERR_RETURN (err);
#endif

  if (MD_IS_SCALAR(M) && VD_IS_SCALAR(y) && VD_IS_SCALAR(x))
  {
    xc    = VD_SCALCMP(x);
    mc    = MD_SCALCMP(M);
    yc    = VD_SCALCMP(y);
    xmask = VD_SCALTYPEMASK(x);
    ymask = VD_SCALTYPEMASK(y);

    /* all levels below finest */
    for (lev=fl; lev<tl; lev++)
      for (v=FIRSTVECTOR(GRID_ON_LEVEL(mg,lev)); v!= NULL; v=SUCCVC(v))
        if ((VDATATYPE(v)&xmask) && (FINE_GRID_DOF(v)))
        {
          sum = 0.0;
          for (mat=VSTART(v); mat!=NULL; mat = MNEXT(mat))
          {
            w = MDEST(mat);
            if ((VDATATYPE(w)&ymask) && (VCLASS(w)>=yclass))
              sum += MVALUE(MADJ(mat),mc) * VVALUE(w,yc);
          }
          VVALUE(v,xc) = sum;
        }

    /* fine level */
    for (v=FIRSTVECTOR(GRID_ON_LEVEL(mg,tl)); v!= NULL; v=SUCCVC(v))
      if ((VDATATYPE(v)&xmask) && (NEW_DEFECT(v)))
      {
        sum = 0.0;
        for (mat=VSTART(v); mat!=NULL; mat = MNEXT(mat))
        {
          w = MDEST(mat);
          if ((VDATATYPE(w)&ymask) && (VCLASS(w)>=yclass))
            sum += MVALUE(MADJ(mat),mc) * VVALUE(w,yc);
        }
        VVALUE(v,xc) = sum;
      }

    return (NUM_OK);
  }


  for (rtype=0; rtype<NVECTYPES; rtype++)
    if (VD_ISDEF_IN_TYPE(x,rtype))
    {
      SET_VD_CMP_N(cx,x,rtype);

      for (ctype=0; ctype<NVECTYPES; ctype++)
        if (MD_ISDEF_IN_RT_CT(M,rtype,ctype))
          switch (MAT_RCKIND(M,rtype,ctype))
          {
          /*		case R1C1:
                  SET_VD_CMP_1(cy,y,ctype);
                  SET_MD_CMP_11(m,M,rtype,ctype);
                  S_BELOW_VLOOP__TYPE(lev,fl,tl,v,mg,rtype)
                  {
                          s0 = 0.0;
                          for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                                  if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                                     TPMATMUL_11(s,mat,m,w,cy)
                          VVALUE(v,cx0) = s0;
                  }
                  S_FINE_VLOOP__TYPE(tl,v,mg,rtype)
                  {
                          s0 = 0.0;
                          for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                                  if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                                     TPMATMUL_11(s,mat,m,w,cy)
                          VVALUE(v,cx0) = s0;
                  }
                  break;

             case R1C2:
                  SET_VD_CMP_2(cy,y,ctype);
                  SET_MD_CMP_12(m,M,rtype,ctype);
                  S_BELOW_VLOOP__TYPE(lev,fl,tl,v,mg,rtype)
                  {
                          s0 = 0.0;
                          for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                                  if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                                     TPMATMUL_12(s,mat,m,w,cy)
                          VVALUE(v,cx0) = s0;
                  }
                  S_FINE_VLOOP__TYPE(tl,v,mg,rtype)
                  {
                          s0 = 0.0;
                          for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                                  if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                                     TPMATMUL_12(s,mat,m,w,cy)
                          VVALUE(v,cx0) = s0;
                  }
                  break;

             case R1C3:
                  SET_VD_CMP_3(cy,y,ctype);
                  SET_MD_CMP_13(m,M,rtype,ctype);
                  S_BELOW_VLOOP__TYPE(lev,fl,tl,v,mg,rtype)
                  {
                          s0 = 0.0;
                          for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                                  if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                                     TPMATMUL_13(s,mat,m,w,cy)
                          VVALUE(v,cx0) = s0;
                  }
                  S_FINE_VLOOP__TYPE(tl,v,mg,rtype)
                  {
                          s0 = 0.0;
                          for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                                  if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                                     TPMATMUL_13(s,mat,m,w,cy)
                          VVALUE(v,cx0) = s0;
                  }
                  break;

             case R2C1:
                  SET_VD_CMP_1(cy,y,ctype);
                  SET_MD_CMP_21(m,M,rtype,ctype);
                  S_BELOW_VLOOP__TYPE(lev,fl,tl,v,mg,rtype)
                  {
                          s0 = s1 = 0.0;
                          for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                                  if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                                     TPMATMUL_21(s,mat,m,w,cy)
                          VVALUE(v,cx0) = s0;
                          VVALUE(v,cx1) = s1;
                  }
                  S_FINE_VLOOP__TYPE(tl,v,mg,rtype)
                  {
                          s0 = s1 = 0.0;
                          for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                                  if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                                     TPMATMUL_21(s,mat,m,w,cy)
                          VVALUE(v,cx0) = s0;
                          VVALUE(v,cx1) = s1;
                  }
                  break;

             case R2C2:
                  SET_VD_CMP_2(cy,y,ctype);
                  SET_MD_CMP_22(m,M,rtype,ctype);
                  S_BELOW_VLOOP__TYPE(lev,fl,tl,v,mg,rtype)
                  {
                          s0 = s1 = 0.0;
                          for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                                  if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                                     TPMATMUL_22(s,mat,m,w,cy)
                          VVALUE(v,cx0) = s0;
                          VVALUE(v,cx1) = s1;
                  }
                  S_FINE_VLOOP__TYPE(tl,v,mg,rtype)
                  {
                          s0 = s1 = 0.0;
                          for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                                  if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                                     TPMATMUL_22(s,mat,m,w,cy)
                          VVALUE(v,cx0) = s0;
                          VVALUE(v,cx1) = s1;
                  }
                  break;

             case R2C3:
                  SET_VD_CMP_3(cy,y,ctype);
                  SET_MD_CMP_23(m,M,rtype,ctype);
                  S_BELOW_VLOOP__TYPE(lev,fl,tl,v,mg,rtype)
                  {
                          s0 = s1 = 0.0;
                          for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                                  if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                                     TPMATMUL_23(s,mat,m,w,cy)
                          VVALUE(v,cx0) = s0;
                          VVALUE(v,cx1) = s1;
                  }
                  S_FINE_VLOOP__TYPE(tl,v,mg,rtype)
                  {
                          s0 = s1 = 0.0;
                          for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                                  if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                                     TPMATMUL_23(s,mat,m,w,cy)
                          VVALUE(v,cx0) = s0;
                          VVALUE(v,cx1) = s1;
                  }
                  break;

             case R3C1:
                  SET_VD_CMP_1(cy,y,ctype);
                  SET_MD_CMP_31(m,M,rtype,ctype);
                  S_BELOW_VLOOP__TYPE(lev,fl,tl,v,mg,rtype)
                  {
                          s0 = s1 = s2 = 0.0;
                          for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                                  if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                                     TPMATMUL_31(s,mat,m,w,cy)
                          VVALUE(v,cx0) = s0;
                          VVALUE(v,cx1) = s1;
                          VVALUE(v,cx2) = s2;
                  }
                  S_FINE_VLOOP__TYPE(tl,v,mg,rtype)
                  {
                          s0 = s1 = s2 = 0.0;
                          for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                                  if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                                     TPMATMUL_31(s,mat,m,w,cy)
                          VVALUE(v,cx0) = s0;
                          VVALUE(v,cx1) = s1;
                          VVALUE(v,cx2) = s2;
                  }
                  break;

             case R3C2:
                  SET_VD_CMP_2(cy,y,ctype);
                  SET_MD_CMP_32(m,M,rtype,ctype);
                  S_BELOW_VLOOP__TYPE(lev,fl,tl,v,mg,rtype)
                  {
                          s0 = s1 = s2 = 0.0;
                          for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                                  if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                                     TPMATMUL_32(s,mat,m,w,cy)
                          VVALUE(v,cx0) = s0;
                          VVALUE(v,cx1) = s1;
                          VVALUE(v,cx2) = s2;
                  }
                  S_FINE_VLOOP__TYPE(tl,v,mg,rtype)
                  {
                          s0 = s1 = s2 = 0.0;
                          for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                                  if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                                     TPMATMUL_32(s,mat,m,w,cy)
                          VVALUE(v,cx0) = s0;
                          VVALUE(v,cx1) = s1;
                          VVALUE(v,cx2) = s2;
                  }
                  break;

             case R3C3:
                  SET_VD_CMP_3(cy,y,ctype);
                  SET_MD_CMP_33(m,M,rtype,ctype);
                  S_BELOW_VLOOP__TYPE(lev,fl,tl,v,mg,rtype)
                  {
                          s0 = s1 = s2 = 0.0;
                          for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                                  if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                                     TPMATMUL_33(s,mat,m,w,cy)
                          VVALUE(v,cx0) = s0;
                          VVALUE(v,cx1) = s1;
                          VVALUE(v,cx2) = s2;
                  }
                  S_FINE_VLOOP__TYPE(tl,v,mg,rtype)
                  {
                          s0 = s1 = s2 = 0.0;
                          for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                                  if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                                     TPMATMUL_33(s,mat,m,w,cy)
                          VVALUE(v,cx0) = s0;
                          VVALUE(v,cx1) = s1;
                          VVALUE(v,cx2) = s2;
                  }
                  break; */

          default :
            nr = MD_ROWS_IN_RT_CT(M,rtype,ctype);
            nc = MD_COLS_IN_RT_CT(M,rtype,ctype);
            S_BELOW_VLOOP__TYPE(lev,fl,tl,v,mg,rtype)
            {
              for (i=0; i<nr; i++) s[i] = 0.0;
              for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                  for (i=0; i<nr; i++)
                    for (j=0; j<nc; j++)
                      s[i] += MVALUE(MADJ(mat),MD_MCMP_OF_RT_CT(M,ctype,rtype,j*nr+i)) *
                              VVALUE(w,VD_CMP_OF_TYPE(y,ctype,j));
              for (i=0; i<nr; i++) VVALUE(v,VD_CMP_OF_TYPE(x,rtype,i)) = s[i];
            }
            S_FINE_VLOOP__TYPE(tl,v,mg,rtype)
            {
              for (i=0; i<nr; i++) s[i] = 0.0;
              for (mat=VSTART(v); mat!=NULL; mat=MNEXT(mat))
                if ((VTYPE(w=MDEST(mat))==ctype) && (VCLASS(w)>=yclass))
                  for (i=0; i<nr; i++)
                    for (j=0; j<nc; j++)
                      s[i] += MVALUE(MADJ(mat),MD_MCMP_OF_RT_CT(M,ctype,rtype,j*nr+i)) *
                              VVALUE(w,VD_CMP_OF_TYPE(y,ctype,j));
              for (i=0; i<nr; i++) VVALUE(v,VD_CMP_OF_TYPE(x,rtype,i)) = s[i];
            }
          }
    }

  return (NUM_OK);
}

/****************************************************************************/
/*D
   l_dtpmatmul - transpose matrix times vector

   SYNOPSIS:
   INT l_dtpmatmul (GRID *g, const VECDATA_DESC *x, INT xclass,
   const MATDATA_DESC *M, const VECDATA_DESC *y, INT yclass);

   PARAMETERS:
   .  g - pointer to grid
   .  x - destination vector data descriptor
   .  xclass - vector class
   .  M - matrix vector descriptor
   .  y - source vector data descriptor
   .  yclass - vector class

   DESCRIPTION:
   This function computes times matrix with vector `x := x + M-Transpose * y`
   on one grid level.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    if NDEBUG is not defined:
   .n    error code from 'MatmulCheckConsistency'
   D*/
/****************************************************************************/

INT l_dtpmatmul (GRID *g, const VECDATA_DESC *x, INT xclass, const MATDATA_DESC *M, const VECDATA_DESC *y, INT yclass)
{
  register VECTOR *v,*w,*first_v;
  register MATRIX *mat;
  INT err,xmask,ymask;
  register SHORT xc,yc,mc;
  DOUBLE sum;

#ifndef NDEBUG
  if ((err=MatmulCheckConsistency(x,M,y))!=NUM_OK)
    REP_ERR_RETURN (err);
#endif

  first_v = FIRSTVECTOR(g);

  if (MD_IS_SCALAR(M) && VD_IS_SCALAR(y) && VD_IS_SCALAR(x))
  {
    xc    = VD_SCALCMP(x);
    mc    = MD_SCALCMP(M);
    yc    = VD_SCALCMP(y);
    xmask = VD_SCALTYPEMASK(x);
    ymask = VD_SCALTYPEMASK(y);

    for (v=first_v; v!= NULL; v=SUCCVC(v))
    {
      if ( (VDATATYPE(v)&xmask) && (VCLASS(v)>=xclass) )
      {
        sum = 0.0;
        for (mat=VSTART(v); mat!=NULL; mat = MNEXT(mat))
        {
          w = MDEST(mat);
          if ( (VDATATYPE(w)&ymask) && (VCLASS(w)>=yclass) )
            sum += MVALUE(MADJ(mat),mc) * VVALUE(w,yc);
        }
        VVALUE(v,xc) += sum;
      }
    }

    return (NUM_OK);
  }

  REP_ERR_RETURN (NUM_ERROR);
}

#ifdef __BLOCK_VECTOR_DESC__

/****************************************************************************/
/*
   d2matmulBS - add the product of two scalar matrices

   SYNOPSIS:
   INT d2matmulBS ( const BLOCKVECTOR *bv_row1, const BV_DESC *bvd_col1,
   const BV_DESC *bvd_col2, const BV_DESC_FORMAT *bvdf, INT M_res_comp,
   INT M1comp, INT M2comp, GRID *grid );

   PARAMETERS:
   .  bv_row1 - row-blockvector of the result matrix and matrix M1
   .  bvd_col1 - description of the column-blockvector of M1 (identical to row-blockvector of M2)
   .  bvd_col2 - description of the column-blockvector of M2 (identical to column-blockvector of the result matrix)
   .  bvdf - format to interpret the 'bvd_col's
   .  M_res_comp - position of the scalar result in the MATRIXs of the blockmatrix bv_row1 times bvd_col2
   .  M1comp - 1. operand; position of the scalar in the MATRIXs of the blockmatrix bv_row1 times bvd_col1
   .  M2comp - 2. operand; position of the scalar result in the MATRIXs of the blockmatrix bvd_col1 times bvd_col2
   .  grid - grid to allocate new matrix-entries from

   DESCRIPTION:
   This function adds the product of 2 matrices `Mres += M1 * M2`
   on one grid level.

   New matrix entries are allocated if 'grid' != 'NULL'; otherwise the product
   is treated as incomplete. Prints out the
   number of additionaly allocated matrix entries if 'mute' >= 100.

   The result matrix must be different to the input matrices.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_OUT_OF_MEM if no memory for additional matrix entries available
 */
/****************************************************************************/

INT d2matmulBS ( const BLOCKVECTOR *bv_row1, const BV_DESC *bvd_col1, const BV_DESC *bvd_col2, const BV_DESC_FORMAT *bvdf, INT M_res_comp, INT M1comp, INT M2comp, GRID *grid )
{
  register VECTOR *vi, *vj, *vk, *end_v;
  register MATRIX *mik, *mkj, *mij;
  register CONNECTION *con;
  INT extra_cons = 0;

  ASSERT( (M_res_comp >= 0) && (M1comp >= 0) && (M2comp >= 0) );

  end_v = BVENDVECTOR( bv_row1 );
  for ( vi = BVFIRSTVECTOR( bv_row1 ); vi != end_v; vi = SUCCVC( vi ) )
    for ( mik = VSTART( vi ); mik != NULL; mik = MNEXT( mik ) )
    {
      vk = MDEST( mik );

      /* if vk does not belong to the described block go to next vk */
      if ( VMATCH( vk, bvd_col1, bvdf ) )
        for ( mkj = VSTART( vk ); mkj != NULL; mkj = MNEXT( mkj ) )
        {
          vj = MDEST( mkj );

          /* if vj does not belong to the described block go to next vj */
          if ( VMATCH( vj, bvd_col2, bvdf ) )
          {
            if ( ( mij = GetMatrix( vi, vj ) ) == NULL )
            {
              if ( grid == NULL )
                continue;

              if ( (con = CreateExtraConnection( grid, vi, vj )) == NULL )
              {
                UserWrite( "Not enough memory in d2matmulBS.\n" );
                REP_ERR_RETURN(NUM_OUT_OF_MEM);
              }
              mij = CMATRIX0( con );
              extra_cons++;
            }
            MVALUE( mij, M_res_comp ) += MVALUE( mik, M1comp ) * MVALUE( mkj, M2comp );
          }
        }
    }
  if ( (GetMuteLevel() >= 100) && (extra_cons != 0) )
    UserWriteF( "%d extra connection(s) allocated in d2matmulBS.\n", extra_cons );

  return (NUM_OK);
}

/****************************************************************************/
/*
   d2matmul_minusBS - subtract the product of two scalar matrices

   SYNOPSIS:
   INT d2matmulBS ( const BLOCKVECTOR *bv_row1, const BV_DESC *bvd_col1,
   const BV_DESC *bvd_col2, const BV_DESC_FORMAT *bvdf, INT M_res_comp,
   INT M1comp, INT M2comp, GRID *grid );

   PARAMETERS:
   .  bv_row1 - row-blockvector of the result matrix and matrix M1
   .  bvd_col1 - description of the column-blockvector of M1 (identical to row-blockvector of M2)
   .  bvd_col2 - description of the column-blockvector of M2 (identical to column-blockvector of the result matrix)
   .  bvdf - format to interpret the 'bvd_col's
   .  M_res_comp - position of the scalar result in the MATRIXs of the blockmatrix bv_row1 times bvd_col2
   .  M1comp - 1. operand; position of the scalar in the MATRIXs of the blockmatrix bv_row1 times bvd_col1
   .  M2comp - 2. operand; position of the scalar result in the MATRIXs of the blockmatrix bvd_col1 times bvd_col2
   .  grid - grid to allocate new matrix-entries from

   DESCRIPTION:
   This function subtracts the product of 2 matrices `Mres -= M1 * M2`
   on one grid level.

   New matrix entries are allocated if 'grid' != 'NULL'; otherwise the product
   is treated as incomplete. Prints out the
   number of additionaly allocated matrix entries if 'mute' >= 100.

   The result matrix must be different to the input matrices.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_OUT_OF_MEM if no memory for additional matrix entries available
 */
/****************************************************************************/

INT d2matmul_minusBS ( const BLOCKVECTOR *bv_row1, const BV_DESC *bvd_col1, const BV_DESC *bvd_col2, const BV_DESC_FORMAT *bvdf, INT M_res_comp, INT M1comp, INT M2comp, GRID *grid )
{
  register VECTOR *vi, *vj, *vk, *end_v;
  register MATRIX *mik, *mkj, *mij;
  register CONNECTION *con;
  INT extra_cons = 0;

  ASSERT( (M_res_comp >= 0) && (M1comp >= 0) && (M2comp >= 0) );

  end_v = BVENDVECTOR( bv_row1 );
  for ( vi = BVFIRSTVECTOR( bv_row1 ); vi != end_v; vi = SUCCVC( vi ) )
    for ( mik = VSTART( vi ); mik != NULL; mik = MNEXT( mik ) )
    {
      vk = MDEST( mik );

      /* if vk does not belong to the described block go to next vk */
      if ( VMATCH( vk, bvd_col1, bvdf ) )
        for ( mkj = VSTART( vk ); mkj != NULL; mkj = MNEXT( mkj ) )
        {
          vj = MDEST( mkj );

          /* if vj does not belong to the described block go to next vj */
          if ( VMATCH( vj, bvd_col2, bvdf ) )
          {
            if ( ( mij = GetMatrix( vi, vj ) ) == NULL )
            {
              if ( grid == NULL )
                continue;

              if ( (con = CreateExtraConnection( grid, vi, vj )) == NULL )
              {
                UserWrite( "Not enough memory in d2matmulBS.\n" );
                REP_ERR_RETURN(GM_OUT_OF_MEM);
              }
              mij = CMATRIX0( con );
              extra_cons++;
            }
            MVALUE( mij, M_res_comp ) -= MVALUE( mik, M1comp ) * MVALUE( mkj, M2comp );
          }
        }
    }
  if ( (GetMuteLevel() >= 100) && (extra_cons != 0) )
    UserWriteF( "%d extra connection(s) allocated in d2matmul_minusBS.\n", extra_cons );

  return (NUM_OK);
}


/****************************************************************************/
/*
   d3matmulBS - add the product of three scalar matrices

   SYNOPSIS:
   INT d3matmulBS ( const BLOCKVECTOR *bv_row1, const BV_DESC *bvd_col1,
   const BV_DESC *bvd_col2, const BV_DESC *bvd_col3, const BV_DESC_FORMAT *bvdf,
   INT M_res_comp, INT M1comp, INT M2comp, INT M3comp, GRID *grid );


   PARAMETERS:
   .  bv_row1 - row-blockvector of the result matrix and matrix M1
   .  bvd_col1 - description of the column-blockvector of M1 (identical to row-blockvector of M2)
   .  bvd_col2 - description of the column-blockvector of M2 (identical to row-blockvector of M3)
   .  bvd_col3 - description of the column-blockvector of M3 (identical to column-blockvector of the result matrix)
   .  bvdf - format to interpret the 'bvd_col's
   .  M_res_comp - position of the scalar result in the MATRIXs of the blockmatrix bv_row1 times bvd_col2
   .  M1comp - 1. operand; position of the scalar in the MATRIXs of the blockmatrix bv_row1 times bvd_col1
   .  M2comp - 2. operand; position of the scalar result in the MATRIXs of the blockmatrix bvd_col1 times bvd_col2
   .  M2comp - 3. operand; position of the scalar result in the MATRIXs of the blockmatrix bvd_col2 times bvd_col3
   .  grid - grid to allocate new matrix-entries from

   DESCRIPTION:
   This function adds the product of 3 matrices `Mres += M1 * M2 * M3`
   on one grid level.

   New matrix entries are allocated if 'grid' != 'NULL'; otherwise the product
   is treated as incomplete. Prints out the
   number of additionaly allocated matrix entries if 'mute' >= 100.

   The result matrix must be different to the input matrices.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok
   .n    NUM_OUT_OF_MEM if no memory for additional matrix entries available
 */
/****************************************************************************/

INT d3matmulBS ( const BLOCKVECTOR *bv_row1, const BV_DESC *bvd_col1, const BV_DESC *bvd_col2, const BV_DESC *bvd_col3, const BV_DESC_FORMAT *bvdf, INT M_res_comp, INT M1comp, INT M2comp, INT M3comp, GRID *grid )
{
  register VECTOR *vi, *vj, *vk, *vl, *end_v;
  register MATRIX *mik, *mkl, *mlj, *mij;
  register CONNECTION *con;
  INT extra_cons = 0;

  ASSERT( (M_res_comp >= 0) && (M1comp >= 0) && (M2comp >= 0) && (M3comp >= 0) );

  end_v = BVENDVECTOR( bv_row1 );
  for ( vi = BVFIRSTVECTOR( bv_row1 ); vi != end_v; vi = SUCCVC( vi ) )
    for ( mik = VSTART( vi ); mik != NULL; mik = MNEXT( mik ) )
    {
      vk = MDEST( mik );

      /* if vk does not belong to the described block go to next vk */
      if ( VMATCH( vk, bvd_col1, bvdf ) )
        for ( mkl = VSTART( vk ); mkl != NULL; mkl = MNEXT( mkl ) )
        {
          vl = MDEST( mkl );

          /* if vl does not belong to the described block go to next vl */
          if ( VMATCH( vl, bvd_col2, bvdf ) )
          {
            for ( mlj = VSTART( vl ); mlj != NULL; mlj = MNEXT( mlj ) )
            {
              vj = MDEST( mlj );

              /* if vj does not belong to the described block go to next vj */
              if ( VMATCH( vj, bvd_col3, bvdf ) )
              {
                if ( ( mij = GetMatrix( vi, vj ) ) == NULL )
                {
                  if ( grid == NULL )
                    continue;

                  if ( (con = CreateExtraConnection( grid, vi, vj )) == NULL )
                  {
                    UserWrite( "Not enough memory in d3matmulBS.\n" );
                    REP_ERR_RETURN(GM_OUT_OF_MEM);
                  }
                  mij = CMATRIX0( con );
                  extra_cons++;
                }
                MVALUE( mij, M_res_comp ) += MVALUE( mik, M1comp ) * MVALUE( mkl, M2comp ) * MVALUE( mlj, M3comp );
              }
            }
          }
        }
    }
  if ( (GetMuteLevel() >= 100) && (extra_cons != 0) )
    UserWriteF( "%d extra connection(s) allocated in d3matmulBS.\n", extra_cons );

  return (NUM_OK);
}


/****************************************************************************/
/*D
   CalculateDefectAndNormBS - calculates the defect of a blockmatrix d := f - K * u

   SYNOPSIS:
    DOUBLE CalculateDefectAndNormBS( const BLOCKVECTOR *bv_row,
            const BV_DESC *bvd_col, const BV_DESC_FORMAT *bvdf, INT d_comp,
                INT f_comp, INT K_comp, INT u_comp );

   PARAMETERS:
   .  bv_row - row-blockvector of the matrix
   .  bvd_col - description of the column-blockvector
   .  bvdf - format to interpret the 'bvd_col'
   .  d_comp - position of the resultant defect in the VECTORs of the blockvector
   .  f_comp - position of the right hand side in the VECTORs of the blockvector
   .  K_comp - position of the matrix in the MATRIXs of the blockvector
   .  u_comp - position of the solution in the VECTORs of the blockvector

   DESCRIPTION:
   This function subtracts scalar matrix times scalar vector
   `d := f - K * u` for all
   VECTORs d, f and u of the blockvectors, given by pointer 'bv_row'
   resp. description 'bvd_col', and MATRIXs K coupling between d(f) and u.

   d_comp == f_comp is allowed; then this function is equivalent to
   'dmatmul_minusBS' and then 'eunormBS'.

   RETURN VALUE:
   INT
   .n    NUM_OK if ok

   SEE ALSO:
   BLOCKVECTOR, blas_routines, dmatmul_minusBS
   D*/
/****************************************************************************/

DOUBLE CalculateDefectAndNormBS( const BLOCKVECTOR *bv_row, const BV_DESC *bvd_col, const BV_DESC_FORMAT *bvdf, INT d_comp, INT f_comp, INT K_comp, INT u_comp )
{
  register VECTOR *v, *end_v;
  register MATRIX *m;
  register DOUBLE sum, result;

  ASSERT( (d_comp >= 0) && (f_comp >= 0) && (K_comp >= 0) && (u_comp >= 0) );

  result = 0.0;
  end_v = BVENDVECTOR( bv_row );
  for ( v = BVFIRSTVECTOR( bv_row ); v != end_v; v = SUCCVC( v ) )
  {
    sum = VVALUE( v, f_comp );
    for ( m = VSTART( v ); m != NULL; m = MNEXT( m ) )
      if ( VMATCH( MDEST(m), bvd_col, bvdf ) )
        sum -= MVALUE( m, K_comp ) * VVALUE( MDEST( m ), u_comp );
    VVALUE( v, d_comp ) = sum;
    result += sum * sum;
  }

  return sqrt( result );
}
#endif /* __BLOCK_VECTOR_DESC__ */

INT l_matflset (GRID *g, INT f)
{
  VECTOR *v;
  MATRIX *m;

  if (f!=0 && f!=1) REP_ERR_RETURN (1);
  for (v=FIRSTVECTOR(g); v!= NULL; v=SUCCVC(v))
    if (VSTART(v) != NULL)
      for (m=MNEXT(VSTART(v)); m!=NULL; m=MNEXT(m))
      {
        SETMUP(m,f);
        SETMDOWN(m,f);
      }

  return (0);
}
