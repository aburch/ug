// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  udm.c	                                                                                                    */
/*																			*/
/* Purpose:   user data manager                                                 */
/*																			*/
/*																			*/
/* Author:	  Christian Wieners                                                                             */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de						        */
/*																			*/
/* History:   December 9, 1996                                                                          */
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

#include "general.h"
#include "debug.h"
#include "gm.h"
#include "ugenv.h"
#include "devices.h"
#include "np.h"

#include "udm.h"

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

#define MAX_NAMES 99

#define READ_DR_VEC_FLAG(p,vt,i)        READ_FLAG((p)->dr.VecReserv[vt][(i)/32],1<<((i)%32))
#define READ_DR_MAT_FLAG(p,vt,i)        READ_FLAG((p)->dr.MatReserv[vt][(i)/32],1<<((i)%32))
#define SET_DR_VEC_FLAG(p,vt,i)         SET_FLAG((p)->dr.VecReserv[vt][(i)/32],1<<((i)%32))
#define SET_DR_MAT_FLAG(p,vt,i)         SET_FLAG((p)->dr.MatReserv[vt][(i)/32],1<<((i)%32))
#define CLEAR_DR_VEC_FLAG(p,vt,i)       CLEAR_FLAG((p)->dr.VecReserv[vt][(i)/32],1<<((i)%32))
#define CLEAR_DR_MAT_FLAG(p,vt,i)       CLEAR_FLAG((p)->dr.MatReserv[vt][(i)/32],1<<((i)%32))

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

static char NoVecNames[MAX_VEC_COMP];
static char NoMatNames[2*MAX_MAT_COMP];
static char TypeName[NVECTYPES][3];

static INT VectorDirID;
static INT MatrixDirID;
static INT VectorVarID;
static INT MatrixVarID;

/* RCS string */
static char RCS_ID("$Header$",UG_RCS_STRING);

/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/


/****************************************************************************/
/*D
   CreateVecDesc - create a vector and fill extra data

   SYNOPSIS:
   VECDATA_DESC *CreateVecDesc (MULTIGRID *theMG, char *name, char *compNames,
   SHORT *NCmpInType);

   PARAMETERS:
   .  theMG - create vector for this multigrid
   .  name - create vector with this name
   .  NCmpInType - 'VECDATA_DESC' specification
   .  compNames - (optional) vector of component names (in the canonic type order)
               one char each (NULL pointer for no names)

   DESCRIPTION:
   This function creates a 'VECDATA_DESC' and fills its components.

   RETURN VALUE:
   VECDATA_DESC *
   .n    pointer to 'VECDATA_DESC' environment item
   .n     NULL if failed
   D*/
/****************************************************************************/

INT ConstructVecOffsets (const SHORT *NCmpInType, SHORT *offset)
{
  INT type;

  offset[0] = 0;
  for (type=1; type<NVECOFFSETS; type++)
    offset[type] = offset[type-1] + NCmpInType[type-1];

  return (NUM_OK);
}

static INT SetScalVecSettings (VECDATA_DESC *vd)
{
  INT tp;

  VD_IS_SCALAR(vd) = FALSE;

  /* check number of components per type */
  for (tp=0; tp<NVECTYPES; tp++)
    if (VD_ISDEF_IN_TYPE(vd,tp))
    {
      if (VD_NCMPS_IN_TYPE(vd,tp)!=1)
        return (NUM_OK);                                                        /* no scalar */
      else
        VD_SCALCMP(vd) = VD_CMP_OF_TYPE(vd,tp,0);
    }

  /* check location of components per type */
  VD_SCALTYPEMASK(vd) = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    if (VD_ISDEF_IN_TYPE(vd,tp))
    {
      VD_SCALTYPEMASK(vd) |= 1<<tp;
      if (VD_SCALCMP(vd)!=VD_CMP_OF_TYPE(vd,tp,0))
        return (NUM_OK);                                                        /* no scalar */
    }

  VD_IS_SCALAR(vd) = TRUE;
  return (NUM_OK);
}

VECDATA_DESC *GetFirstVector (MULTIGRID *theMG)
{
  ENVITEM *item;

  if (ChangeEnvDir("/Multigrids") == NULL) return (NULL);
  if (ChangeEnvDir(ENVITEM_NAME(theMG)) == NULL) return (NULL);
  item = (ENVITEM *)ChangeEnvDir("Vectors");
  if (item == NULL) return (NULL);
  for (item=ENVITEM_DOWN(item); item!=NULL; item=NEXT_ENVITEM(item))
    if (ENVITEM_TYPE(item) == VectorVarID)
      return ((VECDATA_DESC *)item);

  return (NULL);
}

VECDATA_DESC *GetNextVector (VECDATA_DESC *vd)
{
  ENVITEM *item;

  for (item=NEXT_ENVITEM((ENVITEM *)vd); item!=NULL; item=NEXT_ENVITEM(item))
    if (ENVITEM_TYPE(item) == VectorVarID)
      return ((VECDATA_DESC *)item);

  return (NULL);
}

