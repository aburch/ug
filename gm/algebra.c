// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  algebra.c                                                                                                     */
/*																			*/
/* Purpose:   management for algebraic structures							*/
/*																			*/
/* Author:	  Klaus Johannsen												*/
/*			  Interdisziplinaeres Zentrum fuer Wissenschaftliches Rechnen	*/
/*			  Universitaet Heidelberg										*/
/*			  Im Neuenheimer Feld 294										*/
/*			  6900 Heidelberg												*/
/*			  email: ug@ica3.uni-stuttgart.de                                               */
/*																			*/
/*			  blockvector data structure:									*/
/*			  Christian Wrobel                                                                              */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de						        */
/*																			*/
/* History:    1.12.93 begin, ug 3d                                                                             */
/*			  26.10.94 begin combination 2D/3D version						*/
/*			  28.09.95 blockvector implemented (Christian Wrobel)			*/
/*																			*/
/* Remarks:                                                                                                                             */
/*																			*/
/****************************************************************************/


/****************************************************************************/
/*																			*/
/*		defines to exclude functions										*/
/*																			*/
/****************************************************************************/


/****************************************************************************/
/*																			*/
/* include files															*/
/*			  system include files											*/
/*			  application include files                                                                     */
/*																			*/
/****************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "heaps.h"

#include "devices.h"

#include "switch.h"
#include "gm.h"
#include "ugm.h"
#include "evm.h"
#include "misc.h"
#include "simplex.h"
#include "algebra.h"

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

/* for LexAlgDep */
#define ORDERRES                1e-3    /* resolution for LexAlgDep					*/

/* for LexOrderVectorsInGrid */
#define MATTABLESIZE    32

/* constants for the direction of domain halfening */
#define BV_VERTICAL     0
#define BV_HORIZONTAL   1

/* for ordering of matrices */
#define MATHPOS                 +1
#define MATHNEG                 -1

/****************************************************************************/
/*																			*/
/* data structures used in this source file (exported data structures are	*/
/*		  in the corresponding include file!)								*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/

INT MatrixType[MAXVECTORS][MAXVECTORS];

/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/

static INT theAlgDepDirID;                      /* env type for Alg Dep dir                     */
static INT theAlgDepVarID;                      /* env type for ALG_DEP vars                    */

static INT theFindCutDirID;                     /* env type for FindCut dir                     */
static INT theFindCutVarID;                     /* env type for FIND_CUT vars                   */

static FindCutProcPtr FindCutSet;       /* pointer to find cut procedure		*/

/****************************************************************************/
/*
 */
/* definition of exported global variables									*/
/*																			*/
/****************************************************************************/