static INT GetNewVectorName (MULTIGRID *theMG, char *name)
{
  VECDATA_DESC *vd;
  char buffer[NAMESIZE];
  INT i;

  for (i=0; i<MAX_NAMES; i++) {
    sprintf(buffer,"vec%02d",i);
    for (vd = GetFirstVector(theMG); vd != NULL; vd = GetNextVector(vd))
      if (strcmp(ENVITEM_NAME(vd),buffer) == 0) break;
    if (vd == NULL) break;
  }
  if (i == MAX_NAMES) return(1);
  strcpy(name,buffer);

  return(0);
}

VECDATA_DESC *CreateVecDesc (MULTIGRID *theMG, const char *name, const char *compNames,
                             const SHORT *NCmpInType)
{
  VECDATA_DESC *vd;
  SHORT offset[NVECOFFSETS],*Comp;
  char buffer[NAMESIZE];
  INT i,j,tp,ncmp,size;

  if (theMG == NULL)
    return (NULL);

  if (ChangeEnvDir("/Multigrids") == NULL) return (NULL);
  if (ChangeEnvDir(ENVITEM_NAME(theMG)) == NULL) return (NULL);
  if (ChangeEnvDir("Vectors") == NULL) {
    MakeEnvItem("Vectors",VectorDirID,sizeof(ENVDIR));
    if (ChangeEnvDir("Vectors") == NULL) return (NULL);
  }
  if (name != NULL)
    strcpy(buffer,name);
  else if (GetNewVectorName(theMG,buffer)) return (NULL);
  ConstructVecOffsets(NCmpInType,offset);
  ncmp = offset[NVECTYPES];
  if (ncmp <= 0) return (NULL);
  size = sizeof(VECDATA_DESC)+(ncmp-1)*sizeof(SHORT);
  vd = (VECDATA_DESC *) MakeEnvItem (buffer,VectorVarID,size);
  if (vd == NULL) return (NULL);
  if (compNames==NULL)
    memcpy(VM_COMP_NAMEPTR(vd),NoVecNames,MAX(ncmp,MAX_VEC_COMP));
  else
    memcpy(VM_COMP_NAMEPTR(vd),compNames,MAX(ncmp,MAX_VEC_COMP));

  /* fill data in vec data desc */
  i = 0;
  Comp = VM_COMPPTR(vd);
  for (tp=0; tp<NVECTYPES; tp++) {
    VD_NCMPS_IN_TYPE(vd,tp) = NCmpInType[tp];
    VD_CMPPTR_OF_TYPE(vd,tp) = Comp + offset[tp];
    for (j=0; j<MAX_NDOF_MOD_32*32; j++) {
      if (i >= offset[tp+1]) break;
      if (j*sizeof(DOUBLE) >= theMG->theFormat->VectorSizes[tp])
        return(NULL);
      if (READ_DR_VEC_FLAG(theMG,tp,j)) continue;
      Comp[i++] = j;
      SET_DR_VEC_FLAG(theMG,tp,j);
    }
  }
  for (tp=0; tp<NVECOFFSETS; tp++)
    VD_OFFSET(vd,tp) = offset[tp];

  for (tp=0; tp<NVECTYPES; tp++) {
    PRINTDEBUG(numerics,1,("offset %d comp ",offset[tp]));
    for (i=0; i<VD_NCMPS_IN_TYPE(vd,tp); i++)
      PRINTDEBUG(numerics,1,(" %d",VD_CMP_OF_TYPE(vd,tp,i)));
  }
  PRINTDEBUG(numerics,1,("\n"));

  /* fill fields with scalar properties */
  SetScalVecSettings(vd);
  VM_LOCKED(vd) = 0;

  return (vd);
}

VECDATA_DESC *CreateSubVecDesc (MULTIGRID *theMG, const VECDATA_DESC *theVD, const char *name,
                                const SHORT *NCmpInType, const SHORT *Comps, const char *CompNames)
{
  VECDATA_DESC *vd;
  SHORT offset[NVECOFFSETS];
  const SHORT *offptr;
  INT j,k,tp,ncmp,size;

  if (theMG == NULL)
    return (NULL);

  if (ChangeEnvDir("/Multigrids") == NULL) return (NULL);
  if (ChangeEnvDir(ENVITEM_NAME(theMG)) == NULL) return (NULL);
  if (ChangeEnvDir("Vectors") == NULL) return (NULL);
  ConstructVecOffsets(NCmpInType,offset);
  ncmp = offset[NVECTYPES];
  offptr = VD_OFFSETPTR(theVD);
  if (ncmp <= 0) return (NULL);
  size = sizeof(VECDATA_DESC)+(ncmp-1)*sizeof(SHORT);
  vd = (VECDATA_DESC *) MakeEnvItem (name,VectorVarID,size);
  if (vd == NULL) return (NULL);

  /* fill data in vec data desc */
  strcpy(VM_COMP_NAMEPTR(vd),CompNames);
  k = 0;
  for (tp=0; tp<NVECTYPES; tp++) {
    VD_NCMPS_IN_TYPE(vd,tp) = NCmpInType[tp];
    VD_CMPPTR_OF_TYPE(vd,tp) = VM_COMPPTR(vd) + offset[tp];
    for (j=0; j<NCmpInType[tp]; j++) {
      VD_CMP_OF_TYPE(vd,tp,j) = Comps[k++];
    }
  }
  ASSERT(k==offset[NVECTYPES]);
  for (tp=0; tp<NVECOFFSETS; tp++)
    VD_OFFSET(vd,tp) = offset[tp];

  /* fill fields with scalar properties */
  SetScalVecSettings(vd);
  VM_LOCKED(vd) = 0;

  return (vd);
}

/****************************************************************************/
/*D
   CreateMatDesc - create a matrix and fill extra data

   SYNOPSIS:
   MATDATA_DESC *CreateMatDesc (MULTIGRID *theMG, char *name, char *compNames,
   SHORT *RowsInType, SHORT *ColsInType);

   PARAMETERS:
   .  theMG - create vector for this multigrid
   .  name - create vector with this name
   .  RowsInType - 'MATDATA_DESC' specification
   .  ColsInType - 'MATDATA_DESC' specification
   .  compNames - (optional) vector of component names (in the canonic type order)
               two char each (NULL pointer for no names)

   DESCRIPTION:
   This function creates a 'MATDATA_DESC' and fills its components.

   RETURN VALUE:
   MATDATA_DESC *
   .n    pointer to 'MATDATA_DESC' environment item
   .n     NULL if failed
   D*/
/****************************************************************************/

INT ConstructMatOffsets (const SHORT *RowsInType, const SHORT *ColsInType, SHORT *offset)
{
  INT type;

  offset[0] = 0;
  for (type=1; type<NMATOFFSETS; type++)
    offset[type] = offset[type-1] + RowsInType[type-1]*ColsInType[type-1];

  return (NUM_OK);
}

static INT SetScalMatSettings (MATDATA_DESC *md)
{
  INT mtp;

  MD_IS_SCALAR(md) = FALSE;

  /* check number of components per type */
  for (mtp=0; mtp<NMATTYPES; mtp++)
    if (MD_ISDEF_IN_MTYPE(md,mtp))
    {
      if ((MD_ROWS_IN_MTYPE(md,mtp)!=1) || (MD_COLS_IN_MTYPE(md,mtp)!=1))
        return (NUM_OK);                                                        /* no scalar */
      else
        MD_SCALCMP(md) = MD_MCMP_OF_MTYPE(md,mtp,0);
    }

  /* check location of components per type */
  MD_SCAL_RTYPEMASK(md) = MD_SCAL_CTYPEMASK(md) = 0;
  for (mtp=0; mtp<NMATTYPES; mtp++)
    if (MD_ISDEF_IN_MTYPE(md,mtp))
    {
      MD_SCAL_RTYPEMASK(md) |= 1<<MTYPE_RT(mtp);
      MD_SCAL_CTYPEMASK(md) |= 1<<MTYPE_CT(mtp);
      if (MD_SCALCMP(md)!=MD_MCMP_OF_MTYPE(md,mtp,0))
        return (NUM_OK);                                                        /* no scalar */
    }

  MD_IS_SCALAR(md) = TRUE;

  return (NUM_OK);
}

MATDATA_DESC *GetFirstMatrix (MULTIGRID *theMG)
{
  ENVITEM *item;

  if (ChangeEnvDir("/Multigrids") == NULL) return (NULL);
  if (ChangeEnvDir(ENVITEM_NAME(theMG)) == NULL) return (NULL);
  item = (ENVITEM *)ChangeEnvDir("Matrices");
  if (item == NULL) return (NULL);
  for (item=ENVITEM_DOWN(item); item!=NULL; item=NEXT_ENVITEM(item))
    if (ENVITEM_TYPE(item) == MatrixVarID)
      return ((MATDATA_DESC *)item);

  return (NULL);
}

MATDATA_DESC *GetNextMatrix (MATDATA_DESC *md)
{
  ENVITEM *item;

  for (item=NEXT_ENVITEM((ENVITEM *)md); item!=NULL; item=NEXT_ENVITEM(item))
    if (ENVITEM_TYPE(item) == MatrixVarID)
      return ((MATDATA_DESC *)item);

  return (NULL);
}

static INT GetNewMatrixName (MULTIGRID *theMG, char *name)
{
  MATDATA_DESC *md;
  char buffer[NAMESIZE];
  INT i;

  for (i=0; i<MAX_NAMES; i++) {
    sprintf(buffer,"mat%02d",i);
    for (md = GetFirstMatrix(theMG); md != NULL; md = GetNextMatrix(md))
      if (strcmp(ENVITEM_NAME(md),buffer) == 0) break;
    if (md == NULL) break;
  }
  if (i == MAX_NAMES) return(1);
  strcpy(name,buffer);

  return(0);
}