/* 2 often used blockvector description formats */
const BV_DESC_FORMAT DH_bvdf =
{ 2, 16,
  { (BVD_ENTRY_TYPE)0x03,        (BVD_ENTRY_TYPE)0x0f,
            (BVD_ENTRY_TYPE)0x03f,       (BVD_ENTRY_TYPE)0x0ff,
            (BVD_ENTRY_TYPE)0x03ff,      (BVD_ENTRY_TYPE)0x0fff,
            (BVD_ENTRY_TYPE)0x03fff,     (BVD_ENTRY_TYPE)0x0ffff,
            (BVD_ENTRY_TYPE)0x03ffff,    (BVD_ENTRY_TYPE)0x0fffff,
            (BVD_ENTRY_TYPE)0x03fffff,   (BVD_ENTRY_TYPE)0x0ffffff,
            (BVD_ENTRY_TYPE)0x03ffffff,  (BVD_ENTRY_TYPE)0x0fffffff,
            (BVD_ENTRY_TYPE)0x3fffffff,  (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff,  (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff,  (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff,  (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff,  (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff,  (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff,  (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff,  (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff,  (BVD_ENTRY_TYPE)0xffffffff},
  { (BVD_ENTRY_TYPE)0xfffffffc, (BVD_ENTRY_TYPE)0xfffffff3,
            (BVD_ENTRY_TYPE)0xffffffcf, (BVD_ENTRY_TYPE)0xffffff3f,
            (BVD_ENTRY_TYPE)0xfffffcff, (BVD_ENTRY_TYPE)0xfffff3ff,
            (BVD_ENTRY_TYPE)0xffffcfff, (BVD_ENTRY_TYPE)0xffff3fff,
            (BVD_ENTRY_TYPE)0xfffcffff, (BVD_ENTRY_TYPE)0xfff3ffff,
            (BVD_ENTRY_TYPE)0xffcfffff, (BVD_ENTRY_TYPE)0xff3fffff,
            (BVD_ENTRY_TYPE)0xfcffffff, (BVD_ENTRY_TYPE)0xf3ffffff,
            (BVD_ENTRY_TYPE)0xcfffffff, (BVD_ENTRY_TYPE)0x3fffffff,
            (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff}};

const BV_DESC_FORMAT one_level_bvdf =
{ 32, 1,
  {     (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/

static INT ConnectionType [2][MAXVECTORS][MAXVECTORS];

/* for LexOrderVectorsInGrid */
static const INT *Order,*Sign;
static INT SkipV;
static DOUBLE InvMeshSize;

/* data for CVS */
static char rcsid[] = "$Header$";

/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/

INT BlockHalfening( GRID *grid, BLOCKVECTOR *bv, INT left, INT bottom, INT width, INT height, INT side, INT orientation );

#ifdef __version3__

/****************************************************************************/
/*D
   CheckMatrixList - Check matrix list

   SYNOPSIS:
   static void CheckMatrixList (VECTOR *theVector);

   PARAMETERS:
   .  theVector - pointer to a vector list

   DESCRIPTION:
   This function checks matrix list and prints an error message
   if the list is inconsistent.

   RETURN VALUE:
   void
   D*/
/****************************************************************************/

static void CheckMatrixList (VECTOR *theVector)
{
  MATRIX *theMatrix;
  char buffer[256];

  for (theMatrix=VSTART(theVector); theMatrix!= NULL; theMatrix=MNEXT(theMatrix))
  {
    if (MDIAG(theMatrix))
    {
      if (theVector != MDEST(theMatrix))
      {
        sprintf (buffer,"matrix ??? should be diagonal matrix but dest. ptr does not point back\n");
        UserWrite(buffer);
      }
      if (VSTART(theVector) != theMatrix)
      {
        sprintf (buffer,"matrix ??? is diagonal matrix but is not at first place in the list\n");
        UserWrite(buffer);
      }
    }
    else
    if (MDEST(MADJ(theMatrix)) != theVector)
    {
      sprintf (buffer,"adj of matrix ??? does not point back\n");
      UserWrite(buffer);
    }
  }
}

/****************************************************************************/
/*D
   InitBVDF - Initialize a blockvector description format

   SYNOPSIS:
   INT InitBVDF( BV_DESC_FORMAT *bvdf, BLOCKNUMBER max_blocks )

   PARAMETERS:
   .  bvdf - blockvector description format to be initialized
   .  max_blocks - maximum number of blocks provided for each blockvector level

   DESCRIPTION:
   All routines working with a 'blockvector description' need a explanation
   of the internal encoding used in the description. This routine generates
   this necessary explanation. Since the encoding depends on the number
   of blockvectors you want to use on each blockvector level, you have to
   specify this quantity. The background is, that a sequence of blockvector
   numbers is encoded in a bitstring and the number of bits used for each
   level has to be fixed before the first use. In order to get fast access
   to the stored information, a couple of bitmasks are necessary and are
   precalculated and stored in the 'blockvector description format' by this
   function. The memory to store the information is provided by the caller.

   HINTS:
   Since the hole amount of bits available for encoding a sequence of
   blockvector numbers is constant, you may use only few blocks on each
   level and for that have many levels available; or if you have many
   blocks on each level you can work only with few levels at all. Thus you
   have to find a trade off of your requirements.

   Since the relevant information of the 'max_blocks' parameter is only
   the number of bits, necessary to encode this number, all numbers up to
   the next power of 2 will have the same result.

   PREDEFINED FORMATS:
   There are predefined 'blockvector description formats', which are
   often used. This formats you can use without initialization.
   .  DH_bvdf  - useful for domain halfening methods (up to 4 blocks per level)
   .  one_level_bvdf - max. number of blocks in a level but only 1 level

   EXAMPLE:
   To create a format for managing a octree-like structure (that is
   subdividing a 3D cube into 8 subcubes) you can use the following
   created format:
   .vb
   BV_DESC_FORMAT my_bvdf;

   InitBVDF( &my_bvdf, 8 );
   .ve

   RETURN VALUE:
   INT
   .n GM_OK if ok
   .n GM_OUT_OF_RANGE if 'max_blocks' < 2 or if 'max_blocks' greater than possible

   SEE ALSO:
   BLOCKVECTOR, BV_DESC, BV_DESC_FORMAT

   D*/
/****************************************************************************/

INT InitBVDF( BV_DESC_FORMAT *bvdf, BLOCKNUMBER max_blocks )
{
  INT bits;
  INT level;
  BVD_ENTRY_TYPE mask;

  /* calculate number of bits to represent the numbers 0..max_blocks-1,
     i.e. the next integer greater or equal to the logarithmus dualis
     of max_blocks-1 */
  if ( max_blocks < (BLOCKNUMBER)2 )
    return GM_OUT_OF_RANGE;
  max_blocks--;
  bits = 0;
  while( max_blocks > (BLOCKNUMBER)0 )
  {
    max_blocks >>= 1;
    bits++;
  }


  bvdf->bits = bits;
  bvdf->max_level = BVD_MAX_ENTRIES / bits;
  if ( bvdf->max_level == 0 )
    return GM_OUT_OF_RANGE;

  /* calculate the necessary bitmasks */
  mask = ((BVD_ENTRY_TYPE)1 << bits) - (BVD_ENTRY_TYPE)1;
  bvdf->level_mask[0] = mask;
  bvdf->neg_digit_mask[0] = ~mask;
  for ( level = 1; level < BVD_MAX_ENTRIES; level ++ )
  {
    mask <<= bits;
    bvdf->level_mask[level] = bvdf->level_mask[level-1] | mask;
    bvdf->neg_digit_mask[level] = ~mask;
  }

  return GM_OK;
}

/****************************************************************************/
/*D
   PushEntry - Adds a further blockvector number to the existing sequence

   SYNOPSIS:
   INT PushEntry( BV_DESC *bvd, BLOCKNUMBER bnr, const BV_DESC_FORMAT *bvdf )

   PARAMETERS:
   .  bvd - blockvector description to which a further blocknumber is added
   .  bnr - blocknumber to be added
   .  bvdf - blockvector description format to work on the 'bvd'

   DESCRIPTION:
   In a blockvector description the position of a blockvector in a blockvector
   hierarchy is stored as a sequence of blocknumbers. With this function you
   can add a further number to the existing sequence, i.e. to describe
   a subblock of the smallest already existing block in the 'bvd'.

   This function is embedded in the macro 'BVD_PUSH_ENTRY' and provides
   together with the macro 'BVD_DISCARD_LAST_ENTRY' a stack-mechanism
   for storing a sequence of blockvector numbers.

   RETURN VALUE:
   INT
   .n GM_OK if ok
   .n GM_OUT_OF_RANGE if the maximum number of blockvector levels is exeeded

   SEE ALSO:
   BLOCKVECTOR, BV_DESC

   D*/
/****************************************************************************/

INT PushEntry( BV_DESC *bvd, BLOCKNUMBER bnr, const BV_DESC_FORMAT *bvdf )
{
  /* exist, if there is no space for a further number */
  if ( bvd->current >= bvdf->max_level )
    return GM_OUT_OF_RANGE;

  /* store the number */
  BVD_SET_ENTRY( bvd, bvd->current, bnr, bvdf );
  bvd->current++;

  return GM_OK;
}

/****************************************************************************/
/*D
   FindBV - Find the described blockvector in the grid

   SYNOPSIS:
   BLOCKVECTOR *FindBV( const GRID *grid, BV_DESC *bvd, const BV_DESC_FORMAT *bvdf )

   PARAMETERS:
   .  grid - the grid, which in the blockvector is to be searched
   .  bvd - blockvector description specifying the blockvector
   .  bvdf - blockvector description format to read the 'bvd'

   DESCRIPTION:
   Returns the blockvector specified by 'bvd'.

   RETURN VALUE:
   BLOCKVECTOR *
   .n pointer to the specified blockvector
   .n NULL if the specified blockvector does not exist

   SEE ALSO:
   BLOCKVECTOR, BV_DESC, BVD_PUSH_ENTRY

   D*/
/****************************************************************************/

BLOCKVECTOR *FindBV( const GRID *grid, BV_DESC *bvd, const BV_DESC_FORMAT *bvdf )
{
  register BLOCKVECTOR *bv;
  register BLOCKNUMBER nr;

  /* initialize the search with the topmost blockvector in the grid */
  bv = GFIRSTBV( grid );
  BVD_INIT_SEQ_READ( bvd );
  nr = BVD_READ_NEXT_ENTRY( bvd, bvdf );

  /* search trough all blockvector levels defined in bvd */
  while ( TRUE )
  {
    while ( BVNUMBER(bv) != nr )             /* search block with number nr in block list*/
    {
      bv = BVSUCC( bv );
      if( bv == NULL )
        return bv;                              /* desired number not found, return NULL */
    }

    /* block with number nr found, prepare search on next block level */
    nr = BVD_READ_NEXT_ENTRY( bvd, bvdf  );
    if ( nr != NO_BLOCKVECTOR )
      /* there is a further blockvector level specified */
      if ( BV_IS_LEAF_BV( bv ) )
        return NULL;                            /* more block level specified than present */
      else
        bv = BVDOWNBV( bv );                            /* go to next blockvector level */
    else
      /* no more block level specified, current block is the result */
      return bv;
  }
}

/****************************************************************************/
/*D
   GetFreeVector - Get an object from free list if possible

   SYNOPSIS:
   static VECTOR *GetFreeVector (MULTIGRID *theMG, INT VectorType);

   PARAMETERS:
   .  theMG - multigrid structure to extend
   .  VectorType - NODEVECTOR, ENGEVECTOR, SIDEVECTOR or ELEMENTVECTOR

   DESCRIPTION:
   This function gets an object from free list if possible.

   RETURN VALUE:
   VECTOR *
   .n        pointer to a free vector
   .n        NULL if no object available.

   D*/
/****************************************************************************/

static VECTOR *GetFreeVector (MULTIGRID *theMG, INT VectorType)
{
  void **ptr;

  if ((VectorType<0)||(VectorType>=MAXVECTORS)) return(NULL);
  if (theMG->freeVectors[VectorType]==NULL) return(NULL);
  ptr = (void **) theMG->freeVectors[VectorType];
  theMG->freeVectors[VectorType] = ptr[0];
  return((VECTOR *)ptr);
}

/****************************************************************************/
/*D
   GetFreeConnection - Get an object from free list if possible

   SYNOPSIS:
   static CONNECTION *GetFreeConnection (MULTIGRID *theMG, INT Diag, INT RootType,
   INT DestType);

   PARAMETERS:
   .  theMG - multigrid structure to extend
   .  Diag - flag for diagonal entry
   .  RootType - type of source vector
   .  DestType - type of destination vector

   DESCRIPTION:
   This function gets an object from free list of free connections if possible.
   Diag should 1 if the connection connects a 'VECTOR' with itself ('RootType'
   and 'DestType' should be the same then) and 0 else.

   RETURN VALUE:
   CONNECTION *
   .n          pointer to object
   .n          NULL if no object available.

   D*/
/****************************************************************************/

static CONNECTION *GetFreeConnection (MULTIGRID *theMG, INT Diag, INT RootType, INT DestType)
{
  void **ptr;
  INT ConType;

  ConType = ConnectionType[Diag][RootType][DestType];
  if (ConType == -1) return(NULL);
  if (theMG->freeConnections[ConType]==NULL) return(NULL);
  ptr = (void **) theMG->freeConnections[ConType];
  theMG->freeConnections[ConType] = ptr[0];
  return((CONNECTION *)ptr);
}

/****************************************************************************/
/*D
   GetFreeIMatrix - Get an object from free list if possible

   SYNOPSIS:
   static MATRIX *GetFreeIMatrix (MULTIGRID *theMG, INT RootType, INT DestType);

   PARAMETERS:
   .  theMG - multigrid structure to extend
   .  RootType - type of source vector
   .  DestType - type of destination vector

   DESCRIPTION:
   This function gets an object from free list of free connections if possible.
   RETURN VALUE:
   CONNECTION *
   .n          pointer to object
   .n          NULL if no object available.

   D*/
/****************************************************************************/

#ifdef __INTERPOLATION_MATRIX__
static MATRIX *GetFreeIMatrix (MULTIGRID *theMG, INT RootType, INT DestType)
{
  void **ptr;

  if (theMG->freeIMatrices[RootType][DestType]==NULL)
    return(NULL);
  ptr = (void **) theMG->freeIMatrices[RootType][DestType];
  theMG->freeIMatrices[RootType][DestType] = ptr[0];
  return((MATRIX *)ptr);
}
#endif

/****************************************************************************/
/*D
   PutFreeVector - Put an object in the free list

   SYNOPSIS:
   static INT PutFreeVector (MULTIGRID *theMG, VECTOR *object);

   PARAMETERS:
   .  theMG - mg structure to extend
   .  object - pointer to 'VECTOR' to be freed.

   DESCRIPTION:
   This function puts an object in the free list.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    INT>0 if no valid object number.
   D*/
/****************************************************************************/

static INT PutFreeVector (MULTIGRID *theMG, VECTOR *object)
{
  void **ptr;
  INT VectorType;

  VectorType = VTYPE(object);
  if ((VectorType<0)||(VectorType>=MAXVECTORS))
  {
    UserWrite("wrong object given to PutFreeVector\n");
    return(1);
  }

  ptr = (void **) object;
  ptr[0] = theMG->freeVectors[VectorType];
  theMG->freeVectors[VectorType] = (void *)object;
  return(0);
}

/****************************************************************************/
/*D
   PutFreeConnection -  Put an object in the free list

   SYNOPSIS:
   static INT PutFreeConnection (MULTIGRID *theMG, CONNECTION *object);


   PARAMETERS:
   .  theMG - mg structure to extend
   .  object - Pointer to 'CONNECTION' to be freed.

   DESCRIPTION:
   This function puts an object in the free list.

   RETURN VALUE:
   INT
   .n     0 if ok
   .n     INT>0 if no valid object number.
   D*/
/****************************************************************************/

static INT PutFreeConnection (MULTIGRID *theMG, CONNECTION *object)
{
  void **ptr;
  INT ConType;
  MATRIX *Matrix;

  Matrix = (MATRIX*)object;
  ConType = ConnectionType[MDIAG(Matrix)][MROOTTYPE(Matrix)][MDESTTYPE(Matrix)];
  if ((ConType<0)||(ConType>=MAXCONNECTIONS))
  {
    UserWrite("wrong object given to PutFreeConnection\n");
    return(1);
  }

  ptr = (void **) object;
  ptr[0] = theMG->freeConnections[ConType];
  theMG->freeConnections[ConType] = (void *)object;
  return(0);
}

/****************************************************************************/
/*D
   PutFreeIMatrix -  Put an object in the free list

   SYNOPSIS:
   static INT PutFreeIMatrix (MULTIGRID *theMG, MATRIX *object);

   PARAMETERS:
   .  theMG - mg structure to extend
   .  object - Pointer to 'MATRIX' to be freed.

   DESCRIPTION:
   This function puts an object in the free list.

   RETURN VALUE:
   INT
   .n     0 if ok
   .n     INT>0 if no valid object number.
   D*/
/****************************************************************************/

#ifdef __INTERPOLATION_MATRIX__
static INT PutFreeIMatrix (MULTIGRID *theMG, MATRIX *object)
{
  void **ptr;

  ptr = (void **) object;
  ptr[0] = theMG->freeIMatrices[MROOTTYPE(object)][MDESTTYPE(object)];
  theMG->freeIMatrices[MROOTTYPE(object)][MDESTTYPE(object)]
    = (void *)object;
  return(0);
}
#endif

/****************************************************************************/
/*D
   CreateVector -  Return pointer to a new vector structure

   SYNOPSIS:
   INT CreateVector (GRID *theGrid, VECTOR *After, INT VectorType,
   VECTOR **VectorHandle);

   PARAMETERS:
   .  theGrid - grid where vector should be inserted
   .  After - vector after which to insert
   .  VectorType - one of the types defined in gm.h
   .  VectorHandle - handle of new vector, i.e. a pointer to a pointer where
   a pointer to the new vector is placed.

   DESCRIPTION:
   This function returns pointer to a new vector structure. First the free list
   is checked for a free entry, if none is available, a new structure is allocated
   from the heap.

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if error occured.
   D*/
/****************************************************************************/

INT CreateVector (GRID *theGrid, VECTOR *After, INT VectorType, VECTOR **VectorHandle)
{
  MULTIGRID *theMG;
  VECTOR *pv;
  INT ds, Size;

  theMG = MYMG(theGrid);
  ds = theMG->theFormat->VectorSizes[VectorType];
  if (ds == 0)
  {
    *VectorHandle = NULL;
    return (0);
  }
  Size = sizeof(VECTOR)-sizeof(DOUBLE)+ds;
  pv = GetFreeVector(theMG,VectorType);
  if (pv==NULL)
  {
    pv = (VECTOR *) GetMem(theMG->theHeap,Size,FROM_BOTTOM);
    if (pv==NULL)
      return(1);
  }
  memset(pv,0,Size);

  *VectorHandle = pv;

  /* initialize data */
  CTRL(pv) = 0;
  SETOBJT(pv,VEOBJ);
  SETVTYPE(pv,VectorType);
  SETVDATATYPE(pv,DATATYPE_FROM_VECTORTYPE(VectorType));
  SETVCLASS(pv,3);
  SETVNCLASS(pv,0);
  SETVBUILDCON(pv,1);
  SETVNEW(pv,1);
  SETVCNEW(pv,1);
  BVD_INIT( &VBVD( pv ) );
  pv->object = NULL;
  pv->index  = (long)theGrid->nVector;
  pv->skip   = 0;
  pv->start  = NULL;

#ifdef __INTERPOLATION_MATRIX__
  VISTART(pv) = NULL;
#endif

  /* insert in vector list */
  if (After==NULL)
  {
    SUCCVC(pv) = theGrid->firstVector;
    PREDVC(pv) = NULL;
    if (SUCCVC(pv)!=NULL)
      PREDVC(SUCCVC(pv)) = pv;
    theGrid->firstVector = (void*)pv;
    if (theGrid->lastVector==NULL)
      theGrid->lastVector = (void*)pv;
  }
  else
  {
    SUCCVC(pv) = SUCCVC(After);
    PREDVC(pv) = After;
    if (SUCCVC(pv)!=NULL)
      PREDVC(SUCCVC(pv)) = pv;
    else
      theGrid->lastVector = (void*)pv;
    SUCCVC(After) = pv;
  }

  /* counters */
  theGrid->nVector++;

  return (0);
}

/****************************************************************************/
/*D
   CreateBlockvector -  Return pointer to a new blockvector structure

   SYNOPSIS:
   INT CreateBlockvector( GRID *theGrid, BLOCKVECTOR **BVHandle )

   PARAMETERS:
   .  theGrid - grid where blockvector should be inserted
   .  BVHandle - handle of new blockvector, i.e. a pointer to a pointer where
   a pointer to the new blockvector is placed. Is NULL if there was no more
   memory.

   DESCRIPTION:
   This function returns pointer to a new blockvector structure. First
   the free list is checked for a free entry, if none is available,
   a new structure is allocated from the heap.

   RETURN VALUE:
   INT
   .n      GM_OK if ok
   .n      GM_OUT_OF_MEM if there is not not enough memory available
   D*/
/****************************************************************************/

INT CreateBlockvector( GRID *theGrid, BLOCKVECTOR **BVHandle )
{
  MULTIGRID *theMG;
  BLOCKVECTOR *bv;

  theMG = MYMG(theGrid);

  /* try to find an entry in the free list */
  if ( ( bv = (BLOCKVECTOR*)GetFreeObject( theMG, BLOCKVOBJ ) ) == NULL )
    /* if not found, allocate new memory */
    if ( ( bv = GetMem( MGHEAP(theMG), sizeof(BLOCKVECTOR), FROM_BOTTOM ) )
         == NULL )
    {
      *BVHandle = NULL;
      return GM_OUT_OF_MEM;
    }

  memset( bv, 0, sizeof(BLOCKVECTOR) );
  SETOBJT(bv,BLOCKVOBJ);

  *BVHandle = bv;

  return GM_OK;
}

/****************************************************************************/
/*D
   CreateIMatrix -  Return pointer to a new interpolation matrix structure

   SYNOPSIS:
   MATRIX *CreateIMatrix (GRID *theGrid, VECTOR *fvec, VECTOR *cvec);

   PARAMETERS:
   .  TheGrid - grid where matrix should be inserted
   .  fvec - fine grid vector
   .  cvec - coarse grid vector

   DESCRIPTION:
   This function allocates a new 'MATRIX' structures in the
   'imatrix' list of 'fvec'.

   RETURN VALUE:
   MATRIX *
   .n    pointer to the new matrix
   .n    NULL if error occured.
   D*/
/****************************************************************************/

#ifdef __INTERPOLATION_MATRIX__
MATRIX *CreateIMatrix (GRID *theGrid, VECTOR *fvec, VECTOR *cvec)
{
  MULTIGRID *theMG;
  HEAP *theHeap;
  MATRIX *pm,*m;
  INT RootType, DestType, MType, ds, Diag, Size;

  pm = GetIMatrix(fvec,cvec);
  if (pm != NULL)
    return(pm);

  RootType = VTYPE(fvec);
  DestType = VTYPE(cvec);
  MType = MatrixType[RootType][DestType];

  /* check expected size */
  theMG = MYMG(theGrid);
  theHeap = theMG->theHeap;
  ds = theMG->theFormat->IMatrixSizes[MType];
  if (ds == 0)
    return (NULL);
  Size = sizeof(MATRIX)-sizeof(DOUBLE)+ds;
  assert (Size % ALIGNMENT == 0);

  pm = GetFreeIMatrix (theMG,RootType,DestType);
  if (pm==NULL)
  {
    if (MSIZEMAX<Size) return (NULL);
    pm = (MATRIX*)GetMem(theHeap,Size,FROM_BOTTOM);
    if (pm==NULL)
      return (NULL);
  }
  memset(pm,0,Size);

  CTRL(pm) = 0;
  SETMTYPE(pm,MType);
  SETMROOTTYPE(pm,RootType);
  SETMDESTTYPE(pm,DestType);
  SETMSIZE(pm,Size);
  MDEST(pm) = cvec;
  MNEXT(pm) = VISTART(fvec);
  VISTART(fvec) = pm;

  /* counters */
  theGrid->nIMat++;

  return(pm);
}
#endif

/****************************************************************************/
/*D
   CreateConnection -  Return pointer to a new connection structure

   SYNOPSIS:
   CONNECTION *CreateConnection (GRID *theGrid, VECTOR *from, VECTOR *to);

   PARAMETERS:
   .  TheGrid - grid where matrix should be inserted
   .  from - source vector
   .  to - destination vector

   DESCRIPTION:
   This function allocates a new 'CONNECTION' and inserts the two
   'MATRIX' structures in the lists of 'from' and 'to' vector.
   Since the operation is symmetric, the order of 'from' and 'to'
   is not important.

   RETURN VALUE:
   CONNECTION *
   .n    pointer to the new connection
   .n    NULL if error occured.
   D*/
/****************************************************************************/

CONNECTION *CreateConnection (GRID *theGrid, VECTOR *from, VECTOR *to)
{
  MULTIGRID *theMG;
  HEAP *theHeap;
  CONNECTION *pc;
  MATRIX *pm;
  INT RootType, DestType, MType, ds, Diag, Size;

  /* set Diag, RootType and DestType	*/
  Diag = ((from == to) ? 1 : 0);
  RootType = VTYPE(from);
  DestType = VTYPE(to);
  MType = MatrixType[RootType][DestType];

  /* check expected size */
  theMG = MYMG(theGrid);
  theHeap = theMG->theHeap;
  ds = theMG->theFormat->MatrixSizes[MType];
  if (ds == 0)
    return (NULL);
  Size = sizeof(MATRIX)-sizeof(DOUBLE)+ds;
  assert (Size % ALIGNMENT == 0);

  /* is there already the desired connection ? */
  pc = GetConnection(from,to);
  if (pc != NULL)
  {
    SETCEXTRA(pc,0);
    return (pc);
  }

  pc = GetFreeConnection(theMG,Diag,RootType,DestType);
  if (pc==NULL)
  {
    if (MSIZEMAX<Size) return (NULL);
    if (Diag)
      pc = (CONNECTION*)GetMem(theHeap,Size,FROM_BOTTOM);
    else
      pc = (CONNECTION*)GetMem(theHeap,2*Size,FROM_BOTTOM);
    if (pc==NULL)
      return (NULL);
  }
  if (Diag)
    memset(pc,0,Size);
  else
    memset(pc,0,2*Size);

  /* initialize data */
  SETCEXTRA(pc,0);
  pm = CMATRIX0(pc);
  CTRL(pm) = 0;
  SETMTYPE(pm,MType);
  SETMROOTTYPE(pm,RootType);
  SETMDESTTYPE(pm,DestType);
  SETMDIAG(pm,Diag);
  SETMOFFSET(pm,0);
  SETMSIZE(pm,Size);
  SETMNEW(pm,1);
  SETVCNEW(to,1);
  MDEST(pm) = to;
  if (!Diag)
  {
    pm = CMATRIX1(pc);
    CTRL(pm) = 0;
    SETMTYPE(pm,MType);
    SETMROOTTYPE(pm,DestType);
    SETMDESTTYPE(pm,RootType);
    SETMDIAG(pm,Diag);
    SETMOFFSET(pm,1);
    SETMSIZE(pm,Size);
    SETMNEW(pm,1);
    SETVCNEW(from,1);
    MDEST(pm) = from;
  }

  /* set sizes */
  if (!Diag)
  {
    Size = (unsigned int)pm - (unsigned int)pc;
    SETMSIZE(pc,Size);
    SETMSIZE(pm,Size);
  }

  /* put in matrix list */
  if (Diag)
  {
    /* insert at first place in the list (only one matrix) */
    MNEXT(CMATRIX0(pc)) = VSTART(from);
    VSTART(from) = CMATRIX0(pc);
  }
  else
  {
    /* insert at second place in the list (both matrices) */
    pm = VSTART(from);
    if (pm == NULL)
    {
      MNEXT(CMATRIX0(pc)) = NULL;
      VSTART(from) = CMATRIX0(pc);
    }
    else
    {
      MNEXT(CMATRIX0(pc)) = MNEXT(pm);
      MNEXT(pm) = CMATRIX0(pc);
    }

    pm = VSTART(to);
    if (pm == NULL)
    {
      MNEXT(CMATRIX1(pc)) = NULL;
      VSTART(to) = CMATRIX1(pc);
    }
    else
    {
      MNEXT(CMATRIX1(pc)) = MNEXT(pm);
      MNEXT(pm) = CMATRIX1(pc);
    }
  }

  /* counters */
  theGrid->nCon++;

  return(pc);
}

/****************************************************************************/
/*D
   CreateExtraConnection -  Return pointer to a new matrix structure with extra flag set.

   SYNOPSIS:
   CONNECTION *CreateExtraConnection (GRID *theGrid, VECTOR *from,
   VECTOR *to);

   PARAMETERS:
   .  theGrid - grid level where connection will be inserted.
   .  from,to - Pointers to vectors where connection is inserted.

   DESCRIPTION:
   This function returns a pointer to a new 'CONNECTION'
   structure with extra flag set. This e.g. for a direct solver
   or ILU with fill in. The new connections can be differentiated
   from the connections necessary for the stiffness matrix.

   RETURN VALUE:
   INT
   .n    NULL if error occured.
   .n    else pointer to new 'CONNECTION'
   D*/
/****************************************************************************/

CONNECTION      *CreateExtraConnection  (GRID *theGrid, VECTOR *from, VECTOR *to)
{
  CONNECTION *pc;
  pc = CreateConnection(theGrid,from,to);
  if (pc==NULL) return(NULL);
  SETCEXTRA(pc,1);
  return(pc);
}

/****************************************************************************/
/*D
   DisposeIMatrices - Remove interpolation matrix from the data structure

   SYNOPSIS:
   INT DisposeIMatrices (GRID *theGrid, MATRIX *theMatrix);

   PARAMETERS:
   .  theGrid - the grid to remove from
   .  theMatrix - start of matrix list to dispose

   DESCRIPTION:
   This function removes an interpolation  matrix list from the data
   structure.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
   D*/
/****************************************************************************/

#ifdef __INTERPOLATION_MATRIX__
INT DisposeIMatrices (GRID *theGrid, MATRIX *theMatrix)
{
  VECTOR *from, *to;
  MATRIX *Matrix, *NextMatrix;

  for (Matrix=theMatrix; Matrix!=NULL; )
  {
    NextMatrix = NEXT(Matrix);
    /* free connection object */
    if (PutFreeIMatrix(theGrid->mg,Matrix))
      return(1);
    theGrid->nIMat--;
    Matrix = NextMatrix;
  }

  return(0);
}
#endif

/****************************************************************************/
/*D
   DisposeVector - Remove vector from the data structure

   SYNOPSIS:
   INT DisposeVector (GRID *theGrid, VECTOR *theVector);

   PARAMETERS:
   .  theGrid - grid level where theVector is in.
   .  theVector - 'VECTOR' to be disposed.

   DESCRIPTION:
   This function removes vector from the data structure and places
   it in the free list.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
   D*/
/****************************************************************************/

INT DisposeVector (GRID *theGrid, VECTOR *theVector)
{
  MATRIX *theMatrix;

  /* remove all connections concerning the vector */
  for (theMatrix=VSTART(theVector); theMatrix!=NULL;
       theMatrix=MNEXT(theMatrix))
    if (DisposeConnection(theGrid,MMYCON(theMatrix)))
      return (1);

#ifdef __INTERPOLATION_MATRIX__
  if (DisposeIMatrices(theGrid,VISTART(theVector)))
    return (1);
#endif

  /* now remove vector from vector list */
  if (PREDVC(theVector)!=NULL)
    SUCCVC(PREDVC(theVector)) = SUCCVC(theVector);
  else
    theGrid->firstVector = SUCCVC(theVector);
  if (SUCCVC(theVector)!=NULL)
    PREDVC(SUCCVC(theVector)) = PREDVC(theVector);
  else
    theGrid->lastVector = PREDVC(theVector);

  /* reset count flags */
  SETVCOUNT(theVector,0);

  /* delete the vector itself */
  if (PutFreeVector(theGrid->mg,theVector))
    return(1);

  theGrid->nVector--;

  return(0);
}

/****************************************************************************/
/*D
   DisposeBlockvector - Dispose blockvector

   SYNOPSIS:
   INT DisposeBlockvector( GRID *theGrid, BLOCKVECTOR *bv )

   PARAMETERS:
   .  theGrid - grid level where bv is in.
   .  bv - 'BLOCKVECTOR' to be disposed.

   DESCRIPTION:
   This function places the blockvector in the free list. The data structure
   must be kept consistent by the caller!

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
   D*/
/****************************************************************************/

INT DisposeBlockvector( GRID *theGrid, BLOCKVECTOR *bv )
{
  return PutFreeObject( MYMG(theGrid), bv);
}

/****************************************************************************/
/*D
   FreeAllBV - Frees all allocated blockvectors in the grid

   SYNOPSIS:
   void FreeAllBV( GRID *grid )

   PARAMETERS:
   .  grid - the grid from which all blockvectors are to be removed

   DESCRIPTION:
   Frees all allocated blockvectors in the grid and resets GFIRSTBV(grid)
   and GLASTBV(grid) to 'NULL' and gives the memory back to the memory
   of the 'MULTIGRID'.

   RETURN VALUE:
   void

   SEE ALSO:
   BLOCKVECTOR, FreeBVList

   D*/
/****************************************************************************/

void FreeAllBV( GRID *grid )
{
  FreeBVList( grid, GFIRSTBV( grid ) );
  GFIRSTBV( grid ) = NULL;
  GLASTBV( grid ) = NULL;
}

/****************************************************************************/
/*D
   FreeBVList - Frees blockvector-list and all its sons

   SYNOPSIS:
   void FreeBV( GRID *grid, BLOCKVECTOR *bv )

   PARAMETERS:
   .  grid - the grid from which the blockvectors are to be removed
   .  bv - start of the blockvector-list to be removed

   DESCRIPTION:
   Frees double linked blockvector-list, starting with 'bv', and all
   its sons recursively, i.e. all the
   subblockvectors. The vectors belonging to the blockvectors
   are not influenced.

   RETURN VALUE:
   void

   SEE ALSO:
   BLOCKVECTOR, FreeAllBV

   D*/
/****************************************************************************/

void FreeBVList( GRID *grid, BLOCKVECTOR *bv )
{
  register BLOCKVECTOR *bv_h;

  /* free all blocks in the current block list */
  for ( ; bv != NULL; )
  {
    /* free rekursively all higher block levels */
    if ( !BV_IS_LEAF_BV( bv ) )
      FreeBVList( grid, BVDOWNBV( bv ) );

    /* free the current block and go to the next */
    bv_h = BVSUCC( bv );
    DisposeBlockvector( grid, bv );
    bv = bv_h;
  }
}

/****************************************************************************/
/*D
   DisposeConnection - Remove connection from the data structure

   SYNOPSIS:
   INT DisposeConnection (GRID *theGrid, CONNECTION *theConnection);

   PARAMETERS:
   .  theGrid - the grid to remove from
   .  theConnection - connection to dispose

   DESCRIPTION:
   This function removes a connection from the data structure. The connection
   is removed from the list of the two vectors and is placed in the
   free list.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
   D*/
/****************************************************************************/

INT DisposeConnection (GRID *theGrid, CONNECTION *theConnection)
{
  VECTOR *from, *to;
  MATRIX *Matrix, *ReverseMatrix, *SearchMatrix;

  /* remove matrix(s) from their list(s) */
  Matrix = CMATRIX0(theConnection);
  to = MDEST(Matrix);
  if (MDIAG(Matrix))
    /* from == to */
    VSTART(to) = MNEXT(Matrix);
  else
  {
    ReverseMatrix = CMATRIX1(theConnection);
    from = MDEST(ReverseMatrix);
    if (VSTART(from) == Matrix)
      VSTART(from) = MNEXT(Matrix);
    else
      for (SearchMatrix=VSTART(from); SearchMatrix!=NULL; SearchMatrix=MNEXT(SearchMatrix))
        if (MNEXT(SearchMatrix) == Matrix)
          MNEXT(SearchMatrix) = MNEXT(Matrix);
    if (VSTART(to) == ReverseMatrix)
      VSTART(to) = MNEXT(ReverseMatrix);
    else
      for (SearchMatrix=VSTART(to); SearchMatrix!=NULL; SearchMatrix=MNEXT(SearchMatrix))
        if (MNEXT(SearchMatrix) == ReverseMatrix)
          MNEXT(SearchMatrix) = MNEXT(ReverseMatrix);
  }

  /* free connection object */
  if (PutFreeConnection(theGrid->mg,theConnection))
    return(1);

  /* return ok */
  theGrid->nCon--;

  return(0);
}

/****************************************************************************/
/*D
   DisposeDoubledSideVector - Dispose vector associated with two sides in 3D

   SYNOPSIS:
   INT DisposeDoubledSideVector (GRID *theGrid, ELEMENT *Elem0, INT Side0,
   ELEMENT *Elem1, INT Side1);

   PARAMETERS:
   .  theGrid - pointer to grid
   .  Elem0,side0 - first element and side
   .  Elem1,side1 - second element and side

   DESCRIPTION:
   This function disposes a vector associated with two sides in 3D.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
   D*/
/****************************************************************************/

INT DisposeDoubledSideVector (GRID *theGrid, ELEMENT *Elem0, INT Side0, ELEMENT *Elem1, INT Side1)
{
        #ifdef __SIDEDATA__
  VECTOR *Vector0, *Vector1;

  Vector0 = SVECTOR(Elem0,Side0);
  Vector1 = SVECTOR(Elem1,Side1);
  if (Vector0 == Vector1)
    return (0);
  assert(VCOUNT(Vector0)==1 && VCOUNT(Vector1)==1);
  assert(VSTART(Vector0)==NULL || VSTART(Vector1)==NULL);
  if (VSTART(Vector0)==NULL)
  {
    SET_SVECTOR(Elem0,Side0,Vector1);
    SETVCOUNT(Vector1,2);
    if (DisposeVector (theGrid,Vector0))
      return (1);
  }
  else
  {
    SET_SVECTOR(Elem1,Side1,Vector0);
    SETVCOUNT(Vector0,2);
    if (DisposeVector (theGrid,Vector1))
      return (1);
  }

  return (0);
        #endif

        #ifndef __SIDEDATA__
  return (1);
        #endif
}

/****************************************************************************/
/*D
   DisposeConnectionFromVector - Remove all connections associated with a vector

   SYNOPSIS:
   INT DisposeConnectionFromVector (GRID *theGrid, VECTOR *theVector);

   PARAMETERS:
   .  theGrid - grid level where vector belongs to
   .  theVector - vector where connections are disposed from

   DESCRIPTION:
   This function removes all connections from a vector.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
   D*/
/****************************************************************************/

INT DisposeConnectionFromVector (GRID *theGrid, VECTOR *theVector)
{
  while(VSTART(theVector) != NULL)
    if (DisposeConnection (theGrid,MMYCON(VSTART(theVector))))
      return (1);

  return (0);
}


/****************************************************************************/
/*D
   DisposeConnectionFromElement	- Removes all connections from all vectors
   associated with an element

   SYNOPSIS:
   INT DisposeConnectionFromElement (GRID *theGrid, ELEMENT *theElement);

   PARAMETERS:
   .  theGrid - grid level where element is on
   .  theElement - element from which to dispose connections

   DESCRIPTION:
   This function removes all connections from all vectors
   associated with an element.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
   D*/
/****************************************************************************/

INT DisposeConnectionFromElement (GRID *theGrid, ELEMENT *theElement)
{
  INT i;
  VECTOR *vList[20];
  INT cnt;

  GetVectorsOfElement(theElement,&cnt,vList);
  for (i=0; i<cnt; i++)
  {
    if (DisposeConnectionFromVector(theGrid,vList[i])) return(GM_ERROR);
    SETVBUILDCON(vList[i],1);
  }

  if (DIM==3)
  {
    GetVectorsOfSides(theElement,&cnt,vList);
    for (i=0; i<cnt; i++)
    {
      if (DisposeConnectionFromVector(theGrid,vList[i])) return(GM_ERROR);
      SETVBUILDCON(vList[i],1);
    }
  }

  GetVectorsOfEdges(theElement,&cnt,vList);
  for (i=0; i<cnt; i++)
  {
    if (DisposeConnectionFromVector(theGrid,vList[i])) return(GM_ERROR);
    SETVBUILDCON(vList[i],1);
  }

  GetVectorsOfNodes(theElement,&cnt,vList);
  for (i=0; i<cnt; i++)
  {
    if (DisposeConnectionFromVector(theGrid,vList[i])) return(GM_ERROR);
    SETVBUILDCON(vList[i],1);
  }

  return(GM_OK);
}

/****************************************************************************/
/*D
   DisposeConnectionFromElementInNeighborhood -  Remove matrices

   SYNOPSIS:
   static INT DisposeConnectionFromElementInNeighborhood (GRID *theGrid, ELEMENT *theElement,
   INT Depth);

   PARAMETERS:
   .  theGrid - the grid to remove from
   .  theElement - that element
   .  Depth -  that many slices around the element

   DESCRIPTION:
   This function removes connections concerning an element from the data structure
   and stores flags saying: "connection has to be rebuild",
   it does this in a neighborhood of the elem of depth Depth, where depth
   is the distance in the element-neighborship-graph (see also 'FORMAT').

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
   D*/
/****************************************************************************/

static INT DisposeConnectionFromElementInNeighborhood (GRID *theGrid, ELEMENT *theElement, INT Depth)
{
  INT i;

  if (Depth < 0) return (GM_ERROR);

  /* create connection at that depth */
  if(!EBUILDCON(theElement))
    if (DisposeConnectionFromElement(theGrid,theElement))
      return (GM_ERROR);
  SETEBUILDCON(theElement,1);

  /* dispose connection in neighborhood */
  if (Depth > 0)
  {
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      if (DisposeConnectionFromElementInNeighborhood(theGrid,NBELEM(theElement,i),Depth-1))
        return (GM_ERROR);
  }

  return (GM_OK);
}

INT DisposeConnectionsInNeighborhood (GRID *theGrid, ELEMENT *theElement)
{
  INT Depth;
  Depth = (INT)(floor(0.5*(double)theGrid->mg->theFormat->MaxConnectionDepth));
  return(DisposeConnectionFromElementInNeighborhood(theGrid,theElement,Depth));
}

/****************************************************************************/
/*D
   GetMatrix - Return pointer to matrix if it exists

   SYNOPSIS:
   MATRIX *GetMatrix (const VECTOR *FromVector, const VECTOR *ToVector);

   PARAMETERS:
   .  FromVector - starting vector of the Matrix
   .  ToVector - destination vector of the Matrix

   DESCRIPTION:
   This function returns pointer to matrix if it exists. The function
   runs through the single linked list, since the list is
   assumed to be small (sparse matrix!) the cost is assumed to be negligible.

   RETURN VALUE:
   MATRIX *
   .n       pointer to Matrix,
   .n       NULL if Matrix does not exist.
   D*/
/****************************************************************************/

MATRIX *GetMatrix (const VECTOR *FromVector, const VECTOR *ToVector)
{
  MATRIX *theMatrix;

  for (theMatrix=VSTART(FromVector); theMatrix!=NULL; theMatrix = MNEXT(theMatrix))
    if (MDEST(theMatrix)==ToVector)
      return (theMatrix);

  /* return not found */
  return (NULL);
}

/****************************************************************************/
/*D
   GetIMatrix - Return pointer to interpolation matrix if it exists

   SYNOPSIS:
   MATRIX *GetIMatrix (VECTOR *FineVector, VECTOR *CoarseVector);

   PARAMETERS:
   .  FineVector - fine grid vector
   .  CoarseVector - coarse grid vector

   DESCRIPTION:
   This function returns pointer to interpolation matrix.
   If it does not exist already, it retruns NULL.

   RETURN VALUE:
   MATRIX *
   .n       pointer to Matrix,
   .n       NULL if no Matrix exists.
   D*/
/****************************************************************************/

#ifdef __INTERPOLATION_MATRIX__
MATRIX *GetIMatrix (VECTOR *FineVector, VECTOR *CoarseVector)
{
  MATRIX *theMatrix;

  for (theMatrix=VISTART(FineVector); theMatrix!=NULL;
       theMatrix = MNEXT(theMatrix))
    if (MDEST(theMatrix)==CoarseVector)
      return (theMatrix);

  return (NULL);
}
#endif

/****************************************************************************/
/*D
   GetConnection - Return pointer to connection if it exists

   SYNOPSIS:
   CONNECTION *GetConnection (const VECTOR *FromVector, const VECTOR *ToVector);

   PARAMETERS:
   .  FromVector - starting vector of the con
   .  ToVector - destination vector of the con

   DESCRIPTION:
   This function returns pointer to connection if it exists.

   RETURN VALUE:
   CONNECTION *
   .n       pointer to
   .n       NULL if connection does not exist.
   D*/
/****************************************************************************/

CONNECTION *GetConnection (const VECTOR *FromVector, const VECTOR *ToVector)
{
  MATRIX *Matrix;

  Matrix = GetMatrix(FromVector,ToVector);
  if (Matrix != NULL)
    return (MMYCON(Matrix));

  /* return not found */
  return (NULL);
}


/****************************************************************************/
/*D
   GetVectorsOfElement - Get a pointer list to all element data

   SYNOPSIS:
   INT GetVectorsOfElement (const ELEMENT *theElement, INT *cnt, VECTOR **vList);

   PARAMETERS:
   .  theElement - that element
   .  cnt - how many vectors
   .  vList - array to store vector list

   DESCRIPTION:
   This function returns a pointer to the 'VECTOR' associated with the
   element (if the element is allowed to have one). 'cnt' will either
   be 0 or 1.

   RETURN VALUE:
   INT
   .n     GM_OK if ok
   .n     GM_ERROR	if error occured.
   D*/
/****************************************************************************/

INT GetVectorsOfElement (const ELEMENT *theElement, INT *cnt, VECTOR **vList)
{
  *cnt = 0;

        #ifdef __ELEMDATA__
  vList[0] = EVECTOR(theElement);
  *cnt = 1;
        #endif

  return(GM_OK);
}

/****************************************************************************/
/*D
   GetVectorsOfSides - Get a pointer list to all side data

   SYNOPSIS:
   INT GetVectorsOfSides (const ELEMENT *theElement, INT *cnt, VECTOR **vList);

   PARAMETERS:
   .  theElement - that element
   .  cnt - how many vectors
   .  vList - array to store vector list

   DESCRIPTION:
   This function gets a pointer array to all 'VECTOR's in sides of the given element.

   RETURN VALUE:
   INT
   .n     GM_OK if ok
   .n     GM_ERROR	if error occured.
   D*/
/****************************************************************************/

INT GetVectorsOfSides (const ELEMENT *theElement, INT *cnt, VECTOR **vList)
{
        #ifdef __SIDEDATA__
  INT i;
        #endif

  *cnt = 0;
  if (DIM==2) return(GM_OK);

        #ifdef __SIDEDATA__
  for (i=0; i<SIDES_OF_ELEM(theElement); i++)
    vList[(*cnt)++] = SVECTOR(theElement,i);
        #endif

  return(GM_OK);
}

/****************************************************************************/
/*D
   GetVectorsOfEdges - Get a pointer list to all edge data

   SYNOPSIS:
   INT GetVectorsOfEdges (const ELEMENT *theElement, INT *cnt, VECTOR **vList);

   PARAMETERS:
   .  theElement -  that element
   .  cnt - how many vectors
   .  vList - array to store vector list

   DESCRIPTION:
   This function gets a pointer array to all 'VECTOR's in edges of the element.

   RETURN VALUE:
   INT
   .n     GM_OK if ok
   .n     GM_ERROR	if error occured.
   D*/
/****************************************************************************/

INT GetVectorsOfEdges (const ELEMENT *theElement, INT *cnt, VECTOR **vList)
{
        #ifdef __EDGEDATA__
  EDGE *theEdge;
  INT i,j,n;
        #endif

  *cnt = 0;
        #ifdef __EDGEDATA__
  if (DIM==3)
  {
    for (i=0; i<CORNERS_OF_ELEM(theElement); i++)
      for (j=i+1; j<CORNERS_OF_ELEM(theElement); j++)
      {
        theEdge = GetEdge(CORNER(theElement,i),CORNER(theElement,j));
        if (theEdge==NULL) continue;
        vList[(*cnt)++] = EDVECTOR(theEdge);
      }
    return(GM_OK);
  }
  if (DIM==2)
  {
    n = CORNERS_OF_ELEM(theElement);
    for (i=0; i<n; i++)
    {
      theEdge = GetEdge(CORNER(theElement,i),CORNER(theElement,(i+1)%n));
      if (theEdge==NULL) continue;
      vList[(*cnt)++] = EDVECTOR(theEdge);
    }
    return(GM_OK);
  }
        #endif

  return (0);
}

/****************************************************************************/
/*D
   GetVectorsOfNodes - Get a pointer list to all node data

   SYNOPSIS:
   INT GetVectorsOfNodes (const ELEMENT *theElement, INT *cnt, VECTOR **vList);

   PARAMETERS:
   .  theElement -  that element
   .  cnt - how many vectors
   .  vList - array to store vector list

   DESCRIPTION:
   This function gets a pointer array to all 'VECTOR's in nodes of the element.

   RETURN VALUE:
   INT
   .n     GM_OK if ok
   .n     GM_ERROR	if error occured.
   D*/
/****************************************************************************/

INT GetVectorsOfNodes (const ELEMENT *theElement, INT *cnt, VECTOR **vList)
{
        #ifdef __NODEDATA__
  INT i;
        #endif

  *cnt = 0;
        #ifdef __NODEDATA__
  for (i=0; i<CORNERS_OF_ELEM(theElement); i++)
    vList[(*cnt)++] = NVECTOR(CORNER(theElement,i));
        #endif
  return (GM_OK);
}

/****************************************************************************/
/*D
   GetVectorsOfType - Get a pointer list to all vector data of specified type

   SYNOPSIS:
   INT GetVectorsOfType (const ELEMENT *theElement, INT type, INT *cnt, VECTOR **vList)

   PARAMETERS:
   .  theElement -  that element
   .  type - fill array with vectors of this type
   .  cnt - how many vectors
   .  vList - array to store vector list

   DESCRIPTION:
   This function returns a pointer array to all 'VECTOR's of the element of the specified type.

   RETURN VALUE:
   INT
   .n     GM_OK if ok
   .n     GM_ERROR	if error occured.
   D*/
/****************************************************************************/

INT GetVectorsOfType (const ELEMENT *theElement, INT type, INT *cnt, VECTOR **vList)
{
  switch (type)
  {
  case NODEVECTOR :
    return (GetVectorsOfNodes(theElement,cnt,vList));
  case ELEMVECTOR :
    return (GetVectorsOfElement(theElement,cnt,vList));
  case EDGEVECTOR :
    return (GetVectorsOfEdges(theElement,cnt,vList));
  case SIDEVECTOR :
    return (GetVectorsOfSides(theElement,cnt,vList));
  }
  return (GM_ERROR);
}

/****************************************************************************/
/*D
   DisposeExtraConnections - Remove all extra connections from the grid

   SYNOPSIS:
   INT DisposeExtraConnections (GRID *theGrid);

   PARAMETERS:
   .  theGrid - grid to remove from

   DESCRIPTION:
   This function removes all extra connections from the grid, i.e. those
   that have been allocated with the 'CreateExtraConnection' function.

   RETURN VALUE:
   INT
   .n       0 if ok
   .n       1 if error occured
   D*/
/****************************************************************************/

INT DisposeExtraConnections (GRID *theGrid)
{
  VECTOR *theVector;
  MATRIX *theMatrix, *nextMatrix;
  CONNECTION *theCon;

  for (theVector=FIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
  {
    theMatrix = VSTART(theVector);
    while (theMatrix!=NULL)
    {
      nextMatrix = MNEXT(theMatrix);
      theCon = MMYCON(theMatrix);
      if (CEXTRA(theCon)) DisposeConnection(theGrid,theCon);
      theMatrix = nextMatrix;
    }
  }
  return(GM_OK);
}

/****************************************************************************/
/*D
   ElementElementCreateConnection - Create connections of two elements

   SYNOPSIS:
   static INT ElementElementCreateConnection (GRID *theGrid, ELEMENT *Elem0,
   ELEMENT *Elem1, INT ActDepth, INT *ConDepth);

   PARAMETERS:
   .  theGrid - pointer to grid
   .  Elem0,Elem1 - elements to be connected
   .  ActDepth - distance of the two elements in the element neighborship graph
   .  ConDepth - Array containing the connection depth desired. This is constructed
   from the information in the FORMAT.

   DESCRIPTION:
   This function creates connections between all the 'VECTOR's associated
   with two given elements according to the specifications in the 'ConDepth' array.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
   D*/
/****************************************************************************/

static INT ElementElementCreateConnection (GRID *theGrid, ELEMENT *Elem0, ELEMENT *Elem1, INT ActDepth, INT *ConDepth)
{
  INT i,j;
  INT elemCnt0, sideCnt0, edgeCnt0, nodeCnt0;
  INT elemCnt1, sideCnt1, edgeCnt1, nodeCnt1;
  VECTOR *elemVec0[12], *sideVec0[12], *edgeVec0[12], *nodeVec0[12];
  VECTOR *elemVec1[12], *sideVec1[12], *edgeVec1[12], *nodeVec1[12];

  /* initialize pointer arrays */
  if (GetVectorsOfElement(Elem0,&elemCnt0,elemVec0)) return(GM_ERROR);
  if (GetVectorsOfSides(Elem0,&sideCnt0,sideVec0)) return(GM_ERROR);
  if (GetVectorsOfEdges(Elem0,&edgeCnt0,edgeVec0)) return(GM_ERROR);
  if (GetVectorsOfNodes(Elem0,&nodeCnt0,nodeVec0)) return(GM_ERROR);
  if (GetVectorsOfElement(Elem1,&elemCnt1,elemVec1)) return(GM_ERROR);
  if (GetVectorsOfSides(Elem1,&sideCnt1,sideVec1)) return(GM_ERROR);
  if (GetVectorsOfEdges(Elem1,&edgeCnt1,edgeVec1)) return(GM_ERROR);
  if (GetVectorsOfNodes(Elem1,&nodeCnt1,nodeVec1)) return(GM_ERROR);

  /* create node node connection */
  if (ActDepth <= ConDepth[MatrixType[NODEVECTOR][NODEVECTOR]])
    for (i=0; i<nodeCnt0; i++)
      for (j=0; j<nodeCnt1; j++)
        if (CreateConnection(theGrid,nodeVec0[i],nodeVec1[j])==NULL) return(GM_ERROR);

  /* create node edge connection */
  if (ActDepth <= ConDepth[MatrixType[NODEVECTOR][EDGEVECTOR]])
  {
    /* edges in 0 with nodes in 1 */
    for (i=0; i<edgeCnt0; i++)
      for (j=0; j<nodeCnt1; j++)
        if (CreateConnection(theGrid,edgeVec0[i],nodeVec1[j])==NULL) return(GM_ERROR);
    /* edges 1 with nodes in 0 */
    for (i=0; i<edgeCnt1; i++)
      for (j=0; j<nodeCnt0; j++)
        if (CreateConnection(theGrid,edgeVec1[i],nodeVec0[j])==NULL) return(GM_ERROR);
  }

  /* create node side connection */
  if ((DIM==3)&&(ActDepth <= ConDepth[MatrixType[NODEVECTOR][SIDEVECTOR]]))
  {
    for (i=0; i<nodeCnt0; i++)
      for (j=0; j<sideCnt1; j++)
        if (CreateConnection(theGrid,nodeVec0[i],sideVec1[j])==NULL) return (GM_ERROR);
    for (i=0; i<nodeCnt1; i++)
      for (j=0; j<sideCnt0; j++)
        if (CreateConnection(theGrid,nodeVec1[i],sideVec0[j])==NULL) return (GM_ERROR);
  }

  /* create node elem connection */
  if (ActDepth <= ConDepth[MatrixType[NODEVECTOR][ELEMVECTOR]])
  {
    for (i=0; i<nodeCnt0; i++)
      for (j=0; j<elemCnt1; j++)
        if (CreateConnection(theGrid,nodeVec0[i],elemVec1[j])==NULL) return (GM_ERROR);
    for (i=0; i<nodeCnt1; i++)
      for (j=0; j<elemCnt0; j++)
        if (CreateConnection(theGrid,nodeVec1[i],elemVec0[j])==NULL) return (GM_ERROR);
  }

  /* create edge edge connection */
  if (ActDepth <= ConDepth[MatrixType[EDGEVECTOR][EDGEVECTOR]])
  {
    for (i=0; i<edgeCnt0; i++)
      for (j=0; j<edgeCnt1; j++)
        if (CreateConnection(theGrid,edgeVec0[i],edgeVec1[j])==NULL) return (GM_ERROR);
  }

  /* create edge side connection */
  if ((DIM==3)&&(ActDepth <= ConDepth[MatrixType[EDGEVECTOR][SIDEVECTOR]]))
  {
    for (i=0; i<edgeCnt0; i++)
      for (j=0; j<sideCnt1; j++)
        if (CreateConnection(theGrid,edgeVec0[i],sideVec1[j])==NULL) return (GM_ERROR);
    for (i=0; i<edgeCnt1; i++)
      for (j=0; j<sideCnt0; j++)
        if (CreateConnection(theGrid,edgeVec1[i],sideVec0[j])==NULL) return (GM_ERROR);
  }

  /* create edge elem connection */
  if (ActDepth <= ConDepth[MatrixType[EDGEVECTOR][ELEMVECTOR]])
  {
    for (i=0; i<edgeCnt0; i++)
      for (j=0; j<elemCnt1; j++)
        if (CreateConnection(theGrid,edgeVec0[i],elemVec1[j])==NULL) return (GM_ERROR);
    for (i=0; i<edgeCnt1; i++)
      for (j=0; j<elemCnt0; j++)
        if (CreateConnection(theGrid,edgeVec1[i],elemVec0[j])==NULL) return (GM_ERROR);
  }

  /* create side side connection */
  if ((DIM==3)&&(ActDepth <= ConDepth[MatrixType[SIDEVECTOR][SIDEVECTOR]]))
  {
    for (i=0; i<sideCnt0; i++)
      for (j=0; j<sideCnt1; j++)
        if (CreateConnection(theGrid,sideVec0[i],sideVec1[j])==NULL) return (GM_ERROR);
  }

  /* create side elem connection */
  if ((DIM==3)&&(ActDepth <= ConDepth[MatrixType[SIDEVECTOR][ELEMVECTOR]]))
  {
    for (i=0; i<sideCnt0; i++)
      for (j=0; j<elemCnt1; j++)
        if (CreateConnection(theGrid,sideVec0[i],elemVec1[j])==NULL) return (GM_ERROR);
    for (i=0; i<sideCnt1; i++)
      for (j=0; j<elemCnt0; j++)
        if (CreateConnection(theGrid,sideVec1[i],elemVec0[j])==NULL) return (GM_ERROR);
  }

  /* create elem elem connection */
  if (ActDepth <= ConDepth[MatrixType[ELEMVECTOR][ELEMVECTOR]])
  {
    for (i=0; i<elemCnt0; i++)
      for (j=0; j<elemCnt1; j++)
        if (CreateConnection(theGrid,elemVec0[i],elemVec1[j])==NULL) return (GM_ERROR);
  }

  return (0);
}


/****************************************************************************/
/*D
   GetElementInfoFromSideVector	- Get pointers to elements having a common side

   SYNOPSIS:
   INT GetElementInfoFromSideVector (const VECTOR *theVector,
   ELEMENT **Elements, INT *Sides);

   PARAMETERS:
   .  theVector - given vector associated with a side of an element in 3D
   .  Elements - array to be filled with two pointers of elements
   .  Sides - array to be filled with side number within respective element

   DESCRIPTION:
   Given a 'VECTOR' associated with the side of an element, this function
   retrieves pointers to at most two elements having this side in common.
   If the side is part of the exterior boundary of the domain, then
   'Elements[1]' will be the 'NULL' pointer.

   RETURN VALUE:
   INT
   .n     0 if ok
   .n     1 if error occured.
   D*/
/****************************************************************************/

#ifdef __THREEDIM__
INT GetElementInfoFromSideVector (const VECTOR *theVector, ELEMENT **Elements, INT *Sides)
{
  INT i;
  ELEMENT *theNeighbor;

  if (VTYPE(theVector) != SIDEVECTOR)
    return (1);
  Elements[0] = (ELEMENT *)VOBJECT(theVector);
  Sides[0] = VECTORSIDE(theVector);

  /* find neighbor */
  Elements[1] = theNeighbor = NBELEM(Elements[0],Sides[0]);
  if (theNeighbor == NULL) return (0);

  /* search the side */
  for (i=0; i<SIDES_OF_ELEM(theNeighbor); i++)
    if (NBELEM(theNeighbor,i) == Elements[0])
      break;

  /* found ? */
  if (i<SIDES_OF_ELEM(theNeighbor))
  {
    Sides[1] = i;
    return (0);
  }
  return (1);
}
#endif

/****************************************************************************/
/*
   ResetUsedFlagInNeighborhood - Reset all 'USED' flags in neighborhood of an element

   SYNOPSIS:
   static INT ResetUsedFlagInNeighborhood (ELEMENT *theElement,
   INT ActDepth, INT MaxDepth);

   PARAMETERS:
   .  theElement - given element
   .  ActDepth - recursion depth
   .  MaxDepth - end of recursion

   DESCRIPTION:
   This function calls itself recursively and resets all 'USED' flags in the
   neighborhood of depth 'MaxDepth' of 'theElement'. For the first call
   'ActDepth' should be zero.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
 */
/****************************************************************************/

static INT ResetUsedFlagInNeighborhood (ELEMENT *theElement, INT ActDepth, INT MaxDepth)
{
  int i;

  /* is anything to do ? */
  if (theElement==NULL) return (0);

  /* action */
  if (ActDepth>=0) SETUSED(theElement,0);

  /* call all neighbors recursively */
  if (ActDepth<MaxDepth)
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      if (ResetUsedFlagInNeighborhood(NBELEM(theElement,i),ActDepth+1,MaxDepth)) return (1);

  return (0);
}

static INT ConnectWithNeighborhood (ELEMENT *theElement, GRID *theGrid, ELEMENT *centerElement, INT *ConDepth, INT ActDepth, INT MaxDepth)
{
  int i;

  /* is anything to do ? */
  if (theElement==NULL) return (0);

  /* action */
  if (ActDepth>=0)
    if (ElementElementCreateConnection(theGrid,centerElement,theElement,ActDepth,ConDepth))
      return (1);

  /* call all neighbors recursively */
  if (ActDepth<MaxDepth)
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      if (ConnectWithNeighborhood(NBELEM(theElement,i),theGrid,centerElement,ConDepth,ActDepth+1,MaxDepth)) return (1);

  return (0);
}
/****************************************************************************/
/*D
   CreateConnectionsInNeighborhood - Create connection of an element

   SYNOPSIS:
   INT CreateConnectionsInNeighborhood (GRID *theGrid, ELEMENT *theElement);

   PARAMETERS:
   .  theGrid - pointer to grid
   .  theElement - pointer to an element

   DESCRIPTION:
   This function creates connection for all 'VECTOR's of an element
   with the depth specified in the 'FORMAT' structure.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
   D*/
/****************************************************************************/
INT CreateConnectionsInNeighborhood (GRID *theGrid, ELEMENT *theElement)
{
  MULTIGRID *theMG;
  FORMAT *theFormat;
  INT MaxDepth;
  INT *ConDepth;

  /* set pointers */
  theMG = theGrid->mg;
  theFormat = theMG->theFormat;
  MaxDepth = theFormat->MaxConnectionDepth;
  ConDepth = theFormat->ConnectionDepth;

  /* reset used flags in neighborhood */
  if (ResetUsedFlagInNeighborhood(theElement,0,MaxDepth))
    return (1);

  /* create connection in neighborhood */
  if (ConnectWithNeighborhood(theElement,theGrid,theElement,ConDepth,0,MaxDepth))
    return (1);

  return (0);
}

/****************************************************************************/
/*
   CreateconnectionOfInsertedElement -  Create connection of an inserted element

   SYNOPSIS:
   static INT ConnectInsertedWithNeighborhood (ELEMENT *theElement, GRID *theGrid,
   INT ActDepth, INT MaxDepth);

   PARAMETERS:
   .  theElement -
   .  theGrid -
   .  ActDepth -
   .  MaxDepth -

   DESCRITION:
   This function creates connection of an inserted element ,
   i.e. an element is inserted interactively by the user.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
 */
/****************************************************************************/

static INT ConnectInsertedWithNeighborhood (ELEMENT *theElement, GRID *theGrid, INT ActDepth, INT MaxDepth)
{
  int i;

  /* is anything to do ? */
  if (theElement==NULL) return (0);

  /* action */
  if (ActDepth>=0)
    if (CreateConnectionsInNeighborhood(theGrid,theElement))
      return (1);

  /* call all neighbors recursively */
  if (ActDepth<MaxDepth)
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      if (ConnectInsertedWithNeighborhood(NBELEM(theElement,i),theGrid,ActDepth+1,MaxDepth)) return (1);

  return (0);
}

/****************************************************************************/
/*D
   InsertedElementCreateConnection -  Create connection of an inserted element

   SYNOPSIS:
   INT InsertedElementCreateConnection (GRID *theGrid, ELEMENT *theElement);

   PARAMETERS:
   .  theGrid - grid level
   .  theElement -  pointer to an element

   DESCRITION:
   This function creates connections of an inserted element ,
   i.e. when an element is inserted interactively by the user. This function
   is inefficient and only intended for that purpose.

   RETURN VALUE:
   INT
   .n   0 if ok
   .n   1 if error occured.
   D*/
/****************************************************************************/
INT InsertedElementCreateConnection (GRID *theGrid, ELEMENT *theElement)
{
  MULTIGRID *theMG;
  FORMAT *theFormat;
  INT MaxDepth;

  /* set pointers */
  theMG = theGrid->mg;
  theFormat = theMG->theFormat;
  MaxDepth = (INT)(floor(0.5*(double)theFormat->MaxConnectionDepth));

  /* reset used flags in neighborhood */
  if (ResetUsedFlagInNeighborhood(theElement,0,MaxDepth))
    return (1);

  /* call 'CreateConnectionsInNeighborhood' in a neighborhood of theElement */
  if (ConnectInsertedWithNeighborhood (theElement,theGrid,0,MaxDepth))
    return (1);

  return (0);
}

/****************************************************************************/
/*D
   GridCreateConnection	- Create all connections needed on a grid level

   SYNOPSIS:
   INT GridCreateConnection (GRID *theGrid);

   PARAMETERS:
   .  theGrid - pointer to grid

   DESCRIPTION:
   This function creates all connections needed on the grid.

   RETURN VALUE:
   INT
   .n     0 if ok
   .n     1 if error occured.
   D*/
/****************************************************************************/

INT GridCreateConnection (GRID *theGrid)
{
  ELEMENT *theElement;
  VECTOR *vList[20];
  INT i,cnt;

  /* lets see if there's something to do */
  if (theGrid == NULL)
    return (0);

  /* set EBUILDCON-flags also in elements accessing a vector with VBUILDCON true */
  for (theElement=theGrid->elements; theElement!=NULL; theElement=SUCCE(theElement))
  {
    /* see if it is set */
    if (EBUILDCON(theElement)) continue;

    /* check flags in vectors */
    if (DIM==3)
    {
      GetVectorsOfSides(theElement,&cnt,vList);
      for (i=0; i<cnt; i++)
        if (VBUILDCON(vList[i])) {SETEBUILDCON(theElement,1); break;}
    }
    if (EBUILDCON(theElement)) continue;
    GetVectorsOfEdges(theElement,&cnt,vList);
    for (i=0; i<cnt; i++)
      if (VBUILDCON(vList[i])) {SETEBUILDCON(theElement,1); break;}
    if (EBUILDCON(theElement)) continue;
    GetVectorsOfNodes(theElement,&cnt,vList);
    for (i=0; i<cnt; i++)
      if (VBUILDCON(vList[i])) {SETEBUILDCON(theElement,1); break;}
  }

  /* run over all elements with EBUILDCON true and build connections */
  for (theElement=theGrid->elements; theElement!=NULL; theElement=SUCCE(theElement))
    if (EBUILDCON(theElement))             /* this is the trigger ! */
      if (CreateConnectionsInNeighborhood(theGrid,theElement))
        return (1);

  return(GM_OK);
}

/****************************************************************************/
/*D
   MGCreateConnection - Create all connections in multigrid

   SYNOPSIS:
   INT MGCreateConnection (MULTIGRID *theMG);

   PARAMETERS:
   .  theMG - pointer to mulrigrid

   DESCRIPTION:
   This function creates all connections in multigrid.

   RETURN VALUE:
   INT
   .n     0 if ok
   .n     1 if error occured.
   D*/
/****************************************************************************/

INT MGCreateConnection (MULTIGRID *theMG)
{
  INT i;
  GRID *theGrid;
  ELEMENT *theElement;

  for (i=0; i<=theMG->topLevel; i++)
  {
    theGrid = theMG->grids[i];
    for (theElement=theGrid->elements; theElement!=NULL; theElement=SUCCE(theElement))
      SETEBUILDCON(theElement,1);
    if (GridCreateConnection(theGrid)) return (1);
  }

  return (0);
}

/****************************************************************************/
/*D
   ResetAlgebraInternalFlags - Reset USED and EBUILDCON flags in elements

   SYNOPSIS:
   INT PrepareAlgebraModification (MULTIGRID *theMG);

   PARAMETERS:
   .  theMG - pointer to multigrid

   DESCRIPTION:
   This function resets  USED and EBUILDCON flags in elements.

   RETURN VALUE:
   INT
   .n     0 if ok
   .n     1 if error occured.
   D*/
/****************************************************************************/

INT PrepareAlgebraModification (MULTIGRID *theMG)
{
  int j,k;
  ELEMENT *theElement;
  VECTOR *theVector;
  MATRIX *theMatrix;

  j = theMG->topLevel;
  for (k=0; k<=j; k++)
  {
    for (theElement=theMG->grids[k]->elements; theElement!=NULL; theElement=SUCCE(theElement))
    {
      SETUSED(theElement,0);
      SETEBUILDCON(theElement,0);
    }
    for (theVector=theMG->grids[k]->firstVector; theVector!= NULL; theVector=SUCCVC(theVector))
      SETVBUILDCON(theVector,0);
    for (theVector=theMG->grids[k]->firstVector; theVector!= NULL; theVector=SUCCVC(theVector))
    {
      SETVNEW(theVector,0);
      SETVCNEW(theVector,0);
      for (theMatrix=VSTART(theVector); theMatrix!=NULL; theMatrix = MNEXT(theMatrix))
        SETMNEW(theMatrix,0);
    }
  }

  return (0);
}


/****************************************************************************/
/*D
   ElementElementCheck - Check connection of two elements

   SYNOPSIS:
   static INT ElementElementCheck (ELEMENT *Elem0, ELEMENT *Elem1,
   INT ActDepth, INT *ConDepth);

   PARAMETERS:
   .  Elem0,Elem1 - elements to be checked.
   .  ActDepth - recursion depth
   .  ConDepth - connection depth as provided by format

   DESCRIPTION:
   This function recursively checks connection of two elements.

   RETURN VALUE:
   INT
   .n     0 if ok
   .n     1 if error occured.
   D*/
/****************************************************************************/

static INT ElementElementCheck (ELEMENT *Elem0, ELEMENT *Elem1, INT ActDepth, INT *ConDepth)
{
  INT i,j;
  INT elemCnt0, sideCnt0, edgeCnt0, nodeCnt0;
  INT elemCnt1, sideCnt1, edgeCnt1, nodeCnt1;
  VECTOR *elemVec0[12], *sideVec0[12], *edgeVec0[12], *nodeVec0[12];
  VECTOR *elemVec1[12], *sideVec1[12], *edgeVec1[12], *nodeVec1[12];
  CONNECTION *theCon;
  char buffer[256], msg[128];
  INT ReturnCode;

  /* initialize pointer arrays */
  if (GetVectorsOfElement(Elem0,&elemCnt0,elemVec0)) return(GM_ERROR);
  if (GetVectorsOfSides(Elem0,&sideCnt0,sideVec0)) return(GM_ERROR);
  if (GetVectorsOfEdges(Elem0,&edgeCnt0,edgeVec0)) return(GM_ERROR);
  if (GetVectorsOfNodes(Elem0,&nodeCnt0,nodeVec0)) return(GM_ERROR);
  if (GetVectorsOfElement(Elem1,&elemCnt1,elemVec1)) return(GM_ERROR);
  if (GetVectorsOfSides(Elem1,&sideCnt1,sideVec1)) return(GM_ERROR);
  if (GetVectorsOfEdges(Elem1,&edgeCnt1,edgeVec1)) return(GM_ERROR);
  if (GetVectorsOfNodes(Elem1,&nodeCnt1,nodeVec1)) return(GM_ERROR);

  sprintf(msg,"error in connection between element %lu and %lu: ",(long)ID(Elem0),(long)ID(Elem1));

  /* reset return code */
  ReturnCode = GM_OK;

  /* check node node connection */
  if (ActDepth <= ConDepth[MatrixType[NODEVECTOR][NODEVECTOR]])
    for (i=0; i<nodeCnt0; i++)
      for (j=0; j<nodeCnt1; j++)
      {
        theCon = GetConnection(nodeVec0[i],nodeVec1[j]);
        if (theCon==NULL)
        {
          UserWrite(msg);
          sprintf(buffer,"nodeVec0[%d] to nodeVec1[%d]\n",i,j);
          UserWrite(buffer);
          ReturnCode = GM_ERROR;
        }
        else
        {
          SETCUSED(theCon,1);
        }
      }


  /* check node edge connection */
  if (ActDepth <= ConDepth[MatrixType[NODEVECTOR][EDGEVECTOR]])
  {
    /* edges in 0 with nodes in 1 */
    for (i=0; i<edgeCnt0; i++)
      for (j=0; j<nodeCnt1; j++)
      {
        theCon = GetConnection(edgeVec0[i],nodeVec1[j]);
        if (theCon==NULL)
        {
          UserWrite(msg);
          sprintf(buffer,"edgeVec0[%d] to nodeVec1[%d]\n",i,j);
          UserWrite(buffer);
          ReturnCode = GM_ERROR;
        }
        else SETCUSED(theCon,1);
      }
    /* edges 1 with nodes in 0 */
    for (i=0; i<edgeCnt1; i++)
      for (j=0; j<nodeCnt0; j++)
      {
        theCon = GetConnection(edgeVec1[i],nodeVec0[j]);
        if (theCon==NULL)
        {
          UserWrite(msg);
          sprintf(buffer,"edgeVec1[%d] to nodeVec0[%d]\n",i,j);
          UserWrite(buffer);
          ReturnCode = GM_ERROR;
        }
        else SETCUSED(theCon,1);
      }
  }

  /* check node side connection */
  if ((DIM==3)&&(ActDepth <= ConDepth[MatrixType[NODEVECTOR][SIDEVECTOR]]))
  {
    for (i=0; i<nodeCnt0; i++)
      for (j=0; j<sideCnt1; j++)
      {
        theCon = GetConnection(nodeVec0[i],sideVec1[j]);
        if (theCon==NULL)
        {
          UserWrite(msg);
          sprintf(buffer,"nodeVec0[%d] to sideVec1[%d]\n",i,j);
          UserWrite(buffer);
          ReturnCode = GM_ERROR;
        }
        else SETCUSED(theCon,1);
      }
    for (i=0; i<nodeCnt1; i++)
      for (j=0; j<sideCnt0; j++)
      {
        theCon = GetConnection(nodeVec1[i],sideVec0[j]);
        if (theCon==NULL)
        {
          UserWrite(msg);
          sprintf(buffer,"nodeVec1[%d] to sideVec0[%d]\n",i,j);
          UserWrite(buffer);
          ReturnCode = GM_ERROR;
        }
        else SETCUSED(theCon,1);
      }
  }

  /* check node elem connection */
  if (ActDepth <= ConDepth[MatrixType[NODEVECTOR][ELEMVECTOR]])
  {
    for (i=0; i<nodeCnt0; i++)
      for (j=0; j<elemCnt1; j++)
      {
        theCon = GetConnection(nodeVec0[i],elemVec1[j]);
        if (theCon==NULL)
        {
          UserWrite(msg);
          sprintf(buffer,"nodeVec0[%d] to elemVec1[%d]\n",i,j);
          UserWrite(buffer);
          ReturnCode = GM_ERROR;
        }
        else SETCUSED(theCon,1);
      }
    for (i=0; i<nodeCnt1; i++)
      for (j=0; j<elemCnt0; j++)
      {
        theCon = GetConnection(nodeVec1[i],elemVec0[j]);
        if (theCon==NULL)
        {
          UserWrite(msg);
          sprintf(buffer,"nodeVec1[%d] to elemVec0[%d]\n",i,j);
          UserWrite(buffer);
          ReturnCode = GM_ERROR;
        }
        else SETCUSED(theCon,1);
      }
  }

  /* check edge edge connection */
  if (ActDepth <= ConDepth[MatrixType[EDGEVECTOR][EDGEVECTOR]])
  {
    for (i=0; i<edgeCnt0; i++)
      for (j=0; j<edgeCnt1; j++)
      {
        theCon = GetConnection(edgeVec0[i],edgeVec1[j]);
        if (theCon==NULL)
        {
          UserWrite(msg);
          sprintf(buffer,"edgeVec0[%d] to edgeVec1[%d]\n",i,j);
          UserWrite(buffer);
          ReturnCode = GM_ERROR;
        }
        else SETCUSED(theCon,1);
      }
  }

  /* check edge side connection */
  if ((DIM==3)&&(ActDepth <= ConDepth[MatrixType[EDGEVECTOR][SIDEVECTOR]]))
  {
    for (i=0; i<edgeCnt0; i++)
      for (j=0; j<sideCnt1; j++)
      {
        theCon = GetConnection(edgeVec0[i],sideVec1[j]);
        if (theCon==NULL)
        {
          UserWrite(msg);
          sprintf(buffer,"edgeVec0[%d] to sideVec1[%d]\n",i,j);
          UserWrite(buffer);
          ReturnCode = GM_ERROR;
        }
        else SETCUSED(theCon,1);
      }
    for (i=0; i<edgeCnt1; i++)
      for (j=0; j<sideCnt0; j++)
      {
        theCon = GetConnection(edgeVec1[i],sideVec0[j]);
        if (theCon==NULL)
        {
          UserWrite(msg);
          sprintf(buffer,"edgeVec1[%d] to sideVec0[%d]\n",i,j);
          UserWrite(buffer);
          ReturnCode = GM_ERROR;
        }
        else SETCUSED(theCon,1);
      }
  }

  /* check edge elem connection */
  if (ActDepth <= ConDepth[MatrixType[EDGEVECTOR][ELEMVECTOR]])
  {
    for (i=0; i<edgeCnt0; i++)
      for (j=0; j<elemCnt1; j++)
      {
        theCon = GetConnection(edgeVec0[i],elemVec1[j]);
        if (theCon==NULL)
        {
          UserWrite(msg);
          sprintf(buffer,"edgeVec0[%d] to elemVec1[%d]\n",i,j);
          UserWrite(buffer);
          ReturnCode = GM_ERROR;
        }
        else SETCUSED(theCon,1);
      }
    for (i=0; i<edgeCnt1; i++)
      for (j=0; j<elemCnt0; j++)
      {
        theCon = GetConnection(edgeVec1[i],elemVec0[j]);
        if (theCon==NULL)
        {
          UserWrite(msg);
          sprintf(buffer,"edgeVec1[%d] to elemVec0[%d]\n",i,j);
          UserWrite(buffer);
          ReturnCode = GM_ERROR;
        }
        else SETCUSED(theCon,1);
      }
  }

  /* check side side connection */
  if ((DIM==3)&&(ActDepth <= ConDepth[MatrixType[SIDEVECTOR][SIDEVECTOR]]))
  {
    for (i=0; i<sideCnt0; i++)
      for (j=0; j<sideCnt1; j++)
      {
        theCon = GetConnection(sideVec0[i],sideVec1[j]);
        if (theCon==NULL)
        {
          UserWrite(msg);
          sprintf(buffer,"sideVec0[%d] to sideVec1[%d]\n",i,j);
          UserWrite(buffer);
          ReturnCode = GM_ERROR;
        }
        else SETCUSED(theCon,1);
      }
  }

  /* check side elem connection */
  if ((DIM==3)&&(ActDepth <= ConDepth[MatrixType[SIDEVECTOR][ELEMVECTOR]]))
  {
    for (i=0; i<sideCnt0; i++)
      for (j=0; j<elemCnt1; j++)
      {
        theCon = GetConnection(sideVec0[i],elemVec1[j]);
        if (theCon==NULL)
        {
          UserWrite(msg);
          sprintf(buffer,"sideVec0[%d] to elemVec1[%d]\n",i,j);
          UserWrite(buffer);
          ReturnCode = GM_ERROR;
        }
        else SETCUSED(theCon,1);
      }
    for (i=0; i<sideCnt1; i++)
      for (j=0; j<elemCnt0; j++)
      {
        theCon = GetConnection(sideVec1[i],elemVec0[j]);
        if (theCon==NULL)
        {
          UserWrite(msg);
          sprintf(buffer,"sideVec1[%d] to elemVec0[%d]\n",i,j);
          UserWrite(buffer);
          ReturnCode = GM_ERROR;
        }
        else SETCUSED(theCon,1);
      }
  }

  /* check elem elem connection */
  if (ActDepth <= ConDepth[MatrixType[ELEMVECTOR][ELEMVECTOR]])
  {
    for (i=0; i<elemCnt0; i++)
      for (j=0; j<elemCnt1; j++)
      {
        theCon = GetConnection(elemVec0[i],elemVec1[j]);
        if (theCon==NULL)
        {
          UserWrite(msg);
          sprintf(buffer,"elemVec0[%d] to elemVec1[%d]\n",i,j);
          UserWrite(buffer);
          ReturnCode = GM_ERROR;
        }
        else SETCUSED(theCon,1);
      }
  }

  return(ReturnCode);
}


static ELEMENT *CheckNeighborhood (ELEMENT *theElement, ELEMENT *centerElement, INT *ConDepth, INT ActDepth, INT MaxDepth)
{
  int i;

  /* is anything to do ? */
  if (theElement==NULL) return (NULL);

  /* action */
  if (ActDepth>=0)
    if (ElementElementCheck(centerElement,theElement,ActDepth,ConDepth))
      return (centerElement);

  /* call all neighbors recursively */
  if (ActDepth<MaxDepth)
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      return(CheckNeighborhood(NBELEM(theElement,i),centerElement,ConDepth,ActDepth+1,MaxDepth));

  return (NULL);
}
/****************************************************************************/
/*D
   ElementCheckConnection - Check connection of the element

   SYNOPSIS:
   ELEMENT *ElementCheckConnection (GRID *theGrid, ELEMENT *theElement);

   PARAMETERS:
   .  theGrid - pointer to grid
   .  theElement - pointer to element

   DESCRIPTION:
   This function checks all connections of the given element.

   RETURN VALUE:
   ELEMENT *
   .n   pointer to  ELEMENT
   .n   NULL if ok
   .n   != NULL if error occured in that connection.
   D*/
/****************************************************************************/
ELEMENT *ElementCheckConnection (GRID *theGrid, ELEMENT *theElement)
{
  FORMAT *theFormat;
  MULTIGRID *theMG;
  INT MaxDepth;
  INT *ConDepth;

  /* set pointers */
  theMG = theGrid->mg;
  theFormat = theMG->theFormat;
  MaxDepth = theFormat->MaxConnectionDepth;
  ConDepth = theFormat->ConnectionDepth;

  /* call elements recursivly */
  return(CheckNeighborhood (theElement,theElement,ConDepth,0,MaxDepth));
}

/****************************************************************************/
/*D
   CheckConnections - Check if connections are correctly allocated

   SYNOPSIS:
   INT CheckConnections (GRID *theGrid);

   PARAMETERS:
   .  theGrid -  grid level to check

   DESCRIPTION:
   This function checks if connections are correctly allocated.

   RETURN VALUE:
   INT
   .n     GM_OK if ok
   .n     GM_ERROR	if error occured.
   D*/
/****************************************************************************/

INT CheckConnections (GRID *theGrid)
{
  ELEMENT *theElement;
  INT errors;

  errors = 0;

  for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
    if (ElementCheckConnection(theGrid,theElement)!=NULL) errors++;

  if (errors>0) return(GM_ERROR);else return(GM_OK);
}

/****************************************************************************/
/*D
   VectorInElement -  Decide whether a vector corresponds to an element or not
   SYNOPSIS:
   INT VectorInElement (ELEMENT *theElement, VECTOR *theVector);

   PARAMETERS:
   .  theElement - pointer to element
   .  theVector - pointer to a vector

   DESCRIPTION:
   This function decides whether a given vector belongs to the given element, or
   one of its sides, edges or nodes.

   RETURN VALUE:
   INT
   .n    0 if does not correspond
   .n    1 if does correspond.
   D*/
/****************************************************************************/

INT VectorInElement (ELEMENT *theElement, VECTOR *theVector)
{
  INT i;
  VECTOR *vList[20];
  INT cnt;

  GetVectorsOfElement(theElement,&cnt,vList);
  for (i=0; i<cnt; i++)
    if (vList[i]==theVector) return(1);

  if (DIM==3)
  {
    GetVectorsOfSides(theElement,&cnt,vList);
    for (i=0; i<cnt; i++)
      if (vList[i]==theVector) return(1);
  }

  GetVectorsOfEdges(theElement,&cnt,vList);
  for (i=0; i<cnt; i++)
    if (vList[i]==theVector) return(1);

  GetVectorsOfNodes(theElement,&cnt,vList);
  for (i=0; i<cnt; i++)
    if (vList[i]==theVector) return(1);

  return (0);
}

/****************************************************************************/
/*D
   VectorPosition - Calc coordinate position of vector

   SYNOPSIS:
   INT VectorPosition (VECTOR *theVector, COORD *position);

   PARAMETERS:
   .  theVector - a given vector
   .  position - array to be filled

   DESCRIPTION:
   This function calcs physical position of vector. For edge vectors the
   midpoint is returned, and for sides and elements the center of mass.

   RETURN VALUE:
   INT
   .n     0 if ok
   .n     1 if error occured.
   D*/
/****************************************************************************/

INT VectorPosition (VECTOR *theVector, COORD *position)
{
  INT i, j;
  EDGE *theEdge;
  ELEMENT *theElement;
        #ifdef __THREEDIM__
  INT theSide;
        #endif

  switch(VTYPE(theVector))
  {
  case (NODEVECTOR) :
    for (i=0; i<DIM; i++)
      position[i] = CVECT(MYVERTEX((NODE*)VOBJECT(theVector)))[i];
    return (0);

  case (EDGEVECTOR) :
    theEdge = (EDGE*)VOBJECT(theVector);
    for (i=0; i<DIM; i++)
      position[i] = 0.5*(CVECT(MYVERTEX(NBNODE(LINK0(theEdge))))[i] +
                         CVECT(MYVERTEX(NBNODE(LINK1(theEdge))))[i]   );
    return (0);
                #ifdef __THREEDIM__
  case (SIDEVECTOR) :
    theElement = (ELEMENT *)VOBJECT(theVector);
    theSide = VECTORSIDE(theVector);
    for (i=0; i<DIM; i++)
    {
      position[i] = 0.0;
      for(j=0; j<CORNERS_OF_SIDE(theElement,theSide); j++)
        position[i] += CVECT(MYVERTEX(CORNER(theElement,CORNER_OF_SIDE(theElement,theSide,j))))[i];
      position[i] /= CORNERS_OF_SIDE(theElement,theSide);
    }
    return (0);
                #endif
  case (ELEMVECTOR) :
    theElement = (ELEMENT *) VOBJECT(theVector);
    for (i=0; i<DIM; i++)
    {
      position[i] = 0.0;
      for(j=0; j<CORNERS_OF_ELEM(theElement); j++)
        position[i] += CVECT(MYVERTEX(CORNER(theElement,j)))[i];
      position[i] /= CORNERS_OF_ELEM(theElement);
    }
    return (0);
  }

  return (GM_ERROR);
}


/****************************************************************************/
/*D
   SeedVectorClasses -  Initialize vector classes

   SYNOPSIS:
   INT SeedVectorClasses (ELEMENT *theElement);

   PARAMETERS:
   .  theElement - given element

   DESCRIPTION:
   Initialize vector class in all vectors associated with given element with 3.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
   D*/
/****************************************************************************/

INT SeedVectorClasses (ELEMENT *theElement)
{
  INT i;
  VECTOR *vList[20];
  INT cnt;

  GetVectorsOfElement(theElement,&cnt,vList);
  for (i=0; i<cnt; i++) SETVCLASS(vList[i],3);
  if (DIM==3)
  {
    GetVectorsOfSides(theElement,&cnt,vList);
    for (i=0; i<cnt; i++) SETVCLASS(vList[i],3);
  }
  GetVectorsOfEdges(theElement,&cnt,vList);
  for (i=0; i<cnt; i++) SETVCLASS(vList[i],3);
  GetVectorsOfNodes(theElement,&cnt,vList);
  for (i=0; i<cnt; i++) SETVCLASS(vList[i],3);
  return (0);
}
/****************************************************************************/
/*D
   ClearVectorClasses - Reset vector classes

   SYNOPSIS:
   INT ClearVectorClasses (GRID *theGrid);

   PARAMETERS:
   .  theGrid - pointer to grid

   DESCRIPTION:
   Reset all vector classes in all vectors of given grid to 0.

   RETURN VALUE:
   INT
   .n     0 if ok
   .n     1 if error occured.
   D*/
/****************************************************************************/

INT ClearVectorClasses (GRID *theGrid)
{
  VECTOR *theVector;

  /* reset class of each vector to 0 */
  for (theVector=theGrid->firstVector; theVector!=NULL; theVector=SUCCVC(theVector))
    SETVCLASS(theVector,0);

  return(0);
}
/****************************************************************************/
/*D
   PropagateVectorClasses - Compute vector classes after initialization

   SYNOPSIS:
   INT PropagateVectorClasses (GRID *theGrid);

   PARAMETERS:
   .  theGrid - pointer to grid

   DESCRIPTION:
   After vector classes have been reset and initialized, this function
   now computes the class 2 and class 1 vectors.

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if error occured
   D*/
/****************************************************************************/

INT PropagateVectorClasses (GRID *theGrid)
{
  VECTOR *theVector;
  MATRIX *theMatrix;

  /* set vector classes in the algebraic neighborhood to 2 */
  for (theVector=theGrid->firstVector; theVector!=NULL; theVector=SUCCVC(theVector))
    if ((VCLASS(theVector)==3)&&(VSTART(theVector)!=NULL))
      for (theMatrix=MNEXT(VSTART(theVector)); theMatrix!=NULL; theMatrix=MNEXT(theMatrix))
        if ((VCLASS(MDEST(theMatrix))<3)&&
            (CEXTRA(MMYCON(theMatrix))!=1))
          SETVCLASS(MDEST(theMatrix),2);

  /* set vector classes in the algebraic neighborhood to 1 */
  for (theVector=theGrid->firstVector; theVector!=NULL; theVector=SUCCVC(theVector))
    if ((VCLASS(theVector)==2)&&(VSTART(theVector)!=NULL))
      for (theMatrix=MNEXT(VSTART(theVector)); theMatrix!=NULL; theMatrix=MNEXT(theMatrix))
        if ((VCLASS(MDEST(theMatrix))<2)&&
            (CEXTRA(MMYCON(theMatrix))!=1))
          SETVCLASS(MDEST(theMatrix),1);

  return(0);
}

/****************************************************************************/
/*D
   ClearNextVectorClasses - Reset class of the vectors on the next level

   SYNOPSIS:
   INT ClearNextVectorClasses (GRID *theGrid);

   PARAMETERS:
   .  theGrid - pointer to grid

   DESCRIPTION:
   This function clears VNCLASS flag in all vectors. This is the first step to
   compute the class of the dofs on the *NEXT* level, which
   is also the basis for determining copies.

   RETURN VALUE:
   INT
   .n     0 if ok
   .n     1 if error occured.
   D*/
/****************************************************************************/

INT ClearNextVectorClasses (GRID *theGrid)
{
  VECTOR *theVector;

  /* reset class of each vector to 0 */
  for (theVector=theGrid->firstVector; theVector!=NULL; theVector=SUCCVC(theVector))
    SETVNCLASS(theVector,0);

  /* now the refinement algorithm will initialize the class 3 vectors */
  /* on the *NEXT* level.                                                                                       */
  return(0);
}

/****************************************************************************/
/*D
   SeedNextVectorClasses - Set 'VNCLASS' in all vectors associated with element

   SYNOPSIS:
   INT SeedNextVectorClasses (ELEMENT *theElement);

   PARAMETERS:
   .  theElement - pointer to element

   DESCRIPTION:
   Set 'VNCLASS' in all vectors associated with the element to 3.

   RETURN VALUE:
   INT
   .n     0 if ok
   .n     1 if error occured.
   D*/
/****************************************************************************/

INT SeedNextVectorClasses (ELEMENT *theElement)
{
  INT i;
  VECTOR *vList[20];
  INT cnt;

  GetVectorsOfElement(theElement,&cnt,vList);
  for (i=0; i<cnt; i++) SETVNCLASS(vList[i],3);
  if (DIM==3)
  {
    GetVectorsOfSides(theElement,&cnt,vList);
    for (i=0; i<cnt; i++) SETVNCLASS(vList[i],3);
  }
  GetVectorsOfEdges(theElement,&cnt,vList);
  for (i=0; i<cnt; i++) SETVNCLASS(vList[i],3);
  GetVectorsOfNodes(theElement,&cnt,vList);
  for (i=0; i<cnt; i++) SETVNCLASS(vList[i],3);
  return (0);
}


/****************************************************************************/
/*D
   PropagateNextVectorClasses - Compute 'VNCLASS' in all vectors of a grid level

   SYNOPSIS:
   INT PropagateNextVectorClasses (GRID *theGrid);

   PARAMETERS:
   .  theGrid - pointer to grid

   DESCRIPTION:
   Computes values of 'VNCLASS' field in all vectors after seed.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured
   D*/
/****************************************************************************/

INT PropagateNextVectorClasses (GRID *theGrid)
{
  VECTOR *theVector;
  MATRIX *theMatrix;

  /* set vector classes in the algebraic neighborhood to 2 */
  for (theVector=theGrid->firstVector; theVector!=NULL; theVector=SUCCVC(theVector))
    if ((VNCLASS(theVector)==3)&&(VSTART(theVector)!=NULL))
      for (theMatrix=MNEXT(VSTART(theVector)); theMatrix!=NULL; theMatrix=MNEXT(theMatrix))
        if ((VNCLASS(MDEST(theMatrix))<3)
            &&(CEXTRA(MMYCON(theMatrix))!=1))
          SETVNCLASS(MDEST(theMatrix),2);

  /* set vector classes in the algebraic neighborhood to 1 */
  for (theVector=theGrid->firstVector; theVector!=NULL; theVector=SUCCVC(theVector))
    if ((VNCLASS(theVector)==2)&&(VSTART(theVector)!=NULL))
      for (theMatrix=MNEXT(VSTART(theVector)); theMatrix!=NULL; theMatrix=MNEXT(theMatrix))
        if ((VNCLASS(MDEST(theMatrix))<2)
            &&(CEXTRA(MMYCON(theMatrix)) !=1))
          SETVNCLASS(MDEST(theMatrix),1);

  return(0);
}

/****************************************************************************/
/*D
   MaxNextVectorClass - Returns highest vector class of a dof on next level

   SYNOPSIS:
   INT MaxNextVectorClass (ELEMENT *theElement);

   PARAMETERS:
   .  theElement - pointer to element

   DECRIPTION:
   This function returns highest 'VNCLASS' of a vector associated with the
   element.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
   D*/
/****************************************************************************/

INT MaxNextVectorClass (ELEMENT *theElement)
{
  INT i,m;
  VECTOR *vList[20];
  INT cnt;

  m = 0;
  GetVectorsOfElement(theElement,&cnt,vList);
  for (i=0; i<cnt; i++) m = MAX(m,VNCLASS(vList[i]));
  if (DIM==3)
  {
    GetVectorsOfSides(theElement,&cnt,vList);
    for (i=0; i<cnt; i++) m = MAX(m,VNCLASS(vList[i]));
  }
  GetVectorsOfEdges(theElement,&cnt,vList);
  for (i=0; i<cnt; i++) m = MAX(m,VNCLASS(vList[i]));
  GetVectorsOfNodes(theElement,&cnt,vList);
  for (i=0; i<cnt; i++) m = MAX(m,VNCLASS(vList[i]));

  return (m);
}

/****************************************************************************/
/*                                                                          */
/* Function:  LexCompare													*/
/*                                                                          */
/* Purpose:   defines a relation for lexicographic ordering                 */
/*                                                                          */
/* Input:                                                                                                                               */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

static int LexCompare (VECTOR **pvec1, VECTOR **pvec2)
{
  COORD_VECTOR pv1,pv2;
  COORD diff[DIM];

  if (SkipV)
  {
    if (VECSKIP(*pvec1) && !VECSKIP(*pvec2))
      if (SkipV==GM_PUT_AT_BEGIN)
        return (-1);
      else
        return ( 1);

    if (VECSKIP(*pvec2) && !VECSKIP(*pvec1))
      if (SkipV==GM_PUT_AT_BEGIN)
        return ( 1);
      else
        return (-1);
  }
  VectorPosition(*pvec1,pv1);
  VectorPosition(*pvec2,pv2);

  V_DIM_SUBTRACT(pv2,pv1,diff);
  V_DIM_SCALE(InvMeshSize,diff);

  if (fabs(diff[Order[DIM-1]])<ORDERRES)
  {
                #ifdef __THREEDIM__
    if (fabs(diff[Order[DIM-2]])<ORDERRES)
    {
      if (diff[Order[DIM-3]]>0.0) return (-Sign[DIM-3]);
      else return ( Sign[DIM-3]);
    }
    else
                #endif
    if (diff[Order[DIM-2]]>0.0) return (-Sign[DIM-2]);
    else return ( Sign[DIM-2]);
  }
  else
  {
    if (diff[Order[DIM-1]]>0.0) return (-Sign[DIM-1]);
    else return ( Sign[DIM-1]);
  }
}

static int MatrixCompare (MATRIX **MatHandle1, MATRIX **MatHandle2)
{
  INT IND1,IND2;;

  IND1 = INDEX(MDEST(*MatHandle1));
  IND2 = INDEX(MDEST(*MatHandle2));

  if (IND1>IND2)
    return ( 1);
  else
    return (-1);
}


/****************************************************************************/
/*D
   LexOrderVectorsInGrid - order vectors lexicographically

   SYNOPSIS:
   INT LexOrderVectorsInGrid (GRID *theGrid, const INT *order, const INT *sign,
                                INT SpecSkipVecs, INT AlsoOrderMatrices)

   PARAMETERS:
   .  theGrid - grid level
   .  order - hierarchie of ordering directions (x,y[,z])
   .  sign  - signs for the directions
   .  SpecSkipVecs - if TRUE: GM_PUT_AT_BEGIN or GM_PUT_AT_END of vectors with skip TRUE
   .  AlsoOrderMatrices - if TRUE order matrices in the same sense

   DESCRIPTION:
   This function orders the vectors of one level lexicographically.
   It has the complexity of qsort which is n*log(n).

   RETURN VALUE:
   INT
   .n   0 if ok
   .n   1 if error occured.
   D*/
/****************************************************************************/

INT LexOrderVectorsInGrid (GRID *theGrid, const INT *order, const INT *sign, INT SpecSkipVecs, INT AlsoOrderMatrices)
{
  MULTIGRID *theMG;
  VECTOR **table,*theVec;
  MATRIX *theMat,*MatTable[MATTABLESIZE];
  DOMAIN *theDomain;
  INT i,entries,nm;
  HEAP *theHeap;

  theMG   = MYMG(theGrid);
  entries = NVEC(theGrid);

  /* calculate the diameter of the bounding rectangle of the domain */
  theDomain = MGDOMAIN(theMG);
  if (theDomain==NULL) return (1);
  InvMeshSize = POW2(GLEVEL(theGrid)) * pow(NN(GRID_ON_LEVEL(theMG,0)),1.0/DIM) / theDomain->radius;

  /* allocate memory for the node list */
  theHeap = MGHEAP(theMG);
  Mark(theHeap,FROM_TOP);
  if ((table=GetMem(theHeap,entries*sizeof(NODE *),FROM_TOP))==NULL)
  {
    Release(theHeap,FROM_TOP);
    PrintErrorMessage('E',"LexOrderVectorsInGrid","could not allocate memory from the MGHeap");
    return (2);
  }

  /* fill array of pointers to nodes */
  entries = 0;
  for (theVec=FIRSTVECTOR(theGrid); theVec!=NULL; theVec=SUCCVC(theVec))
    table[entries++] = theVec;

  /* sort array of pointers */
  Order = order;
  Sign  = sign;
  SkipV = SpecSkipVecs;
  qsort(table,entries,sizeof(*table),(int (*)(const void *, const void *))LexCompare);

  /* reorder double linked list */
  for (i=0; i<entries-1; i++)
    SUCCVC(table[i]) = table[i+1];

  for (i=1; i<entries; i++)
  {
    INDEX(table[i]) = i;
    PREDVC(table[i]) = table[i-1];
  }
  INDEX(table[0]) = 0;

  SUCCVC(table[entries-1])  = NULL;
  PREDVC(table[0]) = NULL;

  FIRSTVECTOR(theGrid) = table[0];
  LASTVECTOR(theGrid)  = table[entries-1];


  Release(theHeap,FROM_TOP);

  if (!AlsoOrderMatrices)
    return (0);

  /* now we also order the matrices of each vector the same way (just using the new INDICES) */
  /* but leaving the diag at its place of course */
  for (theVec=FIRSTVECTOR(theGrid); theVec!=NULL; theVec=SUCCVC(theVec))
  {
    /* fill array for qsort */
    for (nm=0, theMat=VSTART(theVec); theMat!=NULL; theMat=MNEXT(theMat))
    {
      if (nm>=MATTABLESIZE)
        return (1);

      MatTable[nm++] = theMat;
    }
    qsort(MatTable+1,nm-1,sizeof(LINK*),(int (*)(const void *, const void *))MatrixCompare);

    /* establish pointer connections */
    MNEXT(MatTable[--nm]) = NULL;
    while (nm>0)
    {
      MNEXT(MatTable[nm-1]) = MatTable[nm];
      --nm;
    }
    VSTART(theVec) = MatTable[0];
  }

  return (0);
}

/****************************************************************************/
/*D
   CreateFindCutProc - Create a new find cut procedure in environement

   SYNOPSIS:
   ALG_DEP *CreateFindCutProc (char *name, FindCutProcPtr FindCutProc);

   PARAMETERS:
   .  name - name
   .  FindCutProc -  the find cut procedure

   DESCRIPTION:
   This function creates a new find cut dependency in environement.

   RETURN VALUE:
   INT
   .n   0 if ok
   .n   1 if error occured.
   D*/
/****************************************************************************/

FIND_CUT *CreateFindCutProc (char *name, FindCutProcPtr FindCutProc)
{
  FIND_CUT *newFindCut;

  if (ChangeEnvDir("/FindCut")==NULL)
  {
    UserWrite("cannot change to dir '/FindCut'\n");
    return(NULL);
  }
  newFindCut = (FIND_CUT *) MakeEnvItem (name,theFindCutVarID,sizeof(FIND_CUT));
  if (newFindCut==NULL) return(NULL);

  newFindCut->FindCutProc = FindCutProc;

  return (newFindCut);
}

/****************************************************************************/
/*D
   CreateAlgebraicDependency - Create a new algebraic dependency in environement

   SYNOPSIS:
   ALG_DEP *CreateAlgebraicDependency (char *name,
   DependencyProcPtr DependencyProc);

   PARAMETERS:
   .  name - name
   .  DependencyProc -  the dependency procedure

   DESCRIPTION:
   This function creates a new algebraic dependency in environement.

   RETURN VALUE:
   INT
   .n   0 if ok
   .n   1 if error occured.
   D*/
/****************************************************************************/

ALG_DEP *CreateAlgebraicDependency (char *name, DependencyProcPtr DependencyProc)
{
  ALG_DEP *newAlgDep;

  if (ChangeEnvDir("/Alg Dep")==NULL)
  {
    UserWrite("cannot change to dir '/Alg Dep'\n");
    return(NULL);
  }
  newAlgDep = (ALG_DEP *) MakeEnvItem (name,theAlgDepVarID,sizeof(ALG_DEP));
  if (newAlgDep==NULL) return(NULL);

  newAlgDep->DependencyProc = DependencyProc;

  return (newAlgDep);
}

/****************************************************************************/
/*
   FeedbackVertexVectors - put `every` vector '!VCUSED' into the new order list

   SYNOPSIS:
   static INT FeedbackVertexVectors (GRID *theGrid, VECTOR **CutVectors,
   INT *nb);

   PARAMETERS:
   .  theGrid - pointer to grid
   .  CutVectors -
   .  nb -

   DESCRIPTION:
   This function can be used as a find-cut-procedure for the streamwise ordering
   algorithm. But it just puts `all` the remaining vectors into the new order
   list instead of removing only as much as neccessary to get rid of cyclic
   dependencies.

   It is used as default when no cut procedure is specified.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
 */
/****************************************************************************/

static VECTOR *FeedbackVertexVectors (GRID *theGrid, VECTOR *LastVector, INT *nb)
{
  VECTOR *theVector;

  /* push all remaining vectors */
  *nb = 0;
  for (theVector=FIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
    if (!VCUSED(theVector))
    {
      (*nb)++;
      PREDVC(LastVector) = theVector;
      LastVector = theVector;
      SETVCUSED(theVector,1);
    }
  return (LastVector);
}

/****************************************************************************/
/*
   OrderMatrices - reorder the matrix list of a vector in a circular order (2D only)

   SYNOPSIS:
   static INT OrderMatrices (VECTOR *vec, INT Sense)

   PARAMETERS:
   .  vec - order matrix list of this vector
   .  sense - MATHPOS or MATHNEG

   DESCRIPTION:
   This function reorders the matrix list of a vector in a circular order (2D only).

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
 */
/****************************************************************************/

#ifdef __TWODIM__

static COORD_VECTOR mypos;
static INT sense;

static INT SensCompare (MATRIX **MatHandle1, MATRIX **MatHandle2)
{
  COORD dx1,dy1,dx2,dy2;
  COORD_VECTOR pos1,pos2;

  VectorPosition(MDEST(*MatHandle1),pos1);
  VectorPosition(MDEST(*MatHandle2),pos2);
  dx1 = pos1[0] - mypos[0];
  dy1 = pos1[1] - mypos[1];
  dx2 = pos2[0] - mypos[0];
  dy2 = pos2[1] - mypos[1];

  if (dy1>=0)
  {
    if (dy2<0)
      return (-sense);

    if ((dy1==0) && (dy2==0))
    {
      if (dx1>dx2)
        return (-sense);
      else
        return (sense);
    }

    /* both neighbours in upper half plane */
    if ((dy1*dx2-dx1*dy2)<0)
      return (-sense);
    else
      return (sense);
  }
  else
  {
    if (dy2>=0)
      return (sense);

    /* both neighbours in lower half plane */
    if ((dy1*dx2-dx1*dy2)<0)
      return (-sense);
    else
      return (sense);
  }
}

static INT OrderMatrices (VECTOR *vec, INT Sense)
{
  VECTOR *nbv;
  MATRIX *mat,*MatList[MATTABLESIZE];
  INT i,flag,condition1,condition2,start,nm;

  /* fill matrices without diag into list */
  for (nm=0, mat=MNEXT(VSTART(vec)); mat!=NULL; mat=MNEXT(mat))
  {
    if (nm>=MATTABLESIZE)
      return (1);

    MatList[nm++] = mat;
  }
  VectorPosition(vec,mypos);

  sense = Sense;
  qsort(MatList,nm,sizeof(MATRIX*),(int (*)(const void *, const void *))SensCompare);

  /* find 'begin' */
  flag = FALSE;
  for (start=0; start<nm; start++)
  {
    nbv = MDEST(MatList[start]);
    condition1 = ((VCLASS(nbv)<ACTIVE_CLASS) || VCUSED(nbv));
    condition2 = (condition1 || (OBJT(MYVERTEX((NODE*)VOBJECT(nbv)))==BVOBJ));
    if (flag && !condition1)
      break;
    else if (condition2)
      flag = TRUE;
  }

  /* establish pointer connections */
  for (i=start; i<nm+start-1; i++)
    MNEXT(MatList[i%nm]) = MatList[(i+1)%nm];
  MNEXT(MatList[i%nm]) = NULL;
  MNEXT(VSTART(vec)) = MatList[start%nm];

  return (0);
}

#endif

/****************************************************************************/
/*
   ShellOrderVectors	- Reorder double linked vector list by a shell algorithm

   SYNOPSIS:
   INT ShellOrderVectors (GRID *theGrid, VECTOR *seed)

   PARAMETERS:
   .  theGrid - pointer to grid
   .  seed - start at this vector

   DESCRIPTION:
   This function reorders double linked vector list by a shell algorithm.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
 */
/****************************************************************************/

INT ShellOrderVectors (GRID *theGrid, VECTOR *seed)
{
  VECTOR handle;
  VECTOR *next_out,*last_in;
  VECTOR *theVector,*theNbVector,*succVector,*predVector;
  MATRIX *theMatrix;
  INT i,index,found;

  /****************************************************************************/
  /*	init																	*/
  /****************************************************************************/

  /* init VCUSED */
  found = FALSE;
  for (theVector=FIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
  {
    SETVCUSED(theVector,0);
    if (theVector==seed) found = TRUE;
  }
  if (!found)
    return (1);

  /* in the sequel we use (and therefore destroy) the PREDVC-list for book keeping */

  /* init pointers and push seed */
  next_out = last_in = &handle;
  PREDVC(last_in) = seed;
  last_in = seed;
  SETVCUSED(seed,1);
  PREDVC(last_in) = NULL;
  VINDEX(seed) = index = 1;

  for (next_out=PREDVC(next_out); next_out!=NULL; next_out=PREDVC(next_out))
  {
                #ifdef __TWODIM__
    OrderMatrices(next_out,MATHPOS);
                #endif

    /* push neighbours */
    for (theMatrix=MNEXT(VSTART(next_out)); theMatrix!=NULL; theMatrix=MNEXT(theMatrix))
    {
      if (CEXTRA(MMYCON(theMatrix))) continue;

      theNbVector = MDEST(theMatrix);

      if (VCUSED(theNbVector)) continue;

      PREDVC(last_in) = theNbVector;
      last_in = theNbVector;
      PREDVC(last_in) = NULL;
      SETVCUSED(theNbVector,1);
      VINDEX(theNbVector) = ++index;
    }
  }

  /* insert list one-by-one to LASTVECTOR list of grid */
  LASTVECTOR(theGrid) = NULL;
  for (theVector=PREDVC(&handle); theVector!=NULL; theVector=predVector)
  {
    predVector                      = PREDVC(theVector);
    PREDVC(theVector)   = LASTVECTOR(theGrid);
    LASTVECTOR(theGrid) = theVector;
  }

  /* construct SUCCVC list */
  succVector = NULL;
  for (theVector=LASTVECTOR(theGrid); theVector!=NULL; theVector=PREDVC(theVector))
  {
    SUCCVC(theVector) = succVector;
    succVector = theVector;
  }
  FIRSTVECTOR(theGrid) = succVector;
  PREDVC(succVector)   = NULL;

  /* check # members of succ list */
  i=0;
  for (theVector=FIRSTVECTOR(theGrid); theVector!= NULL; theVector=SUCCVC(theVector)) i++;
  if (NVEC(theGrid) != i)
  {
    UserWrite("vectorstructure corrupted\n");
    return (1);
  }

  /* check # members of pred list */
  i=0;
  for (theVector=LASTVECTOR(theGrid); theVector!= NULL; theVector=PREDVC(theVector)) i++;
  if (NVEC(theGrid) != i)
  {
    UserWrite("vectorstructure corrupted\n");
    return (1);
  }

  return (0);
}

/****************************************************************************/
/*
   OrderVectorAlgebraic	- Reorder double linked node list by a streamline ordering

   SYNOPSIS:
   static INT OrderVectorAlgebraic (GRID *theGrid, INT mode, INT putSkipFirst, INT skipPat);

   PARAMETERS:
   .  theGrid - pointer to grid
   .  mode
   .  putSkipFirst - if TRUE put vectors with a skip pattern larger than SkipPat to begin of list
   .  skipPat - s.a.

   DESCRIPTION:
   This function reorders double linked node list by a streamline ordering.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
 */
/****************************************************************************/

static INT OrderVectorAlgebraic (GRID *theGrid, INT mode, INT putSkipFirst, INT skipPat)
{
  VECTOR FIRST_handle,CUT_handle,LAST_handle;
  VECTOR *FIRST_next_out,*FIRST_last_in;
  VECTOR *LAST_next_out,*LAST_last_in;
  VECTOR *CUT_next_out,*CUT_last_in,*CUT_begin;
  VECTOR *theVector,*theNbVector,*succVector,*predVector;
  MATRIX *theMatrix;
  INT i,k,cycle;
  INT nFIRST,nCUT,nLAST;
  INT up, down;

  /****************************************************************************/
  /*	init																	*/
  /****************************************************************************/

  /* init USED, N_INFLOW and N_OUTFLOW */
  for (theVector=FIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
  {
    /* reset used flag (indicating the vector hasn't yet its new place in the list) */
    SETVCUSED(theVector,0);

    /* count upward and downward matrices */
    up = down = 0;
    if (VCLASS(theVector)==ACTIVE_CLASS)
      for (theMatrix=MNEXT(VSTART(theVector)); theMatrix!=NULL; theMatrix=MNEXT(theMatrix))
      {
        if (MUP(theMatrix)) up++;
        if (MDOWN(theMatrix)) down++;
      }
    SETVUP(theVector,up);
    SETVDOWN(theVector,down);
  }

  /* in the sequel we use (and therefore destroy) the PREDVC-list for book keeping */

  cycle = 0;

  /* init pointers and set the first FIRST and LAST set */
  FIRST_next_out = FIRST_last_in = &FIRST_handle;
  nFIRST = 0;
  LAST_next_out  = LAST_last_in  = &LAST_handle;
  nLAST  = 0;
  CUT_next_out   = CUT_last_in   = &CUT_handle;
  nCUT   = 0;
  for (theVector=FIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
  {
    if (((putSkipFirst) && (VECSKIP(theVector) & skipPat == skipPat)) ||
        (VUP(theVector)==0))
    {
      nFIRST++;
      /* append to FIRST list */
      PREDVC(FIRST_last_in) = theVector;
      FIRST_last_in = theVector;
      SETVCUSED(theVector,1);
      VINDEX(theVector) = 0;
    }
    else if (VDOWN(theVector)==0)
    {
      nLAST++;
      /* append to last list */
      PREDVC(LAST_last_in) = theVector;
      LAST_last_in = theVector;
      SETVCUSED(theVector,1);
      VINDEX(theVector) = 1;
    }
  }
  PREDVC(FIRST_last_in) = PREDVC(LAST_last_in) = NULL;

  UserWriteF("init: #first%d=%10d\n",(int)cycle,(int)nFIRST);
  UserWriteF("init: #last%d =%10d\n",(int)cycle,(int)nLAST);

  do
  {
    cycle++;

    /****************************************************************************/
    /*	find next FIRST-set in vectors not used                                                                 */
    /****************************************************************************/

    nFIRST = nLAST = 0;

    for (FIRST_next_out=PREDVC(FIRST_next_out); FIRST_next_out!=NULL; FIRST_next_out=PREDVC(FIRST_next_out))
    {
                        #ifdef __TWODIM__
      OrderMatrices(FIRST_next_out,MATHPOS);
                        #endif

      for (theMatrix=MNEXT(VSTART(FIRST_next_out)); theMatrix!=NULL; theMatrix=MNEXT(theMatrix))
      {
        theNbVector = MDEST(theMatrix);

        if (VCUSED(theNbVector)) continue;

        if (MDOWN(theMatrix))
        {
          k = VUP(theNbVector);
          assert(k>0);                                                  /* if 0 is supposed to be VCUSED already */
          SETVUP(theNbVector,--k);
          if (k==0)
          {
            /* this vector has only matrices going down */

            nFIRST++;
            /* append to FIRST list */
            PREDVC(FIRST_last_in) = theNbVector;
            FIRST_last_in = theNbVector;
            PREDVC(FIRST_last_in) = NULL;
            SETVCUSED(theNbVector,1);
            VINDEX(theNbVector) = 3*cycle;
          }
        }
        /* we also have to push LAST vectors
           this is for the cases
                        (1) vectors have been pushed because of putSkipFirst
                        (2) vectors have been pushed because VCLASS(theVector)!=ACTIVE_CLASS
                        (3) vectors have been pushed because (nCUT>0) && (mode==GM_FCFCLL)
           (the standard case remains unaffected) */
        if (MUP(theMatrix))
        {
          k = VDOWN(theNbVector);
          assert(k>0);                                                  /* if 0 is supposed to be VCUSED already */
          SETVDOWN(theNbVector,--k);
          if (k==0)
          {
            /* this vector has only matrices going up */

            nLAST++;
            /* append to last list */
            PREDVC(LAST_last_in) = theNbVector;
            LAST_last_in = theNbVector;
            PREDVC(LAST_last_in) = NULL;
            SETVCUSED(theNbVector,1);
            VINDEX(theNbVector) = 3*cycle+1;
          }
        }
      }
    }
    FIRST_next_out = FIRST_last_in;

    UserWriteF("#first%d=%10d\n",(int)cycle,(int)nFIRST);

    /********************************************************************/
    /*	find next LAST-set in vectors not used			                        */
    /********************************************************************/

    for (LAST_next_out=PREDVC(LAST_next_out); LAST_next_out!=NULL; LAST_next_out=PREDVC(LAST_next_out))
    {
                        #ifdef __TWODIM__
      OrderMatrices(LAST_next_out,MATHNEG);
                        #endif

      for (theMatrix=MNEXT(VSTART(LAST_next_out)); theMatrix!=NULL; theMatrix=MNEXT(theMatrix))
      {
        theNbVector = MDEST(theMatrix);
        if (!VCUSED(theNbVector) && MUP(theMatrix))
        {
          k = VDOWN(theNbVector);
          assert(k>0);                                                  /* if 0 is supposed to be VCUSED already */
          SETVDOWN(theNbVector,--k);
          if (k==0)
          {
            /* this vector has only matrices going up */

            nLAST++;
            /* append to last list */
            PREDVC(LAST_last_in) = theNbVector;
            LAST_last_in = theNbVector;
            PREDVC(LAST_last_in) = NULL;
            SETVCUSED(theNbVector,1);
            VINDEX(theNbVector) = 3*cycle+1;
          }
        }
      }
    }
    LAST_next_out = LAST_last_in;

    UserWriteF("#last%d =%10d\n",(int)cycle,(int)nLAST);

    /****************************************************************************/
    /*	get CUT (or Feedback Vertex)-set and do what needs to be done			*/
    /****************************************************************************/

    if (mode==GM_FCFCLL)
    {
      CUT_begin = FIRST_last_in;

      FIRST_last_in = (*FindCutSet)(theGrid,FIRST_last_in,&nCUT);
      if (FIRST_last_in==NULL)
        nCUT = 0;
      else
        PREDVC(FIRST_last_in) = NULL;

      /* set index */
      for (theVector=PREDVC(CUT_begin); theVector!=NULL; theVector=PREDVC(theVector))
        VINDEX(theVector) = 3*cycle+2;

      UserWriteF("#cut%d  =%10d\n",(int)cycle,(int)nCUT);
    }
    else
    {
      CUT_begin = CUT_last_in;

      CUT_last_in = (*FindCutSet)(theGrid,CUT_last_in,&nCUT);
      if (FIRST_last_in==NULL)
        nCUT = 0;
      else
        PREDVC(CUT_last_in) = NULL;

      /* set index */
      for (theVector=PREDVC(CUT_begin); theVector!=NULL; theVector=PREDVC(theVector))
        VINDEX(theVector) = 3*cycle+2;

      UserWriteF("#cut$d  =%10d\n",(int)cycle,(int)nCUT);

      if (nCUT==0) continue;

      /* find FIRST and LAST-vectors */
      if (CUT_next_out==&CUT_handle) CUT_next_out = PREDVC(CUT_next_out);
      for (; CUT_next_out!=NULL; CUT_next_out=PREDVC(CUT_next_out))
        for (theMatrix=MNEXT(VSTART(CUT_next_out)); theMatrix!=NULL; theMatrix=MNEXT(theMatrix))
        {
          theNbVector = MDEST(theMatrix);
          if (!VCUSED(theNbVector))
          {
            if (MDOWN(theMatrix))
            {
              k = VUP(theNbVector);
              assert(k>0);                                                              /* if 0 is supposed to be in VectorList already */
              SETVUP(theNbVector,--k);
              if (k==0)
              {
                /* this vector has only matrices going down */

                nFIRST++;
                /* append to FIRST list */
                PREDVC(FIRST_last_in) = theNbVector;
                FIRST_last_in = theNbVector;
                SETVCUSED(theNbVector,1);
              }
            }
            else if (MUP(theMatrix))
            {
              k = VDOWN(theNbVector);
              assert(k>0);                                                              /* if 0 is supposed to be in VectorList already */
              SETVDOWN(theNbVector,--k);
              if (k==0)
              {
                /* this vector has only matrices going up */

                nLAST++;
                /* append to last list */
                PREDVC(LAST_last_in) = theNbVector;
                LAST_last_in = theNbVector;
                SETVCUSED(theNbVector,1);
              }
            }
          }
        }
      CUT_next_out = CUT_last_in;
      PREDVC(FIRST_last_in) = PREDVC(LAST_last_in) = NULL;
    }

  } while (nCUT>0);

  /* insert FIRST list one-by-one to LASTVECTOR list of grid */
  LASTVECTOR(theGrid) = NULL;
  for (theVector=PREDVC(&FIRST_handle); theVector!=NULL; theVector=predVector)
  {
    predVector                      = PREDVC(theVector);
    PREDVC(theVector)   = LASTVECTOR(theGrid);
    LASTVECTOR(theGrid) = theVector;
  }

  if (mode==GM_FFCCLL)
  {
    /* insert CUT list one-by-one to LASTVECTOR list of grid */
    for (theVector=PREDVC(&CUT_handle); theVector!=NULL; theVector=predVector)
    {
      predVector                      = PREDVC(theVector);
      PREDVC(theVector)   = LASTVECTOR(theGrid);
      LASTVECTOR(theGrid) = theVector;
    }
  }

  /* insert LAST list as a whole to LASTVECTOR list of grid */
  PREDVC(LAST_last_in) = LASTVECTOR(theGrid);
  LASTVECTOR(theGrid)  = PREDVC(&LAST_handle);

  /* construct SUCCVC list */
  succVector = NULL;
  for (theVector=LASTVECTOR(theGrid); theVector!=NULL; theVector=PREDVC(theVector))
  {
    SUCCVC(theVector) = succVector;
    succVector = theVector;
  }
  FIRSTVECTOR(theGrid) = succVector;
  PREDVC(succVector)   = NULL;

  /* check # members of succ list */
  i=0;
  for (theVector=FIRSTVECTOR(theGrid); theVector!= NULL; theVector=SUCCVC(theVector)) i++;
  if (NVEC(theGrid) != i)
  {
    UserWrite("vectorstructure corrupted\n");
    return (1);
  }

  /* check # members of pred list */
  i=0;
  for (theVector=LASTVECTOR(theGrid); theVector!= NULL; theVector=PREDVC(theVector)) i++;
  if (NVEC(theGrid) != i)
  {
    UserWrite("vectorstructure corrupted\n");
    return (1);
  }

  return (0);
}

/****************************************************************************/
/*D
   OrderVectors	-  Driver for general vector ordering

   SYNOPSIS:
   INT OrderVectors (MULTIGRID *theMG, INT levels, INT mode, INT PutSkipFirst, INT SkipPat,
                                        const char *dependency, const char *dep_options, const char *findcut);

   PARAMETERS:
   .  theMG -  multigrid to order
   .  levels -  GM_ALL_LEVELS or GM_CURRENT_LEVEL
   .  mode - GM_FCFCLL or GM_FFCCLL (see 'orderv' command)
   .  PutSkipFirst - if TRUE put vectors with a skip pattern larger than SkipPat to begin of list
   .  SkipPat - s.a.
   .  dependency - name of user defined dependency item
   .  dep_options - options for user dependency function

   DESCRIPTION:
   This function orders 'VECTOR's in a multigrid according to the dependency
   function provided.

   RETURN VALUE:
   INT
   .n     GM_OK if ok
   .n     GM_ERROR if error occured.
   D*/
/****************************************************************************/

INT OrderVectors (MULTIGRID *theMG, INT levels, INT mode, INT PutSkipFirst, INT SkipPat, const char *dependency, const char *dep_options, const char *findcut)
{
  INT i, currlevel, baselevel;
  ALG_DEP *theAlgDep;
  FIND_CUT *theFindCut;
  GRID *theGrid;
  DependencyProcPtr DependencyProc;

  /* check mode */
  if ((mode!=GM_FCFCLL)&&(mode!=GM_FFCCLL)) return(GM_ERROR);

  /* current level */
  currlevel = theMG->currentLevel;

  /* get algebraic dependency */
  theAlgDep = (ALG_DEP *) SearchEnv(dependency,"/Alg Dep",theAlgDepVarID,theAlgDepDirID);
  if (theAlgDep==NULL)
  {
    UserWrite("algebraic dependency not found\n");
    return(GM_ERROR);
  }
  DependencyProc = theAlgDep->DependencyProc;
  if (DependencyProc==NULL)
  {
    UserWrite("don't be stupid: implement a dependency!\n");
    return(GM_ERROR);
  }

  /* get find cut dependency */
  if (findcut==NULL)
  {
    FindCutSet = FeedbackVertexVectors;
    UserWrite("default cut set proc:\n    leaving order of cyclic dependencies unchanged\n");
  }
  else
  {
    theFindCut = (FIND_CUT *) SearchEnv(findcut,"/FindCut",theFindCutVarID,theFindCutDirID);
    if (theFindCut==NULL)
    {
      UserWrite("find cut proc not found\n");
      return(GM_ERROR);
    }
    FindCutSet = theFindCut->FindCutProc;
    if (FindCutSet==NULL)
    {
      UserWrite("don't be stupid: implement a find cut proc!\n");
      return(GM_ERROR);
    }
  }

  /* go */
  if (levels==GM_ALL_LEVELS)
    baselevel = 0;
  else
    baselevel = currlevel;
  for (i=baselevel; i<=currlevel; i++)
  {
    theGrid = theMG->grids[i];
    if ((*DependencyProc)(theGrid,dep_options)) return(GM_ERROR);
    if (OrderVectorAlgebraic(theGrid,mode,PutSkipFirst,SkipPat)) return(GM_ERROR);
  }

  return (GM_OK);
}

/****************************************************************************/
/*D
   LexAlgDep - Dependency function for lexicographic ordering

   SYNOPSIS:
   static INT LexAlgDep (GRID *theGrid, char *data);

   PARAMETERS:
   .  theGrid - pointer to grid
   .  data - option string from 'orderv' command

   DESCRIPTION:
   This function defines a dependency function for lexicographic ordering.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
   D*/
/****************************************************************************/

static INT LexAlgDep (GRID *theGrid, const char *data)
{
  MULTIGRID *theMG;
  DOMAIN *theDomain;
  VECTOR *theVector,*NBVector;
  MATRIX *theMatrix;
  COORD_VECTOR pos,nbpos;
  COORD diff[DIM];
  DOUBLE InvMeshSize;
  INT i,order,res;
  INT Sign[DIM],Order[DIM],xused,yused,zused,error,SpecialTreatSkipVecs;
  char ord[3];

  /* read ordering directions */
        #ifdef __TWODIM__
  res = sscanf(data,expandfmt("%2[rlud]"),ord);
        #else
  res = sscanf(data,expandfmt("%3[rlbfud]"),ord);
        #endif
  if (res!=1)
  {
    PrintErrorMessage('E',"LexAlgDep","could not read order type");
    return(1);
  }
  if (strlen(ord)!=DIM)
  {
                #ifdef __TWODIM__
    PrintErrorMessage('E',"LexAlgDep","specify 2 chars out of 'rlud'");
                #else
    PrintErrorMessage('E',"LexAlgDep","specify 3 chars out of 'rlbfud'");
                #endif
    return(1);
  }
  error = xused = yused = zused = FALSE;
  for (i=0; i<DIM; i++)
    switch (ord[i])
    {
    case 'r' :
      if (xused) error = TRUE;
      xused = TRUE;
      Order[i] = _X_; Sign[i] =  1; break;
    case 'l' :
      if (xused) error = TRUE;
      xused = TRUE;
      Order[i] = _X_; Sign[i] = -1; break;

                        #ifdef __TWODIM__
    case 'u' :
      if (yused) error = TRUE;
      yused = TRUE;
      Order[i] = _Y_; Sign[i] =  1; break;
    case 'd' :
      if (yused) error = TRUE;
      yused = TRUE;
      Order[i] = _Y_; Sign[i] = -1; break;
                        #else
    case 'b' :
      if (yused) error = TRUE;
      yused = TRUE;
      Order[i] = _Y_; Sign[i] =  1; break;
    case 'f' :
      if (yused) error = TRUE;
      yused = TRUE;
      Order[i] = _Y_; Sign[i] = -1; break;

    case 'u' :
      if (zused) error = TRUE;
      zused = TRUE;
      Order[i] = _Z_; Sign[i] =  1; break;
    case 'd' :
      if (zused) error = TRUE;
      zused = TRUE;
      Order[i] = _Z_; Sign[i] = -1; break;
                        #endif
    }
  if (error)
  {
    PrintErrorMessage('E',"LexAlgDep","bad combination of 'rludr' or 'rlbfud' resp.");
    return(1);
  }

  /* treat vectors with skipflag set specially? */
  SpecialTreatSkipVecs = FALSE;
  if              (strchr(data,'<')!=NULL)
    SpecialTreatSkipVecs = GM_PUT_AT_BEGIN;
  else if (strchr(data,'>')!=NULL)
    SpecialTreatSkipVecs = GM_PUT_AT_END;

  theMG   = MYMG(theGrid);

  /* find an approximate measure for the mesh size */
  theDomain = MGDOMAIN(theMG);
  if (theDomain==NULL) return (1);
  InvMeshSize = POW2(GLEVEL(theGrid)) * pow(NN(GRID_ON_LEVEL(theMG,0)),1.0/DIM) / theDomain->radius;

  for (theVector=FIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
  {
    VectorPosition(theVector,pos);

    for (theMatrix=MNEXT(VSTART(theVector)); theMatrix!=NULL; theMatrix=MNEXT(theMatrix))
    {
      NBVector = MDEST(theMatrix);

      SETMUP(theMatrix,0);
      SETMDOWN(theMatrix,0);

      if (SpecialTreatSkipVecs)
      {
        if (VECSKIP(theVector) && !VECSKIP(NBVector))
          if (SpecialTreatSkipVecs==GM_PUT_AT_BEGIN)
            order = -1;
          else
            order =  1;

        if (VECSKIP(NBVector) && !VECSKIP(theVector))
          if (SpecialTreatSkipVecs==GM_PUT_AT_BEGIN)
            order =  1;
          else
            order = -1;
      }
      else
      {
        VectorPosition(NBVector,nbpos);

        V_DIM_SUBTRACT(nbpos,pos,diff);
        V_DIM_SCALE(InvMeshSize,diff);

        if (fabs(diff[Order[DIM-1]])<ORDERRES)
        {
                                        #ifdef __THREEDIM__
          if (fabs(diff[Order[DIM-2]])<ORDERRES)
          {
            if (diff[Order[DIM-3]]>0.0) order = -Sign[DIM-3];
            else order =  Sign[DIM-3];
          }
          else
                                        #endif
          if (diff[Order[DIM-2]]>0.0) order = -Sign[DIM-2];
          else order =  Sign[DIM-2];
        }
        else
        {
          if (diff[Order[DIM-1]]>0.0) order = -Sign[DIM-1];
          else order =  Sign[DIM-1];
        }
      }
      if (order==1) SETMUP(theMatrix,1);
      else SETMDOWN(theMatrix,1);
    }
  }

  return (0);
}

static INT LexAlgDep_old (GRID *theGrid, const char *data)
{
  VECTOR *theVector;
  MATRIX *theMatrix;
  COORD_VECTOR begin, end;
  INT i, index[3], c0, c1, c2, flags;

  c0=c1=c2=0;
  if (data!=NULL)
    if (strlen(data)==3)
    {
      for(i=0; i<3; i++)
        switch(data[i])
        {
        case 'x' :
          index[i]=0;
          c0=1;
          break;
        case 'y' :
          index[i]=1;
          c1=1;
          break;
        case 'z' :
          index[i]=2;
          c2=1;
          break;
        }
    }
    else if (strlen(data)==2)
    {
      c2 = 1;
      index[2] = 2;
      for(i=0; i<2; i++)
        switch(data[i])
        {
        case 'x' :
          index[i]=0;
          c0=1;
          break;
        case 'y' :
          index[i]=1;
          c1=1;
          break;
        }
    }
  if (c0+c1+c2!=3)
  {
    UserWrite("use default lex order: xyz\n");
    index[0]=0;
    index[1]=1;
    index[2]=2;
  }

  for (theVector=theGrid->firstVector; theVector!= NULL; theVector=SUCCVC(theVector))
  {
    if (VectorPosition (theVector,begin)) return (1);
    for (theMatrix=MNEXT(VSTART(theVector)); theMatrix!=NULL; theMatrix=MNEXT(theMatrix))
    {
      if (VectorPosition (MDEST(theMatrix),end)) return (1);
      SETMUP(theMatrix,0);
      SETMDOWN(theMatrix,0);
      flags = (begin[index[0]]<end[index[0]]);
      flags |= ((begin[index[0]]>end[index[0]])<<1);
      if (flags==0)
      {
        flags = (begin[index[1]]<end[index[1]]);
        flags |= ((begin[index[1]]>end[index[1]])<<1);
      }
      if (flags==0)
      {
        flags = (begin[index[2]]<end[index[2]]);
        flags |= ((begin[index[2]]>end[index[2]])<<1);
      }
      switch (flags)
      {
      case 0 :
        break;
      case 1 :
        SETMDOWN(theMatrix,1);
        break;
      case 2 :
        SETMUP(theMatrix,1);
        break;
      default :
        return(1);

      }
    }
  }

  return (0);
}

/****************************************************************************/
/*D
   CreateBVStripe - Creates a stripewise domain decomposition

   SYNOPSIS:
   INT CreateBVStripe( GRID *grid, INT points, INT points_per_stripe )

   PARAMETERS:
   .  grid - the grid containing the vectors to be structured
   .  points  - number of vectors, i.e. gridpoints
   .  points_per_stripe - number of points a stripe should contain

   DESCRIPTION:
   From the list of vectors the blockvector-list is generated in
   the following way: beginning
   at the start of the vector-list, blocks are constructed containing
   'points_per_stripe' consecutive vectors. The start of the new
   blockvector-list is anchored in the 'grid'. The blockvectors are
   numbered beginning from 0 along the construction. If the vector-list
   is longer than 'points', the function will not be troubled (this
   overlapping vectors might be for example dirichlet boundary vectors).

   APPLICATION:
   If the vector-list is ordered lexicographic, each line resp. column
   containing 'points_per_stripe' vectors, then this function constructs
   the blockvector structure belonging to a
   `linewise domain decomposition`.

   If 'points_per_stripe' is a natural multiple of the actual linewidth
   (resp. columnlength), this function constructs the blockvector
   structure belonging to a `stripewise domain decomposition`, each stripe
   consisting of 'points_per_stripe'/linewidth lines (resp. columns).

   RETURN VALUE:

   .n GM_OK if ok
   .n GM_OUT_OF_MEM if there was not enough memory to allocate all blockvectors
   .n GM_INCONSISTANCY if the vector-list was too short

   SEE ALSO:
   BLOCKVECTOR, CreateBVDomainHalfening

   D*/
/****************************************************************************/

INT CreateBVStripe( GRID *grid, INT points, INT points_per_stripe )
{
  BLOCKVECTOR *bv;
  register BLOCKVECTOR *prev;
  register INT i, j, nr_blocks;
  register VECTOR *v;

  /* number of blockvectors to be constructed */
  nr_blocks = ( points + points_per_stripe - 1) / points_per_stripe;

  v = FIRSTVECTOR( grid );

  /* construct each blockvector */
  for ( i = 0; (i < nr_blocks) && (v != NULL); i++ )
  {
    (void)CreateBlockvector( grid, &bv );

    if ( i == 0 )
    {
      GFIRSTBV( grid ) = bv;                    /* anchor blockvector list in the grid */
      BVPRED( bv ) = NULL;
    }
    else
    {
      BVSUCC( prev ) = bv;                              /* continue blockvector list */
      BVPRED( bv ) = prev;
    }

    prev = bv;                                                  /* prepare for the next loop */

    if ( bv == NULL )
      return GM_OUT_OF_MEM;

    SETBVDOWNTYPE( bv, BVDOWNTYPEVECTOR );              /* block is at last block level */
    BVNUMBER( bv ) = i;

    /* let the blockvector point to the vector */
    BVDOWNVECTOR( bv ) = v;
    BVFIRSTVECTOR( bv ) = v;

    /* find successor of the last vector of this blockvector and store it;
       update the blockvector description for all vectors of this
       blockvector
     */
    for ( j = points_per_stripe; (j > 0) && (v != NULL); j-- )
    {
      BVD_PUSH_ENTRY( &VBVD( v ), i, &one_level_bvdf );
      v = SUCCVC( v );
    }
    BVENDVECTOR( bv ) = v;
  }

  BVSUCC( bv ) = NULL;                          /* end of the blockvector list */
  GLASTBV( grid ) = bv;

  /* the for loop exited premature because of v == NULL */
  if ( i < nr_blocks )
    return GM_INCONSISTANCY;

  return GM_OK;
}

/****************************************************************************/
/*D
   CreateBVDomainHalfening - Creates a recursive domain halfening decomposition

   SYNOPSIS:
   INT CreateBVDomainHalfening( GRID *grid, INT side )

   PARAMETERS:
   .  grid - the grid containing the vectors to be structured
   .  side - number of points on the side of the quadratic mesh

   DESCRIPTION:
   The grid must be similar to a quadratic mesh. The vectors must be numbered
   linewise lexicographic. Then this function constructs a hierarchy
   of blockvectors describing a recursive domain halfening.

   A quadratic domain is halfened by cutting along a vertical line. The
   points on this line are gathered into subdomain with number 2. The points
   left to this interface-line are collected in subdomain 0, the right ones
   into subdomain 1. The subdomains are listed according to their numbers.
   Each of the 2 resulting subdomains 0 and 1 are now halfened horizontally
   and processed analogical. The halfening is recursively proceeded until
   a subdomain consists of less than 10 points. The initial grid should have
   a side length allowing all halfenings without a remainder.

   RETURN VALUE:
   INT
   .n GM_OK if ok
   .n GM_OUT_OF_MEM if there is not enough memory to allocate the blockvectors

   SEE ALSO:
   BLOCKVECTOR, CreateBVStripe

   D*/
/****************************************************************************/

INT CreateBVDomainHalfening( GRID *grid, INT side )
{
  BLOCKVECTOR *bv;
  register VECTOR *v;
  INT ret;

  /* create first block of the hierarchy (it is only a dummy
     for temporary use) */
  (void)CreateBlockvector( grid, &bv );
  GFIRSTBV( grid ) = bv;
  if ( bv == NULL )
    return GM_OUT_OF_MEM;
  SETBVDOWNTYPE( bv, BVDOWNTYPEVECTOR );
  BVDOWNVECTOR( bv ) = FIRSTVECTOR( grid );
  BVPRED( bv ) = NULL;
  BVSUCC( bv ) = NULL;
  BVNUMBER( bv ) = 0;
  BVENDVECTOR( bv ) = NULL;
  BVFIRSTVECTOR( bv ) = FIRSTVECTOR( grid );

  ret = BlockHalfening( grid, bv, 0, 0, side, side, side, BV_VERTICAL );

  /* set FIRST- and LASTVECTOR */
  v = FIRSTVECTOR( grid ) = BVFIRSTVECTOR( bv );
  for ( ; SUCCVC( v ) != NULL; v = SUCCVC( v ) )
    ;
  LASTVECTOR( grid ) = v;

  if ( ret != GM_OK )
  {
    GFIRSTBV( grid ) = bv;
    GLASTBV( grid ) = bv;
    FreeAllBV( grid );
    GFIRSTBV( grid ) = NULL;
    GLASTBV( grid ) = NULL;
    return ret;
  }

  /* remove the dummy blockvector and set first and last blockvector
     of the grid */
  GFIRSTBV( grid ) = BVDOWNBV( bv );
  DisposeBlockvector( grid, bv );
  bv = GFIRSTBV( grid );
  while ( BVSUCC( bv ) != NULL )
    bv = BVSUCC( bv );
  GLASTBV( grid ) = bv;

  return GM_OK;
}

/****************************************************************************/
/* (without *D, since only internal function)

   BlockHalfening - internal function to perform the work of CreateBVDomainHalfening

   SYNOPSIS:
   INT BlockHalfening( GRID *grid, BLOCKVECTOR *bv, INT left, INT bottom, INT width, INT height, INT side, INT orientation )

   PARAMETERS:
   .  grid - the grid containing the vectors to be structured
   .  bv - blockvector to be subdivided
   .  left - left side of the rectangle representing the 'bv' in the virtual mesh
   .  bottom - bottom side of the rectangle representing the 'bv' in the virtual mesh
   .  width -  sidewidth of the rectangle representing the 'bv' in the virtual mesh
   .  height - sideheight of the rectangle representing the 'bv' in the virtual mesh
   .  side - sidelength of the original square
   .  orientation - gives the orientation of the cutting line (horizontal or vertical)

   DESCRIPTION:
   Performs the work described for the function CreateBVDomainHalfening
   above.

   Consider a virtual equidistant mesh with meshwidth 1 in both directions.
   Together with the linewise lexicographic numbering of the vectors you can
   easily calculate the geometric position of each vector according to his
   number in this mesh. In this way it is easy to determine to which
   subdomain a given vector belongs. The "geometric" information to this
   function are related to this virtual mesh.

   RETURN VALUE:
   INT
   .n GM_OK if ok
   .n GM_OUT_OF_MEM if there is not enough memory to allocate the blockvectors

 */
/****************************************************************************/

INT BlockHalfening( GRID *grid, BLOCKVECTOR *bv, INT left, INT bottom, INT width, INT height, INT side, INT orientation )
{
  VECTOR *v, *end_v, **v0, **v1, **vi;
  BLOCKVECTOR *bv0, *bv1, *bvi;
  register INT index, interface;

  /* create a double linked list of 3 blockvectors */
  v = BVFIRSTVECTOR( bv );
  end_v = BVENDVECTOR( bv );

  if ( CreateBlockvector( grid, &bv0 ) != GM_OK )
    return GM_OUT_OF_MEM;

  BVPRED( bv0 ) = NULL;
  SETBVDOWNTYPE( bv0, BVDOWNTYPEVECTOR );
  BVNUMBER( bv0 ) = 0;
  v0 = &BVDOWNVECTOR( bv0 );

  if ( CreateBlockvector( grid, &bv1 ) != GM_OK )
  {
    DisposeBlockvector( grid, bv0 );
    return GM_OUT_OF_MEM;
  }

  BVSUCC( bv0 ) = bv1;
  BVPRED( bv1 ) = bv0;
  SETBVDOWNTYPE( bv1, BVDOWNTYPEVECTOR );
  BVNUMBER( bv1 ) = 1;
  v1 = &BVDOWNVECTOR( bv1 );

  if ( CreateBlockvector( grid, &bvi ) != GM_OK )
  {
    DisposeBlockvector( grid, bv1 );
    DisposeBlockvector( grid, bv0 );
    return GM_OUT_OF_MEM;
  }

  BVSUCC( bv1 ) = bvi;
  BVPRED( bvi ) = bv1;
  SETBVDOWNTYPE( bvi, BVDOWNTYPEVECTOR );
  BVNUMBER( bvi ) = 2;
  BVSUCC( bvi ) = NULL;
  vi = &BVDOWNVECTOR( bvi );

  /* all necessary memory allocated; insert new block list in father block */
  SETBVDOWNTYPE( bv, BVDOWNTYPEBV );
  BVDOWNBV( bv ) = bv0;

  /* coordinate of the interface line */
  interface = ( (orientation == BV_VERTICAL) ? (width-1)/2 + left : bottom + (height-1)/2 );

  /* sort each vector in one of the 3 subdomains */
  for ( ; v != end_v; v = SUCCVC( v ) )
  {
    /* calculate row/column number from index */
    index = ( (orientation == BV_VERTICAL) ? VINDEX(v) % side : VINDEX(v) / side );

    if ( index < interface )
    {
      /* vector belongs to subdomain 0 */
      *v0 = v;
      v0 = &SUCCVC( v );
      BVD_PUSH_ENTRY( &VBVD( v ), 0, &DH_bvdf );
    }
    else if ( index > interface )
    {
      /* vector belongs to subdomain 1 */
      *v1 = v;
      v1 = &SUCCVC( v );
      BVD_PUSH_ENTRY( &VBVD( v ), 1, &DH_bvdf );
    }
    else
    {
      /* vector belongs to subdomain 2 (interface) */
      *vi = v;
      vi = &SUCCVC( v );
      BVD_PUSH_ENTRY( &VBVD( v ), 2, &DH_bvdf );
    }

  }
  /* complete the relation between blockvectors and vectors */
  BVENDVECTOR( bv0 ) = BVDOWNVECTOR( bv1 );
  BVENDVECTOR( bv1 ) = BVDOWNVECTOR( bvi );
  BVENDVECTOR( bvi ) = end_v;

  BVFIRSTVECTOR( bv0 ) = BVDOWNVECTOR( bv0 );
  BVFIRSTVECTOR( bv1 ) = BVDOWNVECTOR( bv1 );
  BVFIRSTVECTOR( bvi ) = BVDOWNVECTOR( bvi );

  /* relink the 3 subdomain lists to the global vector list */
  *v0 = BVDOWNVECTOR( bv1 );
  *v1 = BVDOWNVECTOR( bvi );
  *vi = end_v;

  /* next loop */

  if ( orientation == BV_VERTICAL )
  {
    /* new dimensions */
    width = ( width - 1 ) / 2;

    /* if there are too many points per subdomain halfen them */
    if ( ( width * height ) > 9 )
    {
      if ( BlockHalfening( grid, bv0, left, bottom, width, height, side, BV_HORIZONTAL ) == GM_OUT_OF_MEM )
        return GM_OUT_OF_MEM;
      if ( BlockHalfening( grid, bv1, left+width+1, bottom, width, height, side, BV_HORIZONTAL ) == GM_OUT_OF_MEM )
        return GM_OUT_OF_MEM;
    }
  }
  else       /* orientation == BV_HORIZONTAL */
  {
    /* new dimensions */
    height = ( height - 1 ) / 2;

    /* if there are too many points per subdomain halfen them */
    if ( ( width * height ) > 9 )
    {
      if ( BlockHalfening( grid, bv0, left, bottom, width, height, side, BV_VERTICAL ) == GM_OUT_OF_MEM )
        return GM_OUT_OF_MEM;
      if ( BlockHalfening( grid, bv1, left, bottom +height+1, width, height, side, BV_VERTICAL ) == GM_OUT_OF_MEM )
        return GM_OUT_OF_MEM;
    }
  }

  return GM_OK;
}



/****************************************************************************/
/*D
   MoveVector - Move vector within the vector list of the grid

   SYNOPSIS:
   INT MoveVector (GRID *theGrid, VECTOR *moveVector,
   VECTOR *destVector, INT after);

   PARAMETERS:
   .  theGrid - pointer to grid
   .  moveVector - vector to move
   .  destVector - vector before or after which 'moveVector' will be inserted
   .  after - true (1) or false (0)

   DESCRIPTION:
   This function moves a 'VECTOR' within the double linked list. If 'after' is
   true then 'moveVector' will be inserted immediately after 'destVector', if
   'after' is false then it will be inserted immediately before. If 'destVector'
   is 'NULL' then 'moveVector' is inserted at the beginning when 'after' is true
   and at the end of the list when 'after' is false.

   RETURN VALUE:
   INT
   .n     0 if ok
   .n     1 if error occured.
   D*/
/****************************************************************************/

INT MoveVector (GRID *theGrid, VECTOR *moveVector, VECTOR *destVector, INT after)
{
  if (theGrid==NULL || moveVector==NULL) return (1);
  if (moveVector==destVector) return (0);

  /* take vector out of list */
  if (PREDVC(moveVector)!=NULL) SUCCVC(PREDVC(moveVector))      = SUCCVC(moveVector);
  else theGrid->firstVector            = SUCCVC(moveVector);
  if (SUCCVC(moveVector)!=NULL) PREDVC(SUCCVC(moveVector))      = PREDVC(moveVector);
  else theGrid->lastVector             = PREDVC(moveVector);

  /* put it in list */
  if (destVector!=NULL)
  {
    if (after)
    {
      if (SUCCVC(destVector)!=NULL) PREDVC(SUCCVC(destVector))      = moveVector;
      else theGrid->lastVector             = moveVector;
      SUCCVC(moveVector) = SUCCVC(destVector);
      PREDVC(moveVector) = destVector;
      SUCCVC(destVector) = moveVector;
    }
    else
    {
      if (PREDVC(destVector)!=NULL) SUCCVC(PREDVC(destVector))      = moveVector;
      else theGrid->firstVector            = moveVector;
      PREDVC(moveVector) = PREDVC(destVector);
      SUCCVC(moveVector) = destVector;
      PREDVC(destVector) = moveVector;
    }
  }
  else
  {
    if (after)
    {
      SUCCVC(moveVector) = theGrid->firstVector;
      PREDVC(moveVector) = NULL;
      theGrid->firstVector = moveVector;
    }
    else
    {
      SUCCVC(moveVector) = NULL;
      PREDVC(moveVector) = theGrid->lastVector;
      theGrid->lastVector = moveVector;
    }
  }

  return (0);
}

/****************************************************************************/
/*
   InitAlgebra - Init algebra

   SYNOPSIS:
   INT InitAlgebra (void);

   PARAMETERS:
   .  void

   DESCRIPTION:
   This function inits algebra.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
 */
/****************************************************************************/

INT InitAlgebra (void)
{
  INT i, j, n;

  /* install the /Alg Dep directory */
  if (ChangeEnvDir("/")==NULL)
  {
    PrintErrorMessage('F',"InitAlgebra","could not changedir to root");
    return(__LINE__);
  }
  theAlgDepDirID = GetNewEnvDirID();
  if (MakeEnvItem("Alg Dep",theAlgDepDirID,sizeof(ENVDIR))==NULL)
  {
    PrintErrorMessage('F',"InitAlgebra","could not install '/Alg Dep' dir");
    return(__LINE__);
  }
  theAlgDepVarID = GetNewEnvVarID();

  /* install the /FindCut directory */
  if (ChangeEnvDir("/")==NULL)
  {
    PrintErrorMessage('F',"InitAlgebra","could not changedir to root");
    return(__LINE__);
  }
  theFindCutDirID = GetNewEnvDirID();
  if (MakeEnvItem("FindCut",theFindCutDirID,sizeof(ENVDIR))==NULL)
  {
    PrintErrorMessage('F',"InitAlgebra","could not install '/FindCut' dir");
    return(__LINE__);
  }
  theFindCutVarID = GetNewEnvVarID();

  /* set MatrixType-field */
  n=0;
  for (i=0; i<MAXVECTORS; i++)
    for (j=0; j<MAXVECTORS-i; j++)
    {
      MatrixType[j][j+i] = n;
      MatrixType[j+i][j] = n;
      n++;
    }
  if (n != MAXMATRICES)
    return (__LINE__);

  /* set ConnectionType-field */
  for (i=0; i<MAXVECTORS; i++)
    for (j=0; j<MAXVECTORS; j++)
    {
      ConnectionType[0][i][j] = MatrixType[i][j] + MAXVECTORS;
      if (i ==j)
        ConnectionType[1][i][j] = i;
      else
        ConnectionType[1][i][j] = -1;
    }
  if (n+MAXVECTORS != MAXCONNECTIONS)
    return (__LINE__);

  /* init standard algebraic dependencies */
  if (CreateAlgebraicDependency ("lex",LexAlgDep)==NULL) return(__LINE__);

  /* init default find cut proc */
  if (CreateFindCutProc ("lex",FeedbackVertexVectors)==NULL) return(__LINE__);

  return (0);
}

#endif