MATDATA_DESC *CreateMatDesc (MULTIGRID *theMG, const char *name, const char *compNames,
                             const SHORT *RowsInType, const SHORT *ColsInType)
{
  MATDATA_DESC *md;
  SHORT offset[NMATOFFSETS],*Comp;
  char buffer[NAMESIZE];
  INT i,j,tp,ncmp,size;

  if (theMG == NULL)
    return (NULL);

  if (ChangeEnvDir("/Multigrids") == NULL) return (NULL);
  if (ChangeEnvDir(ENVITEM_NAME(theMG)) == NULL) return (NULL);
  if (ChangeEnvDir("Matrices") == NULL) {
    MakeEnvItem("Matrices",MatrixDirID,sizeof(ENVDIR));
    if (ChangeEnvDir("Matrices") == NULL) return (NULL);
  }
  ConstructMatOffsets(RowsInType,ColsInType,offset);
  ncmp = offset[NMATTYPES];
  if (ncmp <= 0) return (NULL);
  size = sizeof(MATDATA_DESC)+(ncmp-1)*sizeof(SHORT);
  if (name != NULL)
    strcpy(buffer,name);
  else if (GetNewMatrixName(theMG,buffer)) return (NULL);
  md = (MATDATA_DESC *) MakeEnvItem (buffer,MatrixVarID,size);
  if (md == NULL) return (NULL);
  if (compNames==NULL)
    memcpy(VM_COMP_NAMEPTR(md),NoMatNames,MAX(ncmp,MAX_MAT_COMP));
  else
    memcpy(VM_COMP_NAMEPTR(md),compNames,MAX(ncmp,MAX_MAT_COMP));

  /* fill data in vec data desc */
  i = 0;
  Comp = VM_COMPPTR(md);
  for (tp=0; tp<NMATTYPES; tp++) {
    MD_ROWS_IN_MTYPE(md,tp) = RowsInType[tp];
    MD_COLS_IN_MTYPE(md,tp) = ColsInType[tp];
    MD_MCMPPTR_OF_MTYPE(md,tp) = Comp + offset[tp];
    for (j=0; j<MAX_NDOF_MOD_32*32; j++) {
      if (i >= offset[tp+1]) break;
      if (j*sizeof(DOUBLE) >=
          theMG->theFormat->MatrixSizes[MatrixType[MTYPE_RT(tp)][MTYPE_CT(tp)]])
        return(NULL);
      if (READ_DR_MAT_FLAG(theMG,tp,j)) continue;
      Comp[i++] = j;
      SET_DR_MAT_FLAG(theMG,tp,j);
    }
  }
  for (tp=0; tp<NMATOFFSETS; tp++)
    MD_MTYPE_OFFSET(md,tp) = offset[tp];

  /* fill fields with scalar properties */
  SetScalMatSettings(md);
  VM_LOCKED(md) = 0;

  return (md);
}

MATDATA_DESC *CreateSubMatDesc (MULTIGRID *theMG, const MATDATA_DESC *theMD,
                                const char *name, const SHORT *RowsInType,
                                const SHORT *ColsInType, const SHORT *Comps, const char *CompNames)
{
  MATDATA_DESC *md;
  SHORT offset[NMATOFFSETS];
  const SHORT *offptr;
  INT j,tp,ncmp,size;

  if (theMG == NULL)
    return (NULL);

  if (ChangeEnvDir("/Multigrids") == NULL) return (NULL);
  if (ChangeEnvDir(ENVITEM_NAME(theMG)) == NULL) return (NULL);
  if (ChangeEnvDir("Matrices") == NULL) return (NULL);
  ConstructMatOffsets(RowsInType,ColsInType,offset);
  ncmp = offset[NMATTYPES];
  offptr = MD_OFFSETPTR(theMD);
  if (ncmp <= 0) return (NULL);
  size = sizeof(MATDATA_DESC)+(ncmp-1)*sizeof(SHORT);
  md = (MATDATA_DESC *) MakeEnvItem (name,MatrixVarID,size);
  if (md == NULL) return (NULL);

  /* fill data in vec data desc */
  for (tp=0; tp<NMATTYPES; tp++) {
    MD_ROWS_IN_MTYPE(md,tp) = RowsInType[tp];
    MD_COLS_IN_MTYPE(md,tp) = ColsInType[tp];
    MD_MCMPPTR_OF_MTYPE(md,tp) = VM_COMPPTR(md) + offset[tp];
    for (j=0; j<RowsInType[tp]*ColsInType[tp]; j++) {
      VM_COMP_NAME(md,offset[tp]+2*j) =
        VM_COMP_NAME(theMD,offptr[tp]+Comps[tp]+2*j);
      VM_COMP_NAME(md,offset[tp]+2*j+1) =
        VM_COMP_NAME(theMD,offptr[tp]+Comps[tp]+2*j+1);
      MD_MCMP_OF_MTYPE(md,tp,j) =
        MD_MCMP_OF_MTYPE(theMD,tp,Comps[tp]+j);
    }
  }
  for (tp=0; tp<NMATOFFSETS; tp++)
    MD_MTYPE_OFFSET(md,tp) = offset[tp];

  /* fill fields with scalar properties */
  SetScalMatSettings(md);
  VM_LOCKED(md) = 0;

  return (md);
}

/****************************************************************************/
/*
   DisplayVecDataDesc - Display VECDATA_DESC entries

   SYNOPSIS:
   INT DisplayVecDataDesc (const VECDATA_DESC *vd)

   PARAMETERS:
   .  vd - VECDATA_DESC to display

   DESCRIPTION:
   This function displays the entries of a VECDATA_DESC: comp-names, comp-positions etc.

   RETURN VALUE:
   INT
   .n      0: ok
   .n      else: error
 */
/****************************************************************************/

INT DisplayVecDataDesc (const VECDATA_DESC *vd)
{
  const SHORT *offset;
  const char *cn;
  INT rt,i;

  if (vd==NULL) return (1);

  UserWriteF("contents of vector symbol '%s'\n",ENVITEM_NAME(vd));

  cn = VM_COMP_NAMEPTR(vd);
  offset = VD_OFFSETPTR(vd);
  for (rt=0; rt<NVECTYPES; rt++)
    if (VD_ISDEF_IN_TYPE(vd,rt))
    {
      UserWrite("-------\n");
      for (i=0; i<VD_NCMPS_IN_TYPE(vd,rt); i++)
        UserWriteF("%s %c %2d\n",(i) ? "  " : TypeName[rt],cn[offset[rt]+i],VD_CMP_OF_TYPE(vd,rt,i));
    }
  UserWrite("-------\n");

  if (VD_IS_SCALAR(vd))
  {
    UserWrite("\nvecsym is scalar:\n");
    UserWriteF("  comp %2d\n",VD_SCALCMP(vd));
    UserWriteF("  mask %2d\n",VD_SCALTYPEMASK(vd));
  }

  UserWrite("\n");

  return (0);
}

INT DisplayMatDataDesc (const MATDATA_DESC *md)
{
  const SHORT *offset;
  const char *cn;
  INT rt,ct,mtp,i,j,nc,maxr[NVECTYPES],maxc[NVECTYPES];

  if (md==NULL) return (1);

  UserWriteF("contents of matrix symbol '%s'\n",ENVITEM_NAME(md));

  cn = VM_COMP_NAMEPTR(md);
  offset = MD_OFFSETPTR(md);

  for (rt=0; rt<NVECTYPES; rt++)
  {
    maxr[rt] = 0;
    for (ct=0; ct<NVECTYPES; ct++)
      if (MD_ISDEF_IN_RT_CT(md,rt,ct))
        maxr[rt] = MAX(maxr[rt],MD_ROWS_IN_RT_CT(md,rt,ct));
  }

  /* headline for col types */
  UserWrite("  ");
  for (ct=0; ct<NVECTYPES; ct++)
  {
    maxc[ct] = 0;
    for (rt=0; rt<NVECTYPES; rt++)
      if (MD_ISDEF_IN_RT_CT(md,rt,ct))
        maxc[ct] = MAX(maxc[ct],MD_COLS_IN_RT_CT(md,rt,ct));
    for (j=0; j<maxc[ct]; j++)
      UserWriteF(" %s%s",(j) ? "" : "|",(j) ? "  " : TypeName[ct]);
  }
  UserWrite("\n--");
  for (ct=0; ct<NVECTYPES; ct++)
    for (j=0; j<maxc[ct]; j++)
      UserWriteF("-%s--",(j) ? "" : "-");

  for (rt=0; rt<NVECTYPES; rt++)
  {
    for (i=0; i<maxr[rt]; i++)
    {
      /* compname line */
      UserWriteF("\n%s",(i) ? "  " : TypeName[rt]);
      if (cn[0]!=' ')
      {
        for (ct=0; ct<NVECTYPES; ct++)
        {
          j = 0;
          if (MD_ISDEF_IN_RT_CT(md,rt,ct))
          {
            mtp = MTP(rt,ct);
            nc  = MD_COLS_IN_MTYPE(md,mtp);
            for (; j<nc; j++)
              UserWriteF(" %s%c%c",(j) ? "" : "|",cn[2*(offset[mtp]+i*nc+j)],cn[2*(offset[mtp]+i*nc+j)+1]);
          }
          for (; j<maxc[ct]; j++)
            UserWriteF(" %s  ",(j) ? "" : "|");
        }
        UserWrite("\n  ");
      }
      /* comp position line */
      for (ct=0; ct<NVECTYPES; ct++)
      {
        j = 0;
        if (MD_ISDEF_IN_RT_CT(md,rt,ct))
        {
          mtp = MTP(rt,ct);
          for (; j<MD_COLS_IN_MTYPE(md,mtp); j++)
            UserWriteF(" %s%2d",(j) ? "" : "|",MD_IJ_CMP_OF_MTYPE(md,mtp,i,j));
        }
        for (; j<maxc[ct]; j++)
          UserWriteF(" %s  ",(j) ? "" : "|");
      }
    }
    if (maxr[rt]>0)
    {
      /* type seperator line */
      UserWrite("\n--");
      for (ct=0; ct<NVECTYPES; ct++)
        for (j=0; j<maxc[ct]; j++)
          UserWriteF("-%s--",(j) ? "" : "-");
    }
  }
  UserWrite("\n");

  if (MD_IS_SCALAR(md))
  {
    UserWrite("\nmatsym is scalar:\n");
    UserWriteF("  comp %2d\n",MD_SCALCMP(md));
    UserWriteF("  rmsk %2d\n",MD_SCAL_RTYPEMASK(md));
    UserWriteF("  cmsk %2d\n",MD_SCAL_CTYPEMASK(md));
  }

  UserWrite("\n");

  return (0);
}

/****************************************************************************/
/*D
   GetVecDataDescByName - find vector data desciptor

   SYNOPSIS:
   VECDATA_DESC *GetVecDataDescByName (MULTIGRID *theMG, char *name);

   PARAMETERS:
   .  theMG - create vector for this multigrid
   .  name - name of a vector

   DESCRIPTION:
   This function finds a vector by name.

   RETURN VALUE:
   VECDATA_DESC *
   .n      pointer to the vector
   .n      NULL if there is no vector of this name in the multigrid
   D*/
/****************************************************************************/

VECDATA_DESC *GetVecDataDescByName (const MULTIGRID *theMG, char *name)
{
  if (ChangeEnvDir("/Multigrids") == NULL) return (NULL);
  if (ChangeEnvDir(ENVITEM_NAME(theMG)) == NULL) return (NULL);
  return((VECDATA_DESC *) SearchEnv(name,"Vectors",
                                    VectorVarID,VectorDirID));
}

/****************************************************************************/
/*D
   GetMatDataDescByName - find matrix data descriptor

   SYNOPSIS:
   MATDATA_DESC *GetMatDataDescByName (MULTIGRID *theMG, char *name);

   PARAMETERS:
   .  theMG - create vector for this multigrid
   .  name - name of a matrix

   DESCRIPTION:
   This function finds a matrix by name.

   RETURN VALUE:
   MATDATA_DESC *
   .n      pointer to the matrix
   .n      NULL if there is no matrix of this name in the multigrid
   D*/
/****************************************************************************/

MATDATA_DESC *GetMatDataDescByName (const MULTIGRID *theMG, char *name)
{
  if (ChangeEnvDir("/Multigrids") == NULL) return (NULL);
  if (ChangeEnvDir(ENVITEM_NAME(theMG)) == NULL) return (NULL);
  return((MATDATA_DESC *) SearchEnv(name,"Matrices",
                                    MatrixVarID,MatrixDirID));
}

/****************************************************************************/
/*D
   AllocVDfromVD - dynamic vector allocation

   SYNOPSIS:
   INT AllocVDfromVD (MULTIGRID *theMG, INT fl, INT tl,
   VECDATA_DESC *template_desc, VECDATA_DESC **new_desc);

   PARAMETERS:
   .  theMG - create vector for this multigrid
   .  fl - from level
   .  tl - to level
   .  template_desc - template vector
   .  new_desc - new vector

   DESCRIPTION:
   This function allocates a new vector.

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if error occurred
 */
/****************************************************************************/

static INT CompVecDesc (VECDATA_DESC *vd, SHORT *NCmpsInType)
{
  INT tp;

  for (tp=0; tp<NVECTYPES; tp++)
    if (VD_NCMPS_IN_TYPE(vd,tp) != NCmpsInType[tp])
      return(1);

  return(0);
}

static INT AllocVecDesc (MULTIGRID *theMG, INT fl, INT tl, VECDATA_DESC *vd)
{
  GRID *theGrid;
  INT i,j,tp;

  if (vd == NULL) return(1);
  for (i=fl; i<=tl; i++) {
    theGrid = GRID_ON_LEVEL(theMG,i);
    for (tp=0; tp<NVECTYPES; tp++)
      for (j=0; j<VD_NCMPS_IN_TYPE(vd,tp); j++)
        if (READ_DR_VEC_FLAG(theGrid,tp,VD_CMP_OF_TYPE(vd,tp,j)))
          return(1);
  }
  for (i=fl; i<=tl; i++) {
    theGrid = GRID_ON_LEVEL(theMG,i);
    for (tp=0; tp<NVECTYPES; tp++)
      for (j=0; j<VD_NCMPS_IN_TYPE(vd,tp); j++)
        SET_DR_VEC_FLAG(theGrid,tp,VD_CMP_OF_TYPE(vd,tp,j));
  }

  return(0);
}

INT AllocVDFromVD (MULTIGRID *theMG, INT fl, INT tl,
                   VECDATA_DESC *template_desc, VECDATA_DESC **new_desc)
{
  VECDATA_DESC *vd;

  if (*new_desc != NULL)
    if (VM_LOCKED(*new_desc))
      return(0);
  if (AllocVecDesc(theMG,fl,tl,*new_desc)) {
    for (vd = GetFirstVector(theMG); vd != NULL; vd = GetNextVector(vd)) {
      if (VM_LOCKED(vd)) continue;
      if (CompVecDesc(vd,template_desc->NCmpInType)) continue;
      if (!AllocVecDesc(theMG,fl,tl,vd)) {
        *new_desc = vd;
        return(0);
      }
    }
    *new_desc = CreateVecDesc(theMG,NULL,template_desc->compNames,
                              template_desc->NCmpInType);
    if (*new_desc == NULL) {
      UserWrite("AllocVDFromVD: cannot create VecDesc\n");
      return(1);
    }
    if (AllocVecDesc(theMG,fl,tl,*new_desc)) {
      UserWrite("AllocVDFromVD: cannot allocate VecDesc\n");
      return(1);
    }
  }

  return(0);
}

/****************************************************************************/
/*D
   FreeVD - dynamic vector deallocation

   SYNOPSIS:
   INT FreeVD (MULTIGRID *theMG, INT fl, INT tl, VECDATA_DESC *vd);

   PARAMETERS:
   .  theMG - create vector for this multigrid
   .  fl - from level
   .  tl - to level
   .  vd - vector descriptor

   DESCRIPTION:
   This function deallocates a vector.

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if error occurred
 */
/****************************************************************************/

INT FreeVD (MULTIGRID *theMG, INT fl, INT tl, VECDATA_DESC *vd)
{
  GRID *theGrid;
  INT i,j,tp;

  if (VM_LOCKED(vd)) return(0);
  PRINTDEBUG(numerics,1,(" FreeVD %s from %d to %d\n",
                         ENVITEM_NAME(vd),fl,tl));
  for (i=fl; i<=tl; i++) {
    theGrid = GRID_ON_LEVEL(theMG,i);
    for (tp=0; tp<NVECTYPES; tp++)
      for (j=0; j<VD_NCMPS_IN_TYPE(vd,tp); j++)
        CLEAR_DR_VEC_FLAG(theGrid,tp,VD_CMP_OF_TYPE(vd,tp,j));
  }

  return(0);
}

/****************************************************************************/
/*D
   AllocMDFromVD - dynamic matrix allocation

   SYNOPSIS:
   INT AllocMDFromVD (MULTIGRID *theMG, INT fl, INT tl,
   VECDATA_DESC *x, VECDATA_DESC *y, MATDATA_DESC **new_desc);

   PARAMETERS:
   .  theMG - create vector for this multigrid
   .  fl - from level
   .  tl - to level
   .  x - template vector for row components
   .  y - template vector for column components
   .  new_desc - new matrix

   DESCRIPTION:
   This function allocates a new matrix.

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if error occurred
 */
/****************************************************************************/

static INT CompMatDesc (MATDATA_DESC *md,
                        SHORT *RowsInType, SHORT *ColsInType)
{
  INT tp;

  for (tp=0; tp<NMATTYPES; tp++) {
    if (MD_COLS_IN_MTYPE(md,tp) != ColsInType[tp])
      return(1);
    if (MD_ROWS_IN_MTYPE(md,tp) != RowsInType[tp])
      return(1);
  }

  return(0);
}

static INT AllocMatDesc (MULTIGRID *theMG, INT fl, INT tl, MATDATA_DESC *md)
{
  GRID *theGrid;
  INT i,j,tp;

  if (md == NULL) return(1);
  for (i=fl; i<=tl; i++) {
    theGrid = GRID_ON_LEVEL(theMG,i);
    for (tp=0; tp<NMATTYPES; tp++)
      for (j=0; j<MD_NCMPS_IN_MTYPE(md,tp); j++)
        if (READ_DR_MAT_FLAG(theGrid,tp,MD_MCMP_OF_MTYPE(md,tp,j)))
          return(1);
  }
  for (i=fl; i<=tl; i++) {
    theGrid = GRID_ON_LEVEL(theMG,i);
    for (tp=0; tp<NMATTYPES; tp++)
      for (j=0; j<MD_NCMPS_IN_MTYPE(md,tp); j++)
        SET_DR_MAT_FLAG(theGrid,tp,MD_MCMP_OF_MTYPE(md,tp,j));
  }

  return(0);
}

INT AllocMDFromVD (MULTIGRID *theMG, INT fl, INT tl,
                   VECDATA_DESC *x, VECDATA_DESC *y, MATDATA_DESC **new_desc)
{
  MATDATA_DESC *md;
  INT i,j,tp;
  SHORT RowsInType[NMATTYPES];
  SHORT ColsInType[NMATTYPES];

  if (*new_desc != NULL)
    if (VM_LOCKED(*new_desc))
      return(0);
  if (AllocMatDesc(theMG,fl,tl,*new_desc)) {
    for (i=0; i<NVECTYPES; i++)
      for (j=0; j<NVECTYPES; j++) {
        tp = MTP(i,j);
        if (VD_NCMPS_IN_TYPE(x,i)*VD_NCMPS_IN_TYPE(y,j) > 0) {
          RowsInType[tp] = VD_NCMPS_IN_TYPE(x,i);
          ColsInType[tp] = VD_NCMPS_IN_TYPE(y,j);
        }
        else
          RowsInType[tp] = ColsInType[tp] = 0;
      }
    for (md = GetFirstMatrix(theMG); md != NULL; md = GetNextMatrix(md)) {
      if (VM_LOCKED(md)) continue;
      if (CompMatDesc(md,RowsInType,ColsInType))
        continue;
      if (!AllocMatDesc(theMG,fl,tl,md)) {
        *new_desc = md;
        return(0);
      }
    }
    *new_desc = CreateMatDesc(theMG,NULL,NULL,RowsInType,ColsInType);
    if (*new_desc == NULL) {
      UserWrite("AllocMDFromVD: cannot create MatDesc\n");
      return(1);
    }
    if (AllocMatDesc(theMG,fl,tl,*new_desc)) {
      UserWrite("AllocMDFromVD: cannot allocate MatDesc\n");
      return(1);
    }
  }

  return(0);
}

/****************************************************************************/
/*D
   AllocMDFromMD - dynamic matrix allocation

   SYNOPSIS:
   INT AllocMDfromMD (MULTIGRID *theMG, INT fl, INT tl,
   MATDATA_DESC *template_desc, MATDATA_DESC **new_desc)

   PARAMETERS:
   .  theMG - create vector for this multigrid
   .  fl - from level
   .  tl - to level
   .  template_desc - template matrix
   .  new_desc - new matrix

   DESCRIPTION:
   This function allocates a new matrix.

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if error occurred
 */
/****************************************************************************/

INT AllocMDFromMD (MULTIGRID *theMG, INT fl, INT tl,
                   MATDATA_DESC *template_desc, MATDATA_DESC **new_desc)
{
  MATDATA_DESC *md;

  if (*new_desc != NULL)
    if (VM_LOCKED(*new_desc))
      return(0);
  if (AllocMatDesc(theMG,fl,tl,*new_desc)) {
    for (md = GetFirstMatrix(theMG); md != NULL; md = GetNextMatrix(md)) {
      if (VM_LOCKED(md)) continue;
      if (CompMatDesc(md,template_desc->RowsInType,
                      template_desc->ColsInType))
        continue;
      if (!AllocMatDesc(theMG,fl,tl,md)) {
        *new_desc = md;
        return(0);
      }
    }
    *new_desc = CreateMatDesc(theMG,NULL,template_desc->compNames,
                              template_desc->RowsInType,
                              template_desc->ColsInType);
    if (*new_desc == NULL) {
      UserWrite("AllocMDFromMD: cannot create MatDesc\n");
      return(1);
    }
    if (AllocMatDesc(theMG,fl,tl,*new_desc)) {
      UserWrite("AllocMDFromMD: cannot allocate MatDesc\n");
      return(1);
    }
  }

  return(0);
}

/****************************************************************************/
/*D
   FreeMD - dynamic vector deallocation

   SYNOPSIS:
   INT FreeMD (MULTIGRID *theMG, INT fl, INT tl, MATDATA_DESC *md);

   PARAMETERS:
   .  theMG - create vector for this multigrid
   .  fl - from level
   .  tl - to level
   .  md - matrix descriptor

   DESCRIPTION:
   This function deallocates a matrix.

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if error occurred
 */
/****************************************************************************/

INT FreeMD (MULTIGRID *theMG, INT fl, INT tl, MATDATA_DESC *md)
{
  GRID *theGrid;
  INT i,j,tp;

  if (VM_LOCKED(md)) return(0);
  for (i=fl; i<=tl; i++) {
    theGrid = GRID_ON_LEVEL(theMG,i);
    for (tp=0; tp<NMATTYPES; tp++)
      for (j=0; j<MD_NCMPS_IN_MTYPE(md,tp); j++)
        CLEAR_DR_MAT_FLAG(theGrid,tp,MD_MCMP_OF_MTYPE(md,tp,j));
  }

  return(0);
}

/****************************************************************************/
/*
   InitUserDataManager - Init this file

   SYNOPSIS:
   INT InitUserDataManager (void);

   PARAMETERS:
   .  void -

   DESCRIPTION:
   This function inits this file.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    __LINE__ if error occured.
 */
/****************************************************************************/

INT InitUserDataManager ()
{
  char *names;
  INT i;

  MatrixDirID = GetNewEnvDirID();
  VectorDirID = GetNewEnvDirID();
  MatrixVarID = GetNewEnvVarID();
  VectorVarID = GetNewEnvVarID();

  names = DEFAULT_NAMES;

  for (i=0; i<MAX(MAX_VEC_COMP,strlen(DEFAULT_NAMES)); i++)
    NoVecNames[i] = names[i];
  for (i=0; i<2*MAX_MAT_COMP; i++)
    NoMatNames[i] = ' ';

  strcpy(TypeName[NODEVECTOR],"nd");
  strcpy(TypeName[EDGEVECTOR],"ed");
  strcpy(TypeName[ELEMVECTOR],"el");
        #ifdef __THREEDIM__
  strcpy(TypeName[SIDEVECTOR],"si");
        #endif

  return (0);
}
