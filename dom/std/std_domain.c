// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/*! \file std_domain.c
 * \ingroup std
 */

/** \addtogroup std
 *
 * @{
 */

/****************************************************************************/
/*                                                                          */
/* File:      std_domain.c                                                  */
/*                                                                          */
/* Purpose:   standard ug domain description                                */
/*                                                                          */
/* Author:    Klaus Johannsen / Christian Wieners                           */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70550 Stuttgart                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* History:   Feb 18 1996 begin, ug version 3.1                             */
/*            Sep 12 1996 ug version 3.4                                    */
/*            Apr  9 1998 first step to Marc                                */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* include files                                                            */
/* system include files                                                     */
/* application include files                                                */
/*                                                                          */
/****************************************************************************/

/* standard C library */
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "domain.h"

/* low modules */
#include "compiler.h"
#include "heaps.h"
#include "ugenv.h"
#include "bio.h"
#include "misc.h"
#include "fileopen.h"
#include "defaults.h"
#include "general.h"
#include "debug.h"
#include "scan.h"
#include "evm.h"

/* dev modules */
#include "ugdevices.h"

/* domain module */
#include "std_domain.h"

#include "namespace.h"

USING_UG_NAMESPACES

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*    compile time constants defining static data size (i.e. arrays)	    */
/*    other constants                                                       */
/*    macros                                                                */
/*                                                                          */
/****************************************************************************/

#define SMALL_DIFF   SMALL_C*100
#define RESOLUTION   100

#define DEFAULTDOMMEMORY 50000

#define OPTIONLEN 32

#define V2_LINCOMB(a,A,b,B,C)              {(C)[0] = (a)*(A)[0] + (b)*(B)[0];\
                                            (C)[1] = (a)*(A)[1] + (b)*(B)[1];}

#define V2_EUKLIDNORM_OF_DIFF(A,B,b)    (b) = sqrt((double)(((A)[0]-(B)[0])*((A)[0]-(B)[0])+((A)[1]-(B)[1])*((A)[1]-(B)[1])));

#define V3_EUKLIDNORM_OF_DIFF(A,B,b)    (b) = (sqrt((double)(((A)[0]-(B)[0])*((A)[0]-(B)[0])+((A)[1]-(B)[1])*((A)[1]-(B)[1])+((A)[2]-(B)[2])*((A)[2]-(B)[2]))));

#ifdef __TWODIM__
#define V_DIM_EUKLIDNORM_OF_DIFF(A,B,b) V2_EUKLIDNORM_OF_DIFF(A,B,b)
#else
#define V_DIM_EUKLIDNORM_OF_DIFF(A,B,b) V3_EUKLIDNORM_OF_DIFF(A,B,b)
#endif

/****************************************************************************/
/*                                                                          */
/* data structures used in this source file (exported data structures are   */
/* in the corresponding include file!)                                      */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* definition of exported global variables	                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)	    */
/*                                                                          */
/****************************************************************************/

static INT theProblemDirID;     /*!< env type for Problem dir                   */
static INT theBdryCondVarID;    /*!<  env type for Problem vars                 */

static INT theDomainDirID;      /*!<  env type for Domain dir                           */
static INT theBdrySegVarID;     /*!<  env type for bdry segment vars            */
static INT theLinSegVarID;      /*!<  env type for linear segment vars              */

static INT theBVPDirID;         /*!<  env type for BVP dir                                      */

static STD_BVP *currBVP;

REP_ERR_FILE;

/* RCS string */
static char
RCS_ID
  ("$Header$",
  UG_RCS_STRING);

/****************************************************************************/
/*                                                                          */
/* forward declarations of functions used before they are defined	    */
/*                                                                          */
/****************************************************************************/

static INT BndPointGlobal (BNDP * aBndP, DOUBLE * global);
static INT PatchGlobal (PATCH * p, DOUBLE * lambda, DOUBLE * global);

/* Marc specials */

#define MAX_LEN         200
#ifdef __THREEDIM__
#define PRISM_MOD
#endif

static char theLine[MAX_LEN + 1];
static INT nCorners, nBndP, nElem, nPPatch, nLPatch, nTPatch, nPri, nFound;
static INT Marc_Extended;

static int
ExpandLine (char *theLine)
{
  int i, j, k, l;

  if (Marc_Extended)
  {
    i = 76;
    j = 69;
    for (l = 0; l < 3; l++)
    {
      theLine[i] = ' ';
      i--;
      theLine[i] = theLine[j];
      i--;
      j--;
      theLine[i] = theLine[j];
      i--;
      j--;
      theLine[i] = 'e';
      i--;
      for (k = 0; k < 18; k++)
      {
        theLine[i] = theLine[j];
        i--;
        j--;
      }
    }
    theLine[i] = ' ';
    /*printf("%s",theLine); */
  }
  else
  {
    i = 41;
    j = 34;
    for (l = 0; l < 3; l++)
    {
      theLine[i] = ' ';
      i--;
      theLine[i] = theLine[j];
      i--;
      j--;
      theLine[i] = theLine[j];
      i--;
      j--;
      theLine[i] = 'e';
      i--;
      for (k = 0; k < 8; k++)
      {
        theLine[i] = theLine[j];
        i--;
        j--;
      }
    }
    theLine[i] = ' ';
  }

  for (i = 5; i < strlen (theLine); i++)
    if (theLine[i] == '-')
      if (theLine[i + 1] == 'e')
      {
        theLine[i] = 'e';
        theLine[i + 1] = '-';
      }

  return (0);
}

static INT
file_readline (FILE * f, char *key)
{
  INT len = strlen (key);

  do
  {
    fgets (theLine, MAX_LEN, f);
    if (strncmp (theLine, key, len) == 0)
      return (0);
  }
  while (!feof (f));

  return (1);
}

static INT
file_elements (FILE * f)
{
  char *buffer;
  int id, n, i, c[8];

  nElem = 0;
  nPri = 0;
  do
  {
    fgets (theLine, MAX_LEN, f);
    /*printf("%s",theLine); */

    if (strlen (theLine) < 3)
      continue;

    if (Marc_Extended == 0)
    {
      theLine[66] = 0;
      theLine[65] = ' ';
      theLine[64] = theLine[54];
      theLine[63] = theLine[53];
      theLine[62] = theLine[52];
      theLine[61] = theLine[51];
      theLine[60] = theLine[50];
      theLine[59] = ' ';
      theLine[58] = theLine[49];
      theLine[57] = theLine[48];
      theLine[56] = theLine[47];
      theLine[55] = theLine[46];
      theLine[54] = theLine[45];
      theLine[53] = ' ';
      theLine[52] = theLine[44];
      theLine[51] = theLine[43];
      theLine[50] = theLine[42];
      theLine[49] = theLine[41];
      theLine[48] = theLine[40];
      theLine[47] = ' ';
      theLine[46] = theLine[39];
      theLine[45] = theLine[38];
      theLine[44] = theLine[37];
      theLine[43] = theLine[36];
      theLine[42] = theLine[35];
      theLine[41] = ' ';
      theLine[40] = theLine[34];
      theLine[39] = theLine[33];
      theLine[38] = theLine[32];
      theLine[37] = theLine[31];
      theLine[36] = theLine[30];
      theLine[35] = ' ';
      theLine[34] = theLine[29];
      theLine[33] = theLine[28];
      theLine[32] = theLine[27];
      theLine[31] = theLine[26];
      theLine[30] = theLine[25];
      theLine[29] = ' ';
      theLine[28] = theLine[24];
      theLine[27] = theLine[23];
      theLine[26] = theLine[22];
      theLine[25] = theLine[21];
      theLine[24] = theLine[20];
      theLine[23] = ' ';
      theLine[22] = theLine[19];
      theLine[21] = theLine[18];
      theLine[20] = theLine[17];
      theLine[19] = theLine[16];
      theLine[18] = theLine[15];
      theLine[17] = ' ';
      theLine[16] = theLine[14];
      theLine[15] = theLine[13];
      theLine[14] = theLine[12];
      theLine[13] = theLine[11];
      theLine[12] = theLine[10];
      theLine[11] = ' ';
      theLine[10] = theLine[9];
      theLine[9] = theLine[8];
      theLine[8] = theLine[7];
      theLine[7] = theLine[6];
      theLine[6] = theLine[5];
      theLine[5] = ' ';
      theLine[4] = theLine[4];
      theLine[3] = theLine[3];
      theLine[2] = theLine[2];
      theLine[1] = theLine[1];
      theLine[0] = theLine[0];
    }

    if (sscanf (theLine, "%d %d", &id, &n) != 2)
      return (0);

    if (sscanf (theLine, "%d %d %d %d %d %d %d %d %d %d",
                &id, &n, c, c + 1, c + 2, c + 3, c + 4, c + 5, c + 6,
                c + 7) != 3 + n)
      return (0);


    if (c[0] == c[1])
      if (c[4] == c[5])
        nPri++;

    nElem++;

    /* printf("file_elem %d %d %d %d %d\n",id,n,c[0],c[1],c[2]);  */

  }
  while (!feof (f));

  return (1);
}

static INT
file_corners (FILE * f)
{
  int N, n, id, i, j, k;
  double c[3];

  fgets (theLine, MAX_LEN, f);

  /* printf("%s",theLine);  */

  if (Marc_Extended == 0)
  {
    theLine[12] = 0;
    theLine[11] = ' ';
    theLine[10] = theLine[9];
    theLine[9] = theLine[8];
    theLine[8] = theLine[7];
    theLine[7] = theLine[6];
    theLine[6] = theLine[5];
    theLine[5] = ' ';
    theLine[4] = theLine[4];
    theLine[3] = theLine[3];
    theLine[2] = theLine[2];
    theLine[1] = theLine[1];
    theLine[0] = theLine[0];
  }

  if (sscanf (theLine, "%d %d", &N, &n) != 2)
    return (1);

  nCorners = 0;
  for (i = 0; i < n; i++)
  {
    fgets (theLine, MAX_LEN, f);

    /* printf(" %d %s",i,theLine);  */

    ExpandLine (theLine);

    if (sscanf (theLine, "%d %lg %lg %lg", &id, c, c + 1, c + 2) != 1 + N)
      return (0);
    nCorners++;

    /* printf("%d %f %f %f\n",id,c[0],c[1],c[2]); */
  }

  return (0);
}

static INT
file_contact (FILE * f)
{
  int id, c;

  /* fgets(theLine, MAX_LEN, f); */

  nBndP = 0;
  do
  {
    fgets (theLine, MAX_LEN, f);
    if (sscanf (theLine, "%d %d", &id, &c) != 2)
      return (0);
    nBndP++;

    /* printf("%d %d \n",id,c);  */

  }
  while (!feof (f));

  UserWrite ("BVP: no contact\n");

  return (0);
}

static INT
file_triangles (FILE * f)
{
  char *buffer;
  int id, n, i, c[3];

  nTPatch = 0;
  do
  {
    fgets (theLine, MAX_LEN, f);
    if (strlen (theLine) < 3)
      continue;
    if (sscanf (theLine, "%5d%5d%5d%5d%5d",
                &id, &n, c, c + 1, c + 2) != 2 + DIM)
      return (0);
    nTPatch++;

    /*printf("%d %d %d %d %d\n",id,n,c[0],c[1],c[2]); */

  }
  while (!feof (f));

  return (1);
}

static INT
file_positions (FILE * f)
{
  char *buffer;
  int id, j, k;
  double c[3];

  fgets (theLine, MAX_LEN, f);
  /* printf("%s",theLine); */
  nPPatch = 0;
  do
  {
    fgets (theLine, MAX_LEN, f);

    ExpandLine (theLine);

    if (sscanf (theLine, "%d %lg %lg %lg", &id, c, c + 1, c + 2) != 1 + DIM)
      return (0);

    nPPatch++;
  }
  while (!feof (f));

  return (1);
}

static INT
Marc_GetPosition (MESH * Mesh, INT i, DOUBLE * x)
{
  INT k;

  if (i < nBndP)
  {
    M_BNDP *p = (M_BNDP *) Mesh->theBndPs[i];

    for (k = 0; k < DIM; k++)
      x[k] = p->pos[k];
  }
  else
    for (k = 0; k < DIM; k++)
      x[k] = Mesh->Position[i - nBndP][k];
  /*printf("i %6d nBndP %d x %6.3f %6.3f %6.3f\n",i,nBndP,x[0],x[1],x[2]); */

  return (0);
}

static INT
file_elements_fill (FILE * f, HEAP * Heap, MESH * Mesh, INT MarkKey)
{
  int id, n, i, c[8], c0, c1, c2, c3, c4, c5, c6, c7;

  nElem = 0;
  nPri = 0;
  nFound = 0;
  do
  {
    fgets (theLine, MAX_LEN, f);
    if (strlen (theLine) < 3)
      continue;

    if (Marc_Extended == 0)
    {
      theLine[66] = 0;
      theLine[65] = ' ';
      theLine[64] = theLine[54];
      theLine[63] = theLine[53];
      theLine[62] = theLine[52];
      theLine[61] = theLine[51];
      theLine[60] = theLine[50];
      theLine[59] = ' ';
      theLine[58] = theLine[49];
      theLine[57] = theLine[48];
      theLine[56] = theLine[47];
      theLine[55] = theLine[46];
      theLine[54] = theLine[45];
      theLine[53] = ' ';
      theLine[52] = theLine[44];
      theLine[51] = theLine[43];
      theLine[50] = theLine[42];
      theLine[49] = theLine[41];
      theLine[48] = theLine[40];
      theLine[47] = ' ';
      theLine[46] = theLine[39];
      theLine[45] = theLine[38];
      theLine[44] = theLine[37];
      theLine[43] = theLine[36];
      theLine[42] = theLine[35];
      theLine[41] = ' ';
      theLine[40] = theLine[34];
      theLine[39] = theLine[33];
      theLine[38] = theLine[32];
      theLine[37] = theLine[31];
      theLine[36] = theLine[30];
      theLine[35] = ' ';
      theLine[34] = theLine[29];
      theLine[33] = theLine[28];
      theLine[32] = theLine[27];
      theLine[31] = theLine[26];
      theLine[30] = theLine[25];
      theLine[29] = ' ';
      theLine[28] = theLine[24];
      theLine[27] = theLine[23];
      theLine[26] = theLine[22];
      theLine[25] = theLine[21];
      theLine[24] = theLine[20];
      theLine[23] = ' ';
      theLine[22] = theLine[19];
      theLine[21] = theLine[18];
      theLine[20] = theLine[17];
      theLine[19] = theLine[16];
      theLine[18] = theLine[15];
      theLine[17] = ' ';
      theLine[16] = theLine[14];
      theLine[15] = theLine[13];
      theLine[14] = theLine[12];
      theLine[13] = theLine[11];
      theLine[12] = theLine[10];
      theLine[11] = ' ';
      theLine[10] = theLine[9];
      theLine[9] = theLine[8];
      theLine[8] = theLine[7];
      theLine[7] = theLine[6];
      theLine[6] = theLine[5];
      theLine[5] = ' ';
      theLine[4] = theLine[4];
      theLine[3] = theLine[3];
      theLine[2] = theLine[2];
      theLine[1] = theLine[1];
      theLine[0] = theLine[0];
    }

    if (sscanf (theLine, "%d %d", &id, &n) != 2)
      return (0);

    if (sscanf (theLine, "%d %d %d %d %d %d %d %d %d %d",
                &id, &n, &c0, &c1, &c2, &c3, &c4, &c5, &c6, &c7) != 3 + n)
      return (0);

    c[0] = c0;
    c[1] = c1;
    c[2] = c2;
    c[3] = c3;
    c[4] = c4;
    c[5] = c5;
    c[6] = c6;
    c[7] = c7;

    if (Mesh != NULL)
      if (c[0] == c[1])
      {
        if (c[4] != c[5])
          return (1);
        Mesh->Element_corners[1][nElem] = 6;
        Mesh->Element_corner_ids[1][nElem][0] = c[0] - 1;
        Mesh->Element_corner_ids[1][nElem][1] = c[2] - 1;
        Mesh->Element_corner_ids[1][nElem][2] = c[3] - 1;
        Mesh->Element_corner_ids[1][nElem][3] = c[4] - 1;
        Mesh->Element_corner_ids[1][nElem][4] = c[6] - 1;
        Mesh->Element_corner_ids[1][nElem][5] = c[7] - 1;

        nPri++;

#ifdef PRISM_MOD

        if ((nPri % 2) == 0)
        {
          INT l, d[6];
          DOUBLE_VECTOR pos;

          for (l = 0; l < 6; l++)
            d[l] = Mesh->Element_corner_ids[1][nElem - 1][l];
          c[0] = c[0] - 1;
          c[1] = c[2] - 1;
          c[2] = c[3] - 1;
          c[3] = c[4] - 1;
          c[4] = c[6] - 1;
          c[5] = c[7] - 1;

          Marc_GetPosition (Mesh, d[0], pos);
          for (l = 0; l < DIM; l++)
            Mesh->Position[nCorners - nBndP][l] = 0.25 * pos[l];
          Marc_GetPosition (Mesh, d[1], pos);
          for (l = 0; l < DIM; l++)
            Mesh->Position[nCorners - nBndP][l] += 0.25 * pos[l];
          Marc_GetPosition (Mesh, d[3], pos);
          for (l = 0; l < DIM; l++)
            Mesh->Position[nCorners - nBndP][l] += 0.25 * pos[l];
          Marc_GetPosition (Mesh, d[4], pos);
          for (l = 0; l < DIM; l++)
            Mesh->Position[nCorners - nBndP][l] += 0.25 * pos[l];


          IFDEBUG (dom, 2)
          assert (Mesh->Element_corners[1][nElem - 1] == 6);

          for (l = 0; l < nElem; l++)
            if (Mesh->Element_corners[1][l] == 8)
              if (CheckPrisms (Mesh->Element_corner_ids[1][l],
                               Mesh->Element_corner_ids[1][nElem -
                                                           1][2],
                               Mesh->Element_corner_ids[1][nElem -
                                                           1][1],
                               Mesh->Element_corner_ids[1][nElem][1],
                               Mesh->Element_corner_ids[1][nElem -
                                                           1][0]))
                nFound++;
          assert (Mesh->Element_corner_ids[1][nElem - 1][0]
                  == Mesh->Element_corner_ids[1][nElem][0]);
          assert (Mesh->Element_corner_ids[1][nElem - 1][3]
                  == Mesh->Element_corner_ids[1][nElem][3]);
          assert (Mesh->Element_corner_ids[1][nElem - 1][1]
                  == Mesh->Element_corner_ids[1][nElem][2]);
          assert (Mesh->Element_corner_ids[1][nElem - 1][4]
                  == Mesh->Element_corner_ids[1][nElem][5]);
          ENDDEBUG nElem--;

          Mesh->Element_corners[1][nElem] = 5;
          Mesh->Element_corner_ids[1][nElem][0] = c[0];
          Mesh->Element_corner_ids[1][nElem][1] = c[1];
          Mesh->Element_corner_ids[1][nElem][2] = c[2];
          Mesh->Element_corner_ids[1][nElem][3] = d[2];
          Mesh->Element_corner_ids[1][nElem][4] = nCorners;
          nElem++;
          Mesh->Element_corners[1][nElem] = 5;
          Mesh->Element_corner_ids[1][nElem][0] = d[0];
          Mesh->Element_corner_ids[1][nElem][1] = d[2];
          Mesh->Element_corner_ids[1][nElem][2] = d[5];
          Mesh->Element_corner_ids[1][nElem][3] = d[3];
          Mesh->Element_corner_ids[1][nElem][4] = nCorners;
          nElem++;
          Mesh->Element_corners[1][nElem] = 5;
          Mesh->Element_corner_ids[1][nElem][0] = d[5];
          Mesh->Element_corner_ids[1][nElem][1] = d[2];
          Mesh->Element_corner_ids[1][nElem][2] = d[1];
          Mesh->Element_corner_ids[1][nElem][3] = d[4];
          Mesh->Element_corner_ids[1][nElem][4] = nCorners;
          nElem++;
          Mesh->Element_corners[1][nElem] = 5;
          Mesh->Element_corner_ids[1][nElem][0] = c[0];
          Mesh->Element_corner_ids[1][nElem][1] = c[3];
          Mesh->Element_corner_ids[1][nElem][2] = c[4];
          Mesh->Element_corner_ids[1][nElem][3] = c[1];
          Mesh->Element_corner_ids[1][nElem][4] = nCorners;
          nElem++;
          Mesh->Element_corners[1][nElem] = 5;
          Mesh->Element_corner_ids[1][nElem][0] = c[1];
          Mesh->Element_corner_ids[1][nElem][1] = c[4];
          Mesh->Element_corner_ids[1][nElem][2] = c[5];
          Mesh->Element_corner_ids[1][nElem][3] = c[2];
          Mesh->Element_corner_ids[1][nElem][4] = nCorners;
          nElem++;
          Mesh->Element_corners[1][nElem] = 4;
          Mesh->Element_corner_ids[1][nElem][0] = d[3];
          Mesh->Element_corner_ids[1][nElem][1] = d[5];
          Mesh->Element_corner_ids[1][nElem][2] = d[4];
          Mesh->Element_corner_ids[1][nElem][3] = nCorners;
          nElem++;
          Mesh->Element_corners[1][nElem] = 4;
          Mesh->Element_corner_ids[1][nElem][0] = c[3];
          Mesh->Element_corner_ids[1][nElem][1] = c[5];
          Mesh->Element_corner_ids[1][nElem][2] = c[4];
          Mesh->Element_corner_ids[1][nElem][3] = nCorners;

          nCorners++;


        }
#endif
      }
      else
      {
        Mesh->Element_corners[1][nElem] = n + 1;
        for (i = 0; i <= n; i++)
          Mesh->Element_corner_ids[1][nElem][i] = c[i] - 1;
      }
    nElem++;

    /*		printf("%d %d %d %d %d\n",id,n,c[0],c[1],c[2]); */

  }
  while (!feof (f));

  return (1);
}

static INT
file_corners_fill (FILE * f, HEAP * Heap, MESH * Mesh, INT MarkKey,
                   DOUBLE * radius, DOUBLE * midpoint)
{
  int N, n, id, i, j, k;
  double c[3], s;

  fgets (theLine, MAX_LEN, f);
  /* printf("%s",theLine); */

  if (Marc_Extended == 0)
  {
    theLine[12] = 0;
    theLine[11] = ' ';
    theLine[10] = theLine[9];
    theLine[9] = theLine[8];
    theLine[8] = theLine[7];
    theLine[7] = theLine[6];
    theLine[6] = theLine[5];
    theLine[5] = ' ';
    theLine[4] = theLine[4];
    theLine[3] = theLine[3];
    theLine[2] = theLine[2];
    theLine[1] = theLine[1];
    theLine[0] = theLine[0];
  }

  if (sscanf (theLine, "%d %d", &N, &n) != 2)
    return (1);

  nCorners = n;

  for (j = 0; j < 3; j++)
    midpoint[j] = 0.0;

  for (i = 0; i < n; i++)
  {
    fgets (theLine, MAX_LEN, f);

    /* printf("in\n%s\n",theLine); */

    ExpandLine (theLine);

    if (sscanf (theLine, "%d %lg %lg %lg", &id, c, c + 1, c + 2) != 1 + N)
      return (0);

    if (i < nBndP)
      for (j = 0; j < DIM; j++)
        midpoint[j] += c[j];

    if (Mesh != NULL)
    {
      if (i < nBndP)
      {
        M_BNDP *p = (M_BNDP *) Mesh->theBndPs[i];

        /* TODO remove  */
        M0_PATCH *patch = (M0_PATCH *) currBVP->patches[i];

        patch->type = MARC_0_PATCH_TYPE;
        patch->id = i;
        for (j = 0; j < DIM; j++)
          patch->pos[j] = c[j];
        /*   */

        p->patch_id = i;
        for (j = 0; j < DIM; j++)
          p->pos[j] = c[j];
      }
      else
        for (j = 0; j < 3; j++)
          Mesh->Position[i - nBndP][j] = c[j];
    }
    /* printf("%d %f %f %f\n",id,c[0],c[1],c[2]);  */
  }
  s = 1.0 / nBndP;
  for (j = 0; j < DIM; j++)
    midpoint[j] *= s;
  if (Mesh != NULL)
  {
    *radius = 0.0;
    for (i = 0; i < nBndP; i++)
    {
      M_BNDP *p = (M_BNDP *) Mesh->theBndPs[i];

      for (j = 0; j < DIM; j++)
        *radius = MAX (*radius, fabs (midpoint[j] - p->pos[j]));
    }
  }
  else
  {
    *radius = 0.0;
    for (j = 0; j < DIM; j++)
      *radius += ABS (midpoint[j]);
  }

  return (0);
}

static INT
file_contact_fill (FILE * f, HEAP * Heap, MESH * Mesh, INT MarkKey)
{
  int id, c;

  /* TODO: adapt for surface mesh */
  return (0);

  /*	fgets(theLine, MAX_LEN, f); */

  if (Mesh == NULL)
    return (0);

  nBndP = 0;
  do
  {
    M_BNDP *p = (M_BNDP *) Mesh->theBndPs[nBndP];

    fgets (theLine, MAX_LEN, f);
    if (sscanf (theLine, "%d %d", &id, &c) != 2)
      return (0);
    nBndP++;

    p->patch_id = nPPatch + c - 1;

    /* printf("%d %d \n",id,c);  */

  }
  while (!feof (f));

  UserWrite ("BVP: no contact\n");

  return (0);
}

static INT
file_triangles_fill (FILE * f, HEAP * Heap, MESH * Mesh, INT MarkKey)
{
  char *buffer;
  int id, n, i, c[3];

  nTPatch = 0;
  do
  {
    M2_PATCH *p = (M2_PATCH *) currBVP->patches[nPPatch + nTPatch];

    fgets (theLine, MAX_LEN, f);
    if (strlen (theLine) < 3)
      continue;
    if (sscanf (theLine, "%5d%5d%5d%5d%5d",
                &id, &n, c, c + 1, c + 2) != 2 + DIM)
      return (0);

    p->type = MARC_2_PATCH_TYPE;
    p->c = n;
    p->id = nTPatch + nPPatch;
    for (i = 0; i < 3; i++)
      p->p[i] = c[i] - 1;

    nTPatch++;

    /* printf("%d id %d %d %d %d %d\n",id,p->id,n,c[0],c[1],c[2]); */

  }
  while (!feof (f));

  return (1);
}

static INT
file_positions_fill (FILE * f, HEAP * Heap, MESH * Mesh, INT MarkKey)
{
  char *buffer;
  int id, j, k;
  double c[3];
  M0_PATCH *p;

  /* TODO: adapt for surface mesh */
  return (0);

  fgets (theLine, MAX_LEN, f);
  nPPatch = 0;
  do
  {
    fgets (theLine, MAX_LEN, f);

    ExpandLine (theLine);

    if (sscanf (theLine, "%d %lg %lg %lg", &id, c, c + 1, c + 2) != 1 + DIM)
      return (0);

    p = (M0_PATCH *) currBVP->patches[nPPatch];
    p->type = MARC_0_PATCH_TYPE;
    p->id = nPPatch;
    for (j = 0; j < 3; j++)
      p->pos[j] = c[j];

    nPPatch++;

    /* printf("%d %f %f %f nP %d\n",id,c[0],c[1],c[2],nPPatch); */

  }
  while (!feof (f));

  return (1);
}

static INT
Marc_Configure (INT argc, char **argv)
{
  STD_BVP *theBVP;
  char BVPName[NAMESIZE];
  INT i;
  char name[NAMESIZE];

  /* get BVP name */
  if ((sscanf
         (argv[0], expandfmt (CONCAT3 (" configure %", NAMELENSTR, "[ -~]")),
         BVPName) != 1) || (strlen (BVPName) == 0))
  {
    for (i = 0; i < argc; i++)
      if (argv[i][0] == 'b')
        if ((sscanf
               (argv[i], expandfmt (CONCAT3 ("b %", NAMELENSTR, "[ -~]")),
               BVPName) == 1) && (strlen (BVPName) > 0))
          break;
    if (i >= argc)
      RETURN (1);
  }
  theBVP = (STD_BVP *) BVP_GetByName (BVPName);
  if (theBVP == NULL)
    RETURN (1);

  for (i = 1; i < argc; i++)
  {
    if (argv[i][0] == 'm')
    {
      if (sscanf (argv[i], "mesh %s", name) != 1)
        continue;
      strcpy (theBVP->mesh_file, name);
    }
    if (argv[i][0] == 'b')
    {
      if (sscanf (argv[i], "bnd %s", name) != 1)
        continue;
      strcpy (theBVP->bnd_file, name);
    }
  }

  return (0);
}

static BVP *
Init_MarcBVP (STD_BVP * theBVP, HEAP * Heap, MESH * Mesh, INT MarkKey)
{
  FILE *stream;
  INT i;

  currBVP = theBVP;
#ifdef ModelP
  SetBVPType (theBVP->type);
#endif
  STD_BVP_NDOMPART (theBVP) = 1;
  STD_BVP_NSUBDOM (theBVP) = 1;
  STD_BVP_S2P_PTR (theBVP) = (INT *)
                             GetFreelistMemory (Heap, (1 + STD_BVP_NSUBDOM (theBVP)) * sizeof (INT));
  if (STD_BVP_S2P_PTR (theBVP) == NULL)
    return (NULL);
  STD_BVP_S2P_PTR (theBVP)[0] = 0;
  STD_BVP_S2P_PTR (theBVP)[1] = 0;

#ifdef ModelP
  if (me == 0)
  {
#endif

  nPPatch = nLPatch = nTPatch = 0;

  /* read numbers of objects */
  stream = fileopen (theBVP->mesh_file, "r");
  if (stream == NULL)
  {
    PrintErrorMessage ('F', "Init_MarcBVP", "could not open file");
    REP_ERR_RETURN_PTR (NULL);
  }
  if (file_readline (stream, "extended"))
  {
    Marc_Extended = 0;
    fclose (stream);
    stream = fileopen (theBVP->mesh_file, "r");
  }
  else
  {
    Marc_Extended = 1;
  }
  if (file_readline (stream, "connectivity"))
  {
    PrintErrorMessage ('F', "Init_MarcBVP",
                       "could not read connectivity");
    REP_ERR_RETURN_PTR (NULL);
  }
  if (file_elements (stream))
  {
    PrintErrorMessage ('F', "Init_MarcBVP", "could not read element");
    REP_ERR_RETURN_PTR (NULL);
  }
  if (file_corners (stream))
  {
    PrintErrorMessage ('F', "Init_MarcBVP", "could not read corners");
    REP_ERR_RETURN_PTR (NULL);
  }
  if (file_readline (stream, "surface"))
  {
    PrintErrorMessage ('F', "Init_MarcBVP", "could not read surface");
    REP_ERR_RETURN_PTR (NULL);
  }
  if (file_contact (stream))
  {
    PrintErrorMessage ('F', "Init_MarcBVP", "could not read contact");
    REP_ERR_RETURN_PTR (NULL);
  }
  fclose (stream);
  stream = fileopen (theBVP->bnd_file, "r");
  if (stream == NULL)
  {
    PrintErrorMessage ('F', "Init_MarcBVP", "could not open file");
    REP_ERR_RETURN_PTR (NULL);
  }
  if (file_readline (stream, "connectivity"))
  {
    PrintErrorMessage ('F', "Init_MarcBVP",
                       "could not store connectivity");
    REP_ERR_RETURN_PTR (NULL);
  }
  if (file_triangles (stream))
  {
    PrintErrorMessage ('F', "Init_MarcBVP", "could not store surface");
    REP_ERR_RETURN_PTR (NULL);
  }
  if (file_positions (stream))
  {
    PrintErrorMessage ('F', "Init_MarcBVP",
                       "could not store positions");
    REP_ERR_RETURN_PTR (NULL);
  }
  fclose (stream);

  PRINTDEBUG (dom, 1,
              ("A: nCorners %d nBndP %d nElem %d nPPatch %d nTPatch %d nPri %d\n",
               nCorners, nBndP, nElem, nPPatch, nTPatch, nPri));

#ifdef PRISM_MOD
  nPri = nPri / 2;
  nElem += 5 * nPri;
  nCorners += nPri;
#endif

  nPPatch = MAX (nPPatch, nBndP);

#ifdef ModelP
}
Broadcast (&nCorners, sizeof (INT));
Broadcast (&nBndP, sizeof (INT));
Broadcast (&nElem, sizeof (INT));
Broadcast (&nPPatch, sizeof (INT));
Broadcast (&nTPatch, sizeof (INT));
#endif

  /* fill mesh and patch infos */
  if (Mesh != NULL)
  {
    Mesh->mesh_status = MESHSTAT_MESH;
    Mesh->nBndP = nBndP;
    Mesh->theBndPs =
      (BNDP **) GetTmpMem (Heap, nBndP * sizeof (M_BNDP *), MarkKey);
    if (Mesh->theBndPs == NULL)
      REP_ERR_RETURN_PTR (NULL);
    for (i = 0; i < nBndP; i++)
    {
      Mesh->theBndPs[i] =
        (BNDP *) GetFreelistMemory (Heap, sizeof (M_BNDP));
      if (Mesh->theBndPs[i] == NULL)
        REP_ERR_RETURN_PTR (NULL);
    }
    Mesh->nInnP = nCorners - nBndP;
    Mesh->Position = (DOUBLE **)
                     GetTmpMem (Heap, (nCorners - nBndP) * sizeof (DOUBLE *), MarkKey);
    if (nCorners > nBndP)
      if (Mesh->Position == NULL)
        REP_ERR_RETURN_PTR (NULL);
    for (i = 0; i < nCorners - nBndP; i++)
    {
      Mesh->Position[i] = (DOUBLE *)
                          GetTmpMem (Heap, 3 * sizeof (DOUBLE), MarkKey);
      if (Mesh->Position[i] == NULL)
        REP_ERR_RETURN_PTR (NULL);
    }
    Mesh->nSubDomains = 1;
    Mesh->nSides = NULL;
    Mesh->Side_corners = NULL;
    Mesh->xy_Side = NULL;
    Mesh->Side_corner_ids = NULL;
    Mesh->nElements = (INT *)
                      GetTmpMem (Heap, (Mesh->nSubDomains + 1) * sizeof (INT), MarkKey);
    Mesh->nElements[0] = 0;
    Mesh->nElements[1] = nElem;
    Mesh->Element_corners = (INT **)
                            GetTmpMem (Heap, (Mesh->nSubDomains + 1) * sizeof (INT), MarkKey);
    Mesh->Element_corners[0] = NULL;
    Mesh->Element_corners[1] = (INT *)
                               GetTmpMem (Heap, nElem * sizeof (INT), MarkKey);
    for (i = 0; i < nElem; i++)
      Mesh->Element_corners[1][i] = 8;
    Mesh->Element_corner_ids = (INT ***)
                               GetTmpMem (Heap, (Mesh->nSubDomains + 1) * sizeof (INT **), MarkKey);
    Mesh->Element_corner_ids[0] = NULL;
    Mesh->Element_corner_ids[1] = (INT **)
                                  GetTmpMem (Heap, nElem * sizeof (INT *), MarkKey);
    for (i = 0; i < nElem; i++)
    {
      Mesh->Element_corner_ids[1][i] = (INT *)
                                       GetTmpMem (Heap, Mesh->Element_corners[1][i] * sizeof (INT),
                                                  MarkKey);
    }
    Mesh->nbElements = NULL;
    Mesh->ElemSideOnBnd = NULL;

    /* parallel part */
    Mesh->VertexLevel = NULL;
    Mesh->VertexPrio = NULL;
    Mesh->ElementLevel = NULL;
    Mesh->ElementPrio = NULL;
  }
  theBVP->patches = (PATCH **)
                    GetFreelistMemory (Heap, (nPPatch + nTPatch) * sizeof (PATCH *));
  for (i = 0; i < nPPatch; i++)
  {
    M0_PATCH *patch =
      (M0_PATCH *) GetFreelistMemory (Heap, sizeof (M0_PATCH));

    patch->type = MARC_0_PATCH_TYPE;
    patch->id = i;
    theBVP->patches[i] = (PATCH *) patch;
  }
  for (i = nPPatch; i < nPPatch + nLPatch; i++)
    theBVP->patches[i] = (PATCH *)
                         GetFreelistMemory (Heap, sizeof (M1_PATCH));
  for (i = nPPatch + nLPatch; i < nPPatch + nLPatch + nTPatch; i++)
    theBVP->patches[i] = (PATCH *)
                         GetFreelistMemory (Heap, sizeof (M2_PATCH));
  currBVP->sideoffset = nPPatch + nLPatch;

  PRINTDEBUG (dom, 1,
              ("B: nCorners %d nBndP %d nElem %d nPPatch %d nTPatch %d\n",
               nCorners, nBndP, nElem, nPPatch, nTPatch));

#ifdef ModelP
  if (me == 0)
  {
#endif

  stream = fileopen (theBVP->bnd_file, "r");
  if (stream == NULL)
    REP_ERR_RETURN_PTR (NULL);

  if (file_readline (stream, "connectivity"))
    REP_ERR_RETURN_PTR (NULL);

  if (file_triangles_fill (stream, Heap, Mesh, MarkKey))
    REP_ERR_RETURN_PTR (NULL);

  if (file_positions_fill (stream, Heap, Mesh, MarkKey))
    REP_ERR_RETURN_PTR (NULL);

  fclose (stream);

  stream = fileopen (theBVP->mesh_file, "r");
  if (stream == NULL)
    REP_ERR_RETURN_PTR (NULL);

  if (file_readline (stream, "coordinates"))
    REP_ERR_RETURN_PTR (NULL);

  if (file_corners_fill (stream, Heap, Mesh, MarkKey,
                         &(theBVP->radius), theBVP->MidPoint))
    REP_ERR_RETURN_PTR (NULL);

  fclose (stream);

  stream = fileopen (theBVP->mesh_file, "r");
  if (stream == NULL)
    REP_ERR_RETURN_PTR (NULL);

  if (file_readline (stream, "connectivity"))
    REP_ERR_RETURN_PTR (NULL);

  if (file_elements_fill (stream, Heap, Mesh, MarkKey))
    REP_ERR_RETURN_PTR (NULL);

  if (file_readline (stream, "surface"))
  {
    PrintErrorMessage ('F', "Init_MarcBVP", "could not read surface");
    REP_ERR_RETURN_PTR (NULL);
  }
  if (file_contact_fill (stream, Heap, Mesh, MarkKey))
    REP_ERR_RETURN_PTR (NULL);

  fclose (stream);

  /*
     #ifdef __THREEDIM__
     if (Mesh != NULL)
     RepairMesh(Heap,MarkKey,Mesh);
     #endif
   */

#ifdef ModelP
}
for (i = 0; i < nPPatch; i++)
  Broadcast (theBVP->patches[i], sizeof (M0_PATCH));
for (i = nPPatch; i < nPPatch + nLPatch; i++)
  Broadcast (theBVP->patches[i], sizeof (M1_PATCH));
for (i = nPPatch + nLPatch; i < nPPatch + nLPatch + nTPatch; i++)
  Broadcast (theBVP->patches[i], sizeof (M2_PATCH));
Broadcast (&(theBVP->radius), sizeof (DOUBLE));
Broadcast (theBVP->MidPoint, DIM * sizeof (DOUBLE));
#endif

  STD_BVP_NDOMPART (theBVP) = 1;
  STD_BVP_NSUBDOM (theBVP) = 1;
  STD_BVP_S2P_PTR (theBVP) = (INT *)
                             GetFreelistMemory (Heap, (1 + STD_BVP_NSUBDOM (theBVP)) * sizeof (INT));
  if (STD_BVP_S2P_PTR (theBVP) == NULL)
    REP_ERR_RETURN_PTR (NULL);
  STD_BVP_S2P_PTR (theBVP)[0] = 0;
  STD_BVP_S2P_PTR (theBVP)[1] = 0;


  PRINTDEBUG (dom, 1,
              ("C: nCorners %d nBndP %d nElem %d nPPatch %d nTPatch %d nFound %d\n",
               nCorners, nBndP, nElem, nPPatch, nTPatch, nFound));

  return ((BVP *) theBVP);
}

static INT
M_BNDP_Global (BNDP * bp, DOUBLE * global)
{
  M_BNDP *p = (M_BNDP *) bp;
  INT j;

  for (j = 0; j < DIM; j++)
    global[j] = p->pos[j];

  return (0);
}

static INT
M_BNDP_Move (BNDP * aBndP, const DOUBLE global[])
{
  printf ("not implemented line %d\n", __LINE__);

  return (1);
}

static INT
M_BNDP_BndCond (BNDP * theBndP, INT * n, INT i,
                DOUBLE * in, DOUBLE * value, INT * type)
{
  printf ("not implemented line %d\n", __LINE__);

  return (1);
}

static INT
M_BNDP_BndPDesc (BNDP * theBndP, INT * move, INT * part)
{
  *move = 0;
  *part = 0;

  return (0);
}

static INT
M_BNDP_BndEDesc (BNDP * theBndP0, BNDP * theBndP1, INT * part)
{
  *part = 0;

  return (0);
}

static BNDS *
M_BNDP_CreateBndS (HEAP * Heap, BNDP ** theBndP, INT n)
{
  M_BNDS *p = (M_BNDS *) GetFreelistMemory (Heap, M_BNDS_NSIZE (n));
  INT i, j;

  for (i = 0; i < n; i++)
  {
    M_BNDP *b = (M_BNDP *) theBndP[i];
    p->p[i].patch_id = b->patch_id;
    for (j = 0; j < DIM; j++)
      p->p[i].pos[j] = b->pos[j];
  }
  p->n = n;

  return ((BNDS *) p);
}

#ifdef __THREEDIM__
/*#define CYLINDER */
#define BENCHMARK
#endif

static BNDP *
M_BNDP_CreateBndP (HEAP * Heap, BNDP * theBndP0,
                   BNDP * theBndP1, DOUBLE lcoord)
{
  M_BNDP *p0 = (M_BNDP *) theBndP0;
  M_BNDP *p1 = (M_BNDP *) theBndP1;
  M_BNDP *p = (M_BNDP *) GetFreelistMemory (Heap, sizeof (M_BNDP));
  INT j;

  ASSERT (p != NULL);

  for (j = 0; j < DIM; j++)
    p->pos[j] = (1.0 - lcoord) * p0->pos[j] + lcoord * p1->pos[j];
#ifdef CYLINDER
  {
    DOUBLE r0 = p0->pos[0] * p0->pos[0] + p0->pos[1] * p0->pos[1];
    DOUBLE r1 = p1->pos[0] * p1->pos[0] + p1->pos[1] * p1->pos[1];

    if ((r1 > 0.01) && (ABS (r1 - r0) < 0.01))
    {
      DOUBLE r = p->pos[0] * p->pos[0] + p->pos[1] * p->pos[1];
      DOUBLE s = sqrt (r1 / r);

      p->pos[0] *= s;
      p->pos[1] *= s;
    }
  }
#endif
#ifdef BENCHMARK
  {
    DOUBLE r0 =
      (p0->pos[0] - 10) * (p0->pos[0] - 10) + p0->pos[1] * p0->pos[1];
    DOUBLE r1 =
      (p1->pos[0] - 10) * (p1->pos[0] - 10) + p1->pos[1] * p1->pos[1];

    if ((r1 > 0.01) && (ABS (r1 - r0) < 0.01))
    {
      DOUBLE r =
        (p->pos[0] - 10) * (p->pos[0] - 10) + p->pos[1] * p->pos[1];
      DOUBLE s = sqrt (r1 / r);

      p->pos[0] = 10 + s * (p->pos[0] - 10);
      p->pos[1] *= s;
    }
  }
#endif

  return ((BNDP *) p);
}

static INT
M_BNDP_Dispose (HEAP * Heap, BNDP * theBndP)
{
  return (PutFreelistMemory (Heap, theBndP, sizeof (M_BNDP)));
}

static INT
M_BNDP_SaveBndP (BNDP * theBndP)
{
  M_BNDP *p = (M_BNDP *) theBndP;
  INT j;
  int iList[1];
  double dList[DIM];

  iList[0] = p->patch_id;
  if (Bio_Write_mint (1, iList))
    return (1);

  for (j = 0; j < DIM; j++)
    dList[j] = p->pos[j];
  if (Bio_Write_mdouble (DIM, dList))
    return (1);

  return (0);
}

static INT
M_BNDP_SaveInsertedBndP (BNDP * theBndP, char *data, INT max_data_size)
{
  printf ("not implemented line %d\n", __LINE__);

  return (1);
}

static BNDP *
M_BNDP_LoadBndP (BVP * theBVP, HEAP * Heap)
{
  M_BNDP *p = (M_BNDP *) GetFreelistMemory (Heap, sizeof (M_BNDP));
  INT j;
  int iList[1];
  double dList[DIM];

  if (Bio_Read_mint (1, iList))
    return (NULL);
  p->patch_id = iList[0];
  if (Bio_Read_mdouble (DIM, dList))
    return (NULL);

  for (j = 0; j < DIM; j++)
    p->pos[j] = dList[j];

  return ((BNDP *) p);
}

static INT
M_BNDS_Global (BNDS * theBndS, DOUBLE * local, DOUBLE * global)
{
  M_BNDS *p = (M_BNDS *) theBndS;
  INT i;

#ifdef __TWODIM__
  for (i = 0; i < DIM; i++)
    global[i] = (1.0 - local[0]) * p->p[0].pos[i] + local[0] * p->p[1].pos[i];
#endif
#ifdef __THREEDIM__
  if (p->n == 4)
    for (i = 0; i < DIM; i++)
      global[i] = (1.0 - local[0]) * (1.0 - local[1]) * p->p[0].pos[i]
                  + local[0] * (1.0 - local[1]) * p->p[1].pos[i]
                  + local[0] * local[1] * p->p[2].pos[i]
                  + (1.0 - local[0]) * local[1] * p->p[3].pos[i];
  else
  if (p->n == 3)
    for (i = 0; i < DIM; i++)
      global[i] = (1.0 - local[0] - local[1]) * p->p[0].pos[i]
                  + local[0] * p->p[1].pos[i] + local[1] * p->p[2].pos[i];
#endif

#ifdef CYLINDER
  {
    DOUBLE r[4];
    INT flag = 1;

    if (p->n != 4)
      return (0);
    for (i = 0; i < p->n; i++)
    {
      r[i] =
        p->p[i].pos[0] * p->p[i].pos[0] + p->p[i].pos[1] * p->p[i].pos[1];
      if (i == 0)
        if (r[0] < 0.01)
          flag = 0;
      if (i > 0)
        if (ABS (r[i] - r[0]) > 0.01)
          flag = 0;
    }
    if (flag == 0)
    {
      flag = 1;
      for (i = 1; i < p->n; i++)
        if (ABS (p->p[i].pos[2] - p->p[0].pos[2]) > 0.01)
          flag = 0;
    }
    if (flag)
    {
      DOUBLE q = global[0] * global[0] + global[1] * global[1];

      if (q > 0.000001)
      {
        DOUBLE s = sqrt (0.25 * (r[0] + r[1] + r[2] + r[3]) / q);

        global[0] *= s;
        global[1] *= s;
      }
    }
  }
#endif
#ifdef BENCHMARK
  {
    DOUBLE r[4];
    INT flag = 1;

    if (p->n != 4)
      return (0);
    for (i = 0; i < p->n; i++)
    {
      r[i] = (p->p[i].pos[0] - 10) * (p->p[i].pos[0] - 10)
             + p->p[i].pos[1] * p->p[i].pos[1];

      if (i == 0)
      {
        if (r[0] < 0.99)
          flag = 0;
        if (r[0] > 1.01)
          flag = 0;
      }
      if (i > 0)
        if (ABS (r[i] - r[0]) > 0.01)
          flag = 0;
    }
    if (flag)
    {
      DOUBLE q = (global[0] - 10) *(global[0] - 10) +global[1] * global[1];

      if (q > 0.000001)
      {
        DOUBLE s = sqrt (0.25 * (r[0] + r[1] + r[2] + r[3]) / q);

        global[0] = 10 + s * (global[0] - 10);
        global[1] *= s;
      }
    }
  }
#endif

  return (0);
}

static INT
M_BNDS_BndCond (BNDS * theBndS, DOUBLE * local,
                DOUBLE * in, DOUBLE * value, INT * type)
{
  M_BNDS *p = (M_BNDS *) theBndS;

  ASSERT (p != NULL);

  if (currBVP->GeneralBndCond != NULL)
  {
    DOUBLE_VECTOR global;
    INT i;
    M2_PATCH *patch = (M2_PATCH *) currBVP->patches[p->patch_id];

    type[0] = patch->c;
    M_BNDS_Global (theBndS, local, global);

    if (in == NULL)
      return ((*(currBVP->GeneralBndCond))
                (NULL, NULL, global, value, type));

    for (i = 0; i < DIM; i++)
      in[i] = global[i];

    return ((*(currBVP->GeneralBndCond))(NULL, NULL, in, value, type));
  }

  printf ("not implemented line %d\n", __LINE__);

  return (1);
}

static INT
M_BNDS_BndSDesc (BNDS * theBndS, INT * id, INT * nbid, INT * part)
{
  *id = 1;
  *nbid = 0;
  *part = 0;

  return (0);
}

static BNDP *
M_BNDS_CreateBndP (HEAP * Heap, BNDS * theBndS, DOUBLE * local)
{
  M_BNDS *ps = (M_BNDS *) theBndS;
  M_BNDP *p = (M_BNDP *) GetFreelistMemory (Heap, sizeof (M_BNDP));
  INT i;

  ASSERT (ps != NULL);
  /*	ASSERT(ps->n == 4); */
  ASSERT (p != NULL);

  M_BNDS_Global (theBndS, local, p->pos);
  p->patch_id = ps->patch_id;

  return ((BNDP *) p);
}

static INT
M_BNDS_Dispose (HEAP * Heap, BNDS * theBndS)
{
  return (PutFreelistMemory (Heap, theBndS, M_BNDS_SIZE (theBndS)));
}


/****************************************************************************/
/** \brief Create a new PROBLEM structure
 *
 * @param  domain - name of the domain
 * @param  name - name of the problem
 * @param  id - identification number for the problem
 * @param  config - pointer to the configuration function
 * @param  numOfCoefficients - number of coefficient functions
 * @param  coeffs[] - pointer to coefficient functions
 * @param  numOfUserFct - number of user coefficient functions
 * @param  userfct[] - pointer to user coefficient functions
 *
 * This function allocates and initializes a new PROBLEM structure in the /domains/domain directory.
 *
 * @return <ul>
 *   <li>    pointer to new PROBLEM </li>
 *   <li>    NULL if out of memory. </li>
 * </ul>
 */
/****************************************************************************/

PROBLEM *
CreateProblem (char *domain, char *name, int id, ConfigProcPtr config,
               int numOfCoefficients, CoeffProcPtr coeffs[],
               int numOfUserFct, UserProcPtr userfct[])
{
  PROBLEM *newProblem;
  int i;

  if (ChangeEnvDir ("/Domains") == NULL)
    return (NULL);
  if (ChangeEnvDir (domain) == NULL)
    return (NULL);

  /* allocate new problem structure */
  newProblem =
    (PROBLEM *) MakeEnvItem (name, theProblemDirID,
                             sizeof (PROBLEM) + (numOfCoefficients +
                                                 numOfUserFct -
                                                 1) * sizeof (void *));
  if (newProblem == NULL)
    return (NULL);

  /* fill in data */
  newProblem->problemID = id;
  newProblem->ConfigProblem = config;
  newProblem->numOfCoeffFct = numOfCoefficients;
  newProblem->numOfUserFct = numOfUserFct;
  for (i = 0; i < numOfCoefficients; i++)
    newProblem->CU_ProcPtr[i] = (void *) (coeffs[i]);
  for (i = 0; i < numOfUserFct; i++)
    newProblem->CU_ProcPtr[i + numOfCoefficients] = (void *) (userfct[i]);

  if (ChangeEnvDir (name) == NULL)
    return (NULL);
  UserWrite ("problem ");
  UserWrite (name);
  UserWrite (" installed\n");

  return (newProblem);
}

/****************************************************************************/
/** \brief Get pointer to a problem structure
 *
 * @param  domain - name of the domain
 * @param  name - name of the problem
 *
 * This function searches a problem structure in the /domains/domain directory.
 *
 * @return <ul>
 *   <li>      pointer to PROBLEM </li>
 *   <li>      NULL if not found or error </li>
 * </ul>
 */
/****************************************************************************/

static PROBLEM *
GetProblem (const char *domain, const char *name)
{
  if (ChangeEnvDir ("/Domains") == NULL)
    return (NULL);
  return ((PROBLEM *)
          SearchEnv (name, domain, theProblemDirID, theDomainDirID));
}

/****************************************************************************/
/** \brief Allocate a new BNDCOND structure
 *
 * @param  name - name of the boundary condition
 * @param  id - identification number of condition
 * @param  theBndCond - the boundary conditions
 * @param  Data - user defined data
 *
 * This function allocates and initializes a new BNDCOND structure in the previously allocated
 * PROBLEM structure
 *
 * @return <ul>
 *   <li>      pointer to </li>
 *   <li>      0 if  out of memory. </li>
 * </ul>
 */
/****************************************************************************/

BOUNDARY_CONDITION * NS_PREFIX
CreateBoundaryCondition (char *name, INT id, BndCondProcPtr theBndCond,
                         void *Data)
{
  BOUNDARY_CONDITION *newBndCond;

  /* allocate boundary condition */
  newBndCond =
    (BOUNDARY_CONDITION *) MakeEnvItem (name, theBdryCondVarID,
                                        sizeof (BOUNDARY_CONDITION));
  if (newBndCond == NULL)
    return (NULL);

  /* fill in data */
  newBndCond->id = id;
  newBndCond->BndCond = theBndCond;
  newBndCond->data = Data;

  return (newBndCond);
}

/****************************************************************************/
/** \brief Allocate a new BNDCOND structure
 *
 * @param  theBCond - the boundary condition
 *
 * This function gets next BNDCOND structure
 *
 * @return <ul>
 *   <li>      pointer to BOUNDARY_CONDITION </li>
 *   <li>      NULL if not found or error. </li>
 * </ul>
 */
/****************************************************************************/
static BOUNDARY_CONDITION *
GetNextBoundaryCondition (BOUNDARY_CONDITION * theBCond)
{
  ENVITEM *theItem;

  theItem = (ENVITEM *) theBCond;

  do
    theItem = NEXT_ENVITEM (theItem);
  while ((theItem != NULL) && (ENVITEM_TYPE (theItem) != theBdryCondVarID));

  return ((BOUNDARY_CONDITION *) theItem);
}

/****************************************************************************/
/** \brief Get first BNDCOND structure of `theProblem`
 *
 * @param  theProblem - pointer to PROBLEM
 *
 * This function gets the first BNDCOND structure of a problem
 *
 * @return <ul>
 *   <li>      pointer to BOUNDARY_CONDITION </li>
 *   <li>      NULL if not found or error. </li>
 * </ul>
 */
/****************************************************************************/
static BOUNDARY_CONDITION *
GetFirstBoundaryCondition (PROBLEM * theProblem)
{
  ENVITEM *theItem;

  theItem = ENVITEM_DOWN (theProblem);

  if (ENVITEM_TYPE (theItem) == theBdryCondVarID)
    return ((BOUNDARY_CONDITION *) theItem);
  else
    return (GetNextBoundaryCondition ((BOUNDARY_CONDITION *) theItem));
}

/****************************************************************************/
/** \brief Create a new DOMAIN data structure with part description
 *
 * @param  name - name of the domain
 * @param  MidPoint - coordinates of some inner point
 * @param  radius - radius of a circle, containing the domain
 * @param  segments - number of the boundary segments
 * @param  corners - number of corners
 * @param  Convex - 0, if convex, 1 - else
 * @param  nParts - number of parts in the domain
 * @param  dpi - description of the parts for lines, segments, points
 *
 * This function allocates and initializes a new DOMAIN data structure in the
 * /domains directory in the environment.
 * Additinally domain parts will defined.
 *
 * @return <ul>
 *   <li>     pointer to a DOMAIN </li>
 *   <li>     NULL if out of memory. </li>
 * </ul>
 */
/****************************************************************************/

DOMAIN * NS_PREFIX
CreateDomainWithParts (char *name, DOUBLE * MidPoint, DOUBLE radius,
                       INT segments, INT corners, INT Convex, INT nParts,
                       const DOMAIN_PART_INFO * dpi)
{
  DOMAIN *newDomain;
  INT i;

  /* change to /domains directory */
  if (ChangeEnvDir ("/Domains") == NULL)
    return (NULL);

  /* allocate new domain structure */
  newDomain = (DOMAIN *) MakeEnvItem (name, theDomainDirID, sizeof (DOMAIN));
  if (newDomain == NULL)
    return (NULL);

  /* fill in data */
  for (i = 0; i < DIM; i++)
    DOMAIN_MIDPOINT (newDomain)[i] = MidPoint[i];
  DOMAIN_RADIUS (newDomain) = radius;
  DOMAIN_NSEGMENT (newDomain) = segments;
  DOMAIN_NCORNER (newDomain) = corners;
  DOMAIN_CONVEX (newDomain) = Convex;
  DOMAIN_NPARTS (newDomain) = nParts;
  DOMAIN_PARTINFO (newDomain) = dpi;

  if (ChangeEnvDir (name) == NULL)
    return (NULL);
  UserWrite ("domain ");
  UserWrite (name);
  UserWrite (" installed\n");

  return (newDomain);
}

/****************************************************************************/
/** \brief Create a new DOMAIN data structure
 *
 * @param  name - name of the domain
 * @param  MidPoint - coordinates of some inner point
 * @param  radius - radius of a circle, containing the domain
 * @param  segments - number of the boundary segments
 * @param  corners - number of corners
 * @param  Convex - 0, if convex, 1 - else
 *
 * This function allocates and initializes a new DOMAIN data structure in the
 * /domains directory in the environment.
 *
 * @return <ul>
 *   <li>     pointer to a DOMAIN </li>
 *   <li>     NULL if out of memory. </li>
 * </ul>
 */
/****************************************************************************/

DOMAIN *NS_PREFIX
CreateDomain (char *name, DOUBLE * MidPoint, DOUBLE radius, INT segments,
              INT corners, INT Convex)
{
  return (CreateDomainWithParts
            (name, MidPoint, radius, segments, corners, Convex, 1, NULL));
}

/****************************************************************************/
/** \brief Get a pointer to a domain structure
 *
 * @param  name - name of the domain
 *
 * This function searches the environment for a domain with the name `name`
 * and return a pointer to the domain structure.
 *
 * @return <ul>
 *   <li>     pointer to a DOMAIN </li>
 *   <li>     NULL if not found or error.  </li>
 * </ul>
 */
/****************************************************************************/

DOMAIN *NS_PREFIX
GetDomain (char *name)
{
  return ((DOMAIN *)
          SearchEnv (name, "/Domains", theDomainDirID, theDomainDirID));
}

/****************************************************************************/
/** \brief Create a new BOUNDARY_SEGMENT
 *
 * @param  name - name of the boundary segment
 * @param  left - id of left subdomain
 * @param  right - id of right subdomain
 * @param  id - id of this boundary segment
 * @param  type - type of the boundary segment
 * @param  res  - resolution of the boundary segment
 * @param  point - the endpoints of the boundary segment
 * @param  alpha - list where the parameter interval begins
 * @param  beta - list where the parameter interval ends
 * @param  BndSegFunc - function mapping parameters
 * @param  data - user defined space
 *
 * This function allocates and initializes a new BOUNDARY_SEGMENT for the previously
 * allocated DOMAIN structure.
 *
 * @return <ul>
 *   <li>     Pointer to a BOUNDARY_SEGMENT </li>
 *   <li>     NULL if out of memory.     </li>
 * </ul>
 */
/****************************************************************************/

BOUNDARY_SEGMENT *NS_PREFIX
CreateBoundarySegment (char *name,
                       INT left, INT right, INT id, INT type, INT res,
                       INT * point, DOUBLE * alpha, DOUBLE * beta,
                       BndSegFuncPtr BndSegFunc, void *data)
{
  BOUNDARY_SEGMENT *newSegment;
  INT i;

  /* allocate the boundary segment */
  newSegment =
    (BOUNDARY_SEGMENT *) MakeEnvItem (name, theBdrySegVarID,
                                      sizeof (BOUNDARY_SEGMENT));
  if (newSegment == NULL)
    return (NULL);

  /* fill in data */
  newSegment->left = left;
  newSegment->right = right;
  newSegment->id = id;
  newSegment->segType = type;
  for (i = 0; i < CORNERS_OF_BND_SEG; i++)
    newSegment->points[i] = point[i];
  newSegment->resolution = res;
  for (i = 0; i < DIM_OF_BND; i++)
  {
    newSegment->alpha[i] = alpha[i];
    newSegment->beta[i] = beta[i];
  }
  newSegment->BndSegFunc = BndSegFunc;
  newSegment->data = data;

  return (newSegment);
}

/****************************************************************************/
/** \brief Get next boundary segment of a domain
 *
 * @param  theBSeg - pointer to a boundary segment
 *
 * This function gets the next boundary segment of a domain.
 *
 * @return <ul>
 *   <li>     pointer to next BOUNDARY_SEGMENT </li>
 *   <li>     NULL if no more found.  </li>
 * </ul>
 */
/****************************************************************************/

static BOUNDARY_SEGMENT *
GetNextBoundarySegment (BOUNDARY_SEGMENT * theBSeg)
{
  ENVITEM *theItem;

  theItem = (ENVITEM *) theBSeg;

  do
    theItem = NEXT_ENVITEM (theItem);
  while ((theItem != NULL) && (ENVITEM_TYPE (theItem) != theBdrySegVarID));

  return ((BOUNDARY_SEGMENT *) theItem);
}

/****************************************************************************/
/** \brief Get first boundary segment of a domain
 *
 * @param  theDomain - pointer to the domain structure
 *
 * This function returns the first boundary segment of a domain.
 *
 * @return <ul>
 *   <li>     pointer to a BOUNDARY_SEGMENT </li>
 *   <li>     NULL if not found or error.  </li>
 * </ul>
 */
/****************************************************************************/

static BOUNDARY_SEGMENT *
GetFirstBoundarySegment (DOMAIN * theDomain)
{
  ENVITEM *theItem;

  theItem = ENVITEM_DOWN (theDomain);

  if (ENVITEM_TYPE (theItem) == theBdrySegVarID)
    return ((BOUNDARY_SEGMENT *) theItem);
  else
    return (GetNextBoundarySegment ((BOUNDARY_SEGMENT *) theItem));
}

/****************************************************************************/
/* \brief ???   (This function is here for upward compatibility)
 *
 * \todo Do we still need this?
 *
 * @param  name - name of the boundary segment
 * @param  left - id of the left subdomain
 * @param  right - id of the right subdomain
 * @param  id -
 * @param  from -
 * @param  to -
 * @param  res -
 * @param  alpha -
 * @param  beta -
 * @param  BndSegFunc -
 * @param  data -
 *
 *  This function defines CreateBoundarySegment2D for old style 2D definitions
 *  (they were not dimension independent)
 *
 * @return <ul>
 *   <li>     pointer to </li>
 *   <li>     NULL if object not available.	 </li>
 * </ul>
 */
/****************************************************************************/

BOUNDARY_SEGMENT *NS_PREFIX
CreateBoundarySegment2D (char *name, int left, int right,
                         int id, int from, int to, int res, DOUBLE alpha,
                         DOUBLE beta, BndSegFuncPtr BndSegFunc, void *data)
{
  INT pt[3];
  DOUBLE alp[3], bet[3];

  pt[0] = from;
  pt[1] = to;
  alp[0] = alpha;
  bet[0] = beta;

  return (CreateBoundarySegment (name, left, right, id, NON_PERIODIC,
                                 res, pt, alp, bet, BndSegFunc, data));
}

/****************************************************************************/
/** \brief  Create a new LINEAR_SEGMENT
 *
 * @param  name - name of the boundary segment
 * @param  left - id of left subdomain
 * @param  right - id of right subdomain
 * @param  id - id of this boundary segment
 * @param  n - number of corners
 * @param  point - the endpoints of the boundary segment
 * @param  x - coordinates
 *
 * This function allocates and initializes a new LINEAR_SEGMENT
 * for the previously allocated DOMAIN structure.
 *
 * @return <ul>
 *   <li>     Pointer to a LINEAR_SEGMENT </li>
 *   <li>     NULL if out of memory. </li>
 * </ul>
 */
/****************************************************************************/

LINEAR_SEGMENT *NS_PREFIX
CreateLinearSegment (char *name,
                     INT left, INT right, INT id,
                     INT n, INT * point,
                     DOUBLE x[MAX_CORNERS_OF_LINEAR_PATCH][DIM])
{
  LINEAR_SEGMENT *newSegment;
  INT i, k;

  if (n > MAX_CORNERS_OF_LINEAR_PATCH)
    return (NULL);

  /* allocate the boundary segment */
  newSegment = (LINEAR_SEGMENT *)
               MakeEnvItem (name, theLinSegVarID, sizeof (LINEAR_SEGMENT));

  if (newSegment == NULL)
    return (NULL);

  /* fill in data */
  newSegment->left = left;
  newSegment->right = right;
  newSegment->id = id;
  newSegment->n = n;
  for (i = 0; i < n; i++)
  {
    newSegment->points[i] = point[i];
    for (k = 0; k < DIM_OF_BND; k++)
      newSegment->x[i][k] = x[i][k];
  }

  return (newSegment);
}

static LINEAR_SEGMENT *
GetNextLinearSegment (LINEAR_SEGMENT * theBSeg)
{
  ENVITEM *theItem;

  theItem = (ENVITEM *) theBSeg;

  do
    theItem = NEXT_ENVITEM (theItem);
  while ((theItem != NULL) && (ENVITEM_TYPE (theItem) != theLinSegVarID));

  return ((LINEAR_SEGMENT *) theItem);
}

static LINEAR_SEGMENT *
GetFirstLinearSegment (DOMAIN * theDomain)
{
  ENVITEM *theItem;

  theItem = ENVITEM_DOWN (theDomain);

  if (ENVITEM_TYPE (theItem) == theLinSegVarID)
    return ((LINEAR_SEGMENT *) theItem);
  else
    return (GetNextLinearSegment ((LINEAR_SEGMENT *) theItem));
}


BVP *NS_PREFIX
Create_MarcBVP (char *BVPName, BndCondProcPtr theBndCond,
                int numOfCoeffFct, CoeffProcPtr coeffs[],
                int numOfUserFct, UserProcPtr userfct[])
{
  STD_BVP *theBVP;
  INT i, n;

  /* change to /BVP directory */
  if (ChangeEnvDir ("/BVP") == NULL)
    return (NULL);

  /* allocate new domain structure */
  n = (numOfCoeffFct + numOfUserFct - 1) * sizeof (void *);
  theBVP =
    (STD_BVP *) MakeEnvItem (BVPName, theBVPDirID, sizeof (STD_BVP) + n);
  if (theBVP == NULL)
    return (NULL);
  if (ChangeEnvDir (BVPName) == NULL)
    return (NULL);

  theBVP->numOfCoeffFct = numOfCoeffFct;
  theBVP->numOfUserFct = numOfUserFct;
  for (i = 0; i < numOfCoeffFct; i++)
    theBVP->CU_ProcPtr[i] = (void *) (coeffs[i]);
  for (i = 0; i < numOfUserFct; i++)
    theBVP->CU_ProcPtr[i + numOfCoeffFct] = (void *) (userfct[i]);

  STD_BVP_S2P_PTR (theBVP) = NULL;

  theBVP->Domain = NULL;
  theBVP->Problem = NULL;
  theBVP->ConfigProc = Marc_Configure;
  theBVP->GeneralBndCond = theBndCond;

  theBVP->type = BVP_MARC;

  UserWriteF ("BVP %s installed.\n", BVPName);

  return ((BVP *) theBVP);
}

BVP *NS_PREFIX
CreateBoundaryValueProblem (char *BVPName, BndCondProcPtr theBndCond,
                            int numOfCoeffFct, CoeffProcPtr coeffs[],
                            int numOfUserFct, UserProcPtr userfct[])
{
  STD_BVP *theBVP;
  INT i, n;

  /* change to /BVP directory */
  if (ChangeEnvDir ("/BVP") == NULL)
    return (NULL);

  /* allocate new domain structure */
  n = (numOfCoeffFct + numOfUserFct - 1) * sizeof (void *);
  theBVP =
    (STD_BVP *) MakeEnvItem (BVPName, theBVPDirID, sizeof (STD_BVP) + n);
  if (theBVP == NULL)
    return (NULL);
  if (ChangeEnvDir (BVPName) == NULL)
    return (NULL);

  theBVP->numOfCoeffFct = numOfCoeffFct;
  theBVP->numOfUserFct = numOfUserFct;
  for (i = 0; i < numOfCoeffFct; i++)
    theBVP->CU_ProcPtr[i] = (void *) (coeffs[i]);
  for (i = 0; i < numOfUserFct; i++)
    theBVP->CU_ProcPtr[i + numOfCoeffFct] = (void *) (userfct[i]);

  STD_BVP_S2P_PTR (theBVP) = NULL;

  theBVP->Domain = NULL;
  theBVP->Problem = NULL;
  theBVP->ConfigProc = STD_BVP_Configure;
  theBVP->GeneralBndCond = theBndCond;

  theBVP->type = BVP_STANDARD;

  UserWriteF ("BVP %s installed.\n", BVPName);

  return ((BVP *) theBVP);
}

/****************************************************************************/
/** \brief Create BoundaryValueProblem from Domain and Problem
 *
 * @param  BVP - name of BVP to be created
 * @param  Domain - name of domain
 * @param  Problem - name of Problem
 *
 * @return <ul>
 *   <li>     pointer BoundaryValueProblem </li>
 *   <li>     NULL if not creatable.  </li>
 * </ul>
 *//****************************************************************************/

BVP *NS_PREFIX
CreateBVP (char *BVPName, char *DomainName, char *ProblemName)
{
  STD_BVP *theBVP;
  DOMAIN *theDomain;
  PROBLEM *theProblem;
  INT i, n;

  /* get domain and problem */
  theDomain = GetDomain (DomainName);
  if (theDomain == NULL)
    return (NULL);
  theProblem = GetProblem (DomainName, ProblemName);
  if (theProblem == NULL)
    return (NULL);

  /* change to /BVP directory */
  if (ChangeEnvDir ("/BVP") == NULL)
    return (NULL);

  /* allocate new domain structure */
  n =
    (theProblem->numOfCoeffFct + theProblem->numOfUserFct -
     1) * sizeof (void *);
  theBVP =
    (STD_BVP *) MakeEnvItem (BVPName, theBVPDirID, sizeof (STD_BVP) + n);
  if (theBVP == NULL)
    return (NULL);
  if (ChangeEnvDir (BVPName) == NULL)
    return (NULL);
  for (i = 0; i < theProblem->numOfCoeffFct; i++)
    theBVP->CU_ProcPtr[i] = theProblem->CU_ProcPtr[i];
  for (i = 0; i < theProblem->numOfUserFct; i++)
    theBVP->CU_ProcPtr[i + theProblem->numOfCoeffFct] =
      theProblem->CU_ProcPtr[i + theProblem->numOfCoeffFct];
  theBVP->numOfCoeffFct = theProblem->numOfCoeffFct;
  theBVP->numOfUserFct = theProblem->numOfUserFct;
  STD_BVP_S2P_PTR (theBVP) = NULL;

  theBVP->Domain = theDomain;
  theBVP->Problem = theProblem;

  /* fill in data of problem */
  theBVP->ConfigProc = theProblem->ConfigProblem;
  theBVP->GeneralBndCond = NULL;

  theBVP->type = BVP_STANDARD;

  UserWriteF ("BVP %s installed.\n", BVPName);

  return ((BVP *) theBVP);
}

static INT
Problem_Configure (INT argc, char **argv)
{
  DOMAIN *theDomain;
  PROBLEM *theProblem;
  BOUNDARY_CONDITION *theBndCond;
  PATCH *thePatch;
  INT n;
  char ProblemName[NAMESIZE];
  INT i;

  for (i = 0; i < argc; i++)
    if ((argv[i][0] == 'p') && (argv[i][1] == ' '))
      if ((sscanf (argv[i], expandfmt (CONCAT3 ("p %", NAMELENSTR, "[ -~]")),
                   ProblemName) != 1) || (strlen (ProblemName) == 0))
        continue;

  theDomain = currBVP->Domain;
  if (theDomain == NULL)
    return (1);
  theProblem = GetProblem (ENVITEM_NAME (theDomain), ProblemName);
  if (theProblem == NULL)
    return (1);
  if (currBVP->numOfCoeffFct < theProblem->numOfCoeffFct)
    return (1);
  if (currBVP->numOfUserFct < theProblem->numOfUserFct)
    return (1);
  for (i = 0; i < theProblem->numOfCoeffFct; i++)
    currBVP->CU_ProcPtr[i] = theProblem->CU_ProcPtr[i];
  for (i = 0; i < theProblem->numOfUserFct; i++)
    currBVP->CU_ProcPtr[i + theProblem->numOfCoeffFct] =
      theProblem->CU_ProcPtr[i + theProblem->numOfCoeffFct];
  currBVP->Problem = theProblem;
  n = currBVP->sideoffset;
  for (theBndCond = GetFirstBoundaryCondition (theProblem);
       theBndCond != NULL; theBndCond = GetNextBoundaryCondition (theBndCond))
  {
    thePatch = currBVP->patches[n];
    assert (n - currBVP->sideoffset == theBndCond->id);
    PARAM_PATCH_BC (thePatch) = theBndCond->BndCond;
    PARAM_PATCH_BCD (thePatch) = theBndCond->data;
    n++;
  }

  UserWriteF ("%s configured with problem %s\n",
              ENVITEM_NAME (currBVP), ProblemName);

  return (0);
}

BVP *NS_PREFIX
CreateBVP_Problem (char *BVPName, char *DomainName, char *ProblemName)
{
  STD_BVP *theBVP;
  DOMAIN *theDomain;
  PROBLEM *theProblem;
  INT i, n;

  /* get domain and problem */
  theDomain = GetDomain (DomainName);
  if (theDomain == NULL)
    return (NULL);
  theProblem = GetProblem (DomainName, ProblemName);
  if (theProblem == NULL)
    return (NULL);

  /* change to /BVP directory */
  if (ChangeEnvDir ("/BVP") == NULL)
    return (NULL);

  /* allocate new domain structure */
  n =
    (theProblem->numOfCoeffFct + theProblem->numOfUserFct -
     1) * sizeof (void *);
  theBVP =
    (STD_BVP *) MakeEnvItem (BVPName, theBVPDirID, sizeof (STD_BVP) + n);
  if (theBVP == NULL)
    return (NULL);
  if (ChangeEnvDir (BVPName) == NULL)
    return (NULL);
  for (i = 0; i < theProblem->numOfCoeffFct; i++)
    theBVP->CU_ProcPtr[i] = theProblem->CU_ProcPtr[i];
  for (i = 0; i < theProblem->numOfUserFct; i++)
    theBVP->CU_ProcPtr[i + theProblem->numOfCoeffFct] =
      theProblem->CU_ProcPtr[i + theProblem->numOfCoeffFct];
  theBVP->numOfCoeffFct = theProblem->numOfCoeffFct;
  theBVP->numOfUserFct = theProblem->numOfUserFct;
  STD_BVP_S2P_PTR (theBVP) = NULL;

  theBVP->Domain = theDomain;
  theBVP->Problem = theProblem;

  /* fill in data of problem */
  theBVP->ConfigProc = Problem_Configure;
  theBVP->GeneralBndCond = NULL;

  theBVP->type = BVP_STANDARD;

  UserWriteF ("BVP %s installed.\n", BVPName);

  return ((BVP *) theBVP);
}

const char *NS_PREFIX
GetBVP_DomainName (const BVP * aBVP)
{
  const STD_BVP *theBVP = (const STD_BVP *) aBVP;

  if (theBVP == NULL)
    return NULL;

  return ENVITEM_NAME (theBVP->Domain);
}

const char *NS_PREFIX
GetBVP_ProblemName (const BVP * aBVP)
{
  const STD_BVP *theBVP = (const STD_BVP *) aBVP;

  if (theBVP == NULL)
    return NULL;

  return ENVITEM_NAME (theBVP->Problem);
}

static INT
GetNumberOfPatches (PATCH * p)
{
  switch (PATCH_TYPE (p))
  {
  case PARAMETRIC_PATCH_TYPE :
  case LINEAR_PATCH_TYPE :
    return (1);
  case POINT_PATCH_TYPE :
    return (POINT_PATCH_N (p));
#ifdef __THREEDIM__
  case LINE_PATCH_TYPE :
    return (LINE_PATCH_N (p));
#endif
  }

  return (-1);
}

static INT
GetPatchId (PATCH * p, INT i)
{
  switch (PATCH_TYPE (p))
  {
  case PARAMETRIC_PATCH_TYPE :
    return (PATCH_ID (p));
  case POINT_PATCH_TYPE :
    return (POINT_PATCH_PID (p, i));
#ifdef __THREEDIM__
  case LINE_PATCH_TYPE :
    return (LINE_PATCH_PID (p, i));
#endif
  }

  return (-1);
}

static BNDP *
CreateBndPOnPoint (HEAP * Heap, PATCH * p)
{
  BND_PS *ps;
  PATCH *pp;
  INT j, l, m;

  if (PATCH_TYPE (p) != POINT_PATCH_TYPE)
    return (NULL);

  PRINTDEBUG (dom, 1, ("    p %d\n", PATCH_ID (p)));
  for (l = 0; l < GetNumberOfPatches (p); l++)
    PRINTDEBUG (dom, 1, ("    bp pid %d\n", GetPatchId (p, l)));

  m = POINT_PATCH_N (p);
  ps = (BND_PS *) GetFreelistMemory (Heap, sizeof (BND_PS)
                                     + (m - 1) * sizeof (COORD_BND_VECTOR));
  if (ps == NULL)
    REP_ERR_RETURN (NULL);
  ps->n = m;
  ps->patch_id = PATCH_ID (p);

  for (j = 0; j < m; j++)
  {
    pp = currBVP->patches[POINT_PATCH_PID (p, j)];
    if (PATCH_TYPE (pp) == PARAMETRIC_PATCH_TYPE)
    {
      PRINTDEBUG (dom, 1, ("cp r %f %f %f %f\n",
                           PARAM_PATCH_RANGE (pp)[0][0],
                           PARAM_PATCH_RANGE (pp)[0][1],
                           PARAM_PATCH_RANGE (pp)[1][0],
                           PARAM_PATCH_RANGE (pp)[1][1]));
      switch (POINT_PATCH_CID (p, j))
      {
#ifdef __TWODIM__
      case 0 :
        ps->local[j][0] = PARAM_PATCH_RANGE (pp)[0][0];
        break;
      case 1 :
        ps->local[j][0] = PARAM_PATCH_RANGE (pp)[1][0];
        break;
#endif
#ifdef __THREEDIM__
      case 0 :
        ps->local[j][0] = PARAM_PATCH_RANGE (pp)[0][0];
        ps->local[j][1] = PARAM_PATCH_RANGE (pp)[0][1];
        break;
      case 1 :
        ps->local[j][0] = PARAM_PATCH_RANGE (pp)[1][0];
        ps->local[j][1] = PARAM_PATCH_RANGE (pp)[0][1];
        break;
      case 2 :
        ps->local[j][0] = PARAM_PATCH_RANGE (pp)[1][0];
        ps->local[j][1] = PARAM_PATCH_RANGE (pp)[1][1];
        break;
      case 3 :
        ps->local[j][0] = PARAM_PATCH_RANGE (pp)[0][0];
        ps->local[j][1] = PARAM_PATCH_RANGE (pp)[1][1];
        break;
#endif
      }
      PRINTDEBUG (dom, 1, ("mesh loc j %d pid %d cid %d loc %f %f\n",
                           j,
                           POINT_PATCH_PID (p, j),
                           POINT_PATCH_CID (p, j),
                           ps->local[j][0], ps->local[j][1]));
    }
    else if (PATCH_TYPE (pp) == LINEAR_PATCH_TYPE)
    {
      switch (POINT_PATCH_CID (p, j))
      {
#ifdef __TWODIM__
      case 0 :
        ps->local[j][0] = 0.0;
        break;
      case 1 :
        ps->local[j][0] = 1.0;
        break;
#endif
#ifdef __THREEDIM__
      case 0 :
        ps->local[j][0] = 0.0;
        ps->local[j][1] = 0.0;
        break;
      case 1 :
        ps->local[j][0] = 1.0;
        ps->local[j][1] = 0.0;
        break;
      case 2 :
        ps->local[j][0] = 0.0;
        ps->local[j][1] = 1.0;
        break;
#endif
      }
    }
  }
  if (!PATCH_IS_FIXED (p))
  {
    /* store global coordinates */
    BND_DATA (ps) = GetFreelistMemory (Heap, DIM * sizeof (DOUBLE));
    if (BND_DATA (ps) == NULL)
      REP_ERR_RETURN (NULL);

    if (BndPointGlobal ((BNDP *) ps, (DOUBLE *) BND_DATA (ps)))
      REP_ERR_RETURN (NULL);
  }
  return ((BNDP *) ps);
}

static INT
CreateCornerPoints (HEAP * Heap, STD_BVP * theBVP, BNDP ** bndp)
{
  int i;

  for (i = 0; i < theBVP->ncorners; i++)
  {
    bndp[i] = CreateBndPOnPoint (Heap, theBVP->patches[i]);
    if (bndp[i] == NULL)
      REP_ERR_RETURN (1);
  }

  for (i = 0; i < theBVP->ncorners; i++)
    PRINTDEBUG (dom, 1, (" id %d\n", PATCH_ID (theBVP->patches[i])));

  return (0);
}

BVP *NS_PREFIX
BVP_Init (char *name, HEAP * Heap, MESH * Mesh, INT MarkKey)
{
  STD_BVP *theBVP;
  DOMAIN *theDomain;
  PROBLEM *theProblem;
  BOUNDARY_SEGMENT *theSegment;
  LINEAR_SEGMENT *theLinSegment;
  BOUNDARY_CONDITION *theBndCond;
  PATCH **corners, **sides, *thePatch;
  INT i, j, n, m, maxSubDomains, ncorners, nlines, nsides, free;
#       ifdef __THREEDIM__
  PATCH **lines;
  INT k, err;
#       endif

  theBVP = (STD_BVP *) BVP_GetByName (name);
  if (theBVP == NULL)
    return (NULL);
  currBVP = theBVP;
#ifdef ModelP
  SetBVPType (theBVP->type);
#endif

  if (theBVP->type == BVP_MARC)
    return (Init_MarcBVP (theBVP, Heap, Mesh, MarkKey));

  theDomain = theBVP->Domain;
  if (theDomain == NULL)
    return (NULL);
  theProblem = theBVP->Problem;

  /* fill in data of domain */
  for (i = 0; i < DIM; i++)
    theBVP->MidPoint[i] = theDomain->MidPoint[i];
  theBVP->radius = theDomain->radius;
  theBVP->domConvex = theDomain->domConvex;

  ncorners = theDomain->numOfCorners;
  nsides = theDomain->numOfSegments;

  /* create parameter patches */
  maxSubDomains = 0;
  sides = (PATCH **) GetTmpMem (Heap, nsides * sizeof (PATCH *), MarkKey);
  if (sides == NULL)
    return (NULL);

  for (i = 0; i < nsides; i++)
    sides[i] = NULL;
  theBVP->nsides = nsides;
  for (theSegment = GetFirstBoundarySegment (theDomain); theSegment != NULL;
       theSegment = GetNextBoundarySegment (theSegment))
  {
    if ((theSegment->id < 0) || (theSegment->id >= nsides))
      return (NULL);
    thePatch = (PATCH *) GetFreelistMemory (Heap, sizeof (PARAMETER_PATCH));
    if (thePatch == NULL)
      return (NULL);
    PATCH_TYPE (thePatch) = PARAMETRIC_PATCH_TYPE;
    PATCH_ID (thePatch) = theSegment->id;
    if (theSegment->segType == FREE)
      PATCH_STATE (thePatch) = PATCH_FREE;
    else
      PATCH_STATE (thePatch) = PATCH_FIXED;
    PARAM_PATCH_LEFT (thePatch) = theSegment->left;
    PARAM_PATCH_RIGHT (thePatch) = theSegment->right;
    PARAM_PATCH_BC (thePatch) = NULL;
    PARAM_PATCH_BCD (thePatch) = NULL;
    for (i = 0; i < 2 * DIM_OF_BND; i++)
      PARAM_PATCH_POINTS (thePatch, i) = theSegment->points[i];
    PARAM_PATCH_RES (thePatch) = SEG_RESOLUTION (theSegment);
    for (i = 0; i < DIM_OF_BND; i++)
    {
      PARAM_PATCH_RANGE (thePatch)[0][i] = theSegment->alpha[i];
      PARAM_PATCH_RANGE (thePatch)[1][i] = theSegment->beta[i];
    }
    PARAM_PATCH_BS (thePatch) = theSegment->BndSegFunc;
    PARAM_PATCH_BSD (thePatch) = theSegment->data;
    maxSubDomains = MAX (maxSubDomains, theSegment->left);
    maxSubDomains = MAX (maxSubDomains, theSegment->right);
    sides[theSegment->id] = thePatch;
    PRINTDEBUG (dom, 1, ("sides id %d type %d left %d right %d\n",
                         PATCH_ID (thePatch), PATCH_TYPE (thePatch),
                         PARAM_PATCH_LEFT (thePatch),
                         PARAM_PATCH_RIGHT (thePatch)));
    for (i = 0; i < 2 * DIM_OF_BND; i++)
      PRINTDEBUG (dom, 1,
                  ("   corners %d", PARAM_PATCH_POINTS (thePatch, i)));
    PRINTDEBUG (dom, 1, ("\n"));
  }
  for (theLinSegment = GetFirstLinearSegment (theDomain);
       theLinSegment != NULL;
       theLinSegment = GetNextLinearSegment (theLinSegment))
  {
    if ((theLinSegment->id < 0) || (theLinSegment->id >= nsides))
      return (NULL);
    thePatch = (PATCH *) GetFreelistMemory (Heap, sizeof (LINEAR_PATCH));
    if (thePatch == NULL)
      return (NULL);
    PATCH_TYPE (thePatch) = LINEAR_PATCH_TYPE;
    PATCH_ID (thePatch) = theLinSegment->id;
    LINEAR_PATCH_LEFT (thePatch) = theLinSegment->left;
    LINEAR_PATCH_RIGHT (thePatch) = theLinSegment->right;
    LINEAR_PATCH_N (thePatch) = theLinSegment->n;
    for (j = 0; j < theLinSegment->n; j++)
    {
      LINEAR_PATCH_POINTS (thePatch, j) = theLinSegment->points[j];
      for (i = 0; i < DIM; i++)
        LINEAR_PATCH_POS (thePatch, j)[i] = theLinSegment->x[j][i];
    }
    maxSubDomains = MAX (maxSubDomains, theLinSegment->left);
    maxSubDomains = MAX (maxSubDomains, theLinSegment->right);
    sides[theLinSegment->id] = thePatch;
    PRINTDEBUG (dom, 1, ("sides id %d type %d left %d right %d\n",
                         PATCH_ID (thePatch), PATCH_TYPE (thePatch),
                         LINEAR_PATCH_LEFT (thePatch),
                         LINEAR_PATCH_RIGHT (thePatch)));
    /** \todo why this here??? (CVS-merge mess-up?) */
    if (theProblem != NULL)
    {
      UserWrite ("Use CreateBoundaryValueProblem!");
      return (NULL);
    }
  }
  theBVP->numOfSubdomains = maxSubDomains;
  PRINTDEBUG (dom, 1, (" bvp nsubcf %x\n", theBVP->numOfSubdomains));
  for (i = 0; i < nsides; i++)
    if (sides[i] == NULL)
      return (NULL);

  if (theProblem != NULL)
    for (theBndCond = GetFirstBoundaryCondition (theProblem);
         theBndCond != NULL;
         theBndCond = GetNextBoundaryCondition (theBndCond))
    {
      i = theBndCond->id;
      if ((i < 0) || (i >= nsides))
        return (NULL);
      thePatch = sides[i];
      PARAM_PATCH_BC (thePatch) = theBndCond->BndCond;
      PARAM_PATCH_BCD (thePatch) = theBndCond->data;
    }

  /* create point patches */
  corners = (PATCH **) GetTmpMem (Heap, ncorners * sizeof (PATCH *), MarkKey);
  if (corners == NULL)
    return (NULL);
  theBVP->ncorners = ncorners;
  for (i = 0; i < ncorners; i++)
  {
    m = 0;
    /* count parameter patchs */
    for (j = 0; j < nsides; j++)
    {
      if (PATCH_TYPE (sides[j]) == LINEAR_PATCH_TYPE)
      {
        for (n = 0; n < LINEAR_PATCH_N (sides[j]); n++)
          if (LINEAR_PATCH_POINTS (sides[j], n) == i)
            m++;
      }
      else if (PATCH_TYPE (sides[j]) == PARAMETRIC_PATCH_TYPE)
      {
        for (n = 0; n < 2 * DIM_OF_BND; n++)
          if (PARAM_PATCH_POINTS (sides[j], n) == i)
            m++;
      }
    }
    thePatch =
      (PATCH *) GetFreelistMemory (Heap, sizeof (POINT_PATCH)
                                   + (m -
                                      1) * sizeof (struct point_on_patch));
    if (thePatch == NULL)
      return (NULL);
    PATCH_TYPE (thePatch) = POINT_PATCH_TYPE;
    PATCH_ID (thePatch) = i;
    m = 0;
    free = 0;
    for (j = 0; j < nsides; j++)
    {
      if (PATCH_TYPE (sides[j]) == LINEAR_PATCH_TYPE)
      {
        for (n = 0; n < LINEAR_PATCH_N (sides[j]); n++)
          if (LINEAR_PATCH_POINTS (sides[j], n) == i)
          {
            POINT_PATCH_PID (thePatch, m) = j;
            POINT_PATCH_CID (thePatch, m++) = n;
            if (PATCH_IS_FREE (sides[j]))
              free++;
          }
      }
      else if (PATCH_TYPE (sides[j]) == PARAMETRIC_PATCH_TYPE)
      {
        for (n = 0; n < 2 * DIM_OF_BND; n++)
          if (PARAM_PATCH_POINTS (sides[j], n) == i)
          {
            POINT_PATCH_PID (thePatch, m) = j;
            POINT_PATCH_CID (thePatch, m++) = n;
            if (PATCH_IS_FREE (sides[j]))
              free++;
          }
      }
    }
    if (free == m)
      PATCH_STATE (thePatch) = PATCH_FREE;
    else if (free == 0)
      PATCH_STATE (thePatch) = PATCH_FIXED;
    else
      PATCH_STATE (thePatch) = PATCH_BND_OF_FREE;

    for (j = 0; j < nsides; j++)
      POINT_PATCH_N (thePatch) = m;
    corners[i] = thePatch;
    PRINTDEBUG (dom, 1, ("corners id %d type %d n %d\n",
                         PATCH_ID (thePatch), PATCH_TYPE (thePatch),
                         POINT_PATCH_N (thePatch)));
    for (j = 0; j < POINT_PATCH_N (thePatch); j++)
      PRINTDEBUG (dom, 1, (" pid %d cid %d\n",
                           POINT_PATCH_PID (thePatch, j),
                           POINT_PATCH_CID (thePatch, j)));
  }

  /* create line patches */
  nlines = 0;
#ifdef __THREEDIM__
  lines =
    (PATCH **) GetTmpMem (Heap, ncorners * ncorners * sizeof (PATCH *),
                          MarkKey);
  if (lines == NULL)
    return (NULL);
  err = 0;
  for (i = 0; i < ncorners; i++)
    for (j = i + 1; j < ncorners; j++)
    {
      k = 0;
      for (n = 0; n < POINT_PATCH_N (corners[i]); n++)
        for (m = 0; m < POINT_PATCH_N (corners[j]); m++)
          if (POINT_PATCH_PID (corners[i], n) ==
              POINT_PATCH_PID (corners[j], m))
            k++;
      if (k < 2)
        /* points share one patch only and ly on opposite corners of this patch */
        continue;
      thePatch =
        (PATCH *) GetFreelistMemory (Heap, sizeof (LINE_PATCH)
                                     + (k -
                                        1) * sizeof (struct line_on_patch));
      if (thePatch == NULL)
        return (NULL);
      PATCH_TYPE (thePatch) = LINE_PATCH_TYPE;
      PATCH_ID (thePatch) = nlines;
      LINE_PATCH_C0 (thePatch) = i;
      LINE_PATCH_C1 (thePatch) = j;
      k = 0;
      free = 0;
      for (n = 0; n < POINT_PATCH_N (corners[i]); n++)
        for (m = 0; m < POINT_PATCH_N (corners[j]); m++)
          if (POINT_PATCH_PID (corners[i], n) ==
              POINT_PATCH_PID (corners[j], m))
          {
            LINE_PATCH_PID (thePatch, k) =
              POINT_PATCH_PID (corners[i], n);
            LINE_PATCH_CID0 (thePatch, k) =
              POINT_PATCH_CID (corners[i], n);
            LINE_PATCH_CID1 (thePatch, k) =
              POINT_PATCH_CID (corners[j], m);
            if (PATCH_IS_FREE (sides[LINE_PATCH_PID (thePatch, k)]))
              free++;
            k++;
          }
      LINE_PATCH_N (thePatch) = k;

      for (n = 0; n < LINE_PATCH_N (thePatch); n++)
        PRINTDEBUG (dom, 1, (" pid %d cid %d %d",
                             LINE_PATCH_PID (thePatch, n),
                             LINE_PATCH_CID0 (thePatch, n),
                             LINE_PATCH_CID1 (thePatch, n)));
      PRINTDEBUG (dom, 1, ("\n"));

      IFDEBUG (dom, 10) if (k == 2)
      {
        INT o0, o1, s0, s1;

        s0 = LINE_PATCH_PID (thePatch, 0);
        s1 = LINE_PATCH_PID (thePatch, 1);
        o0 =
          (LINE_PATCH_CID0 (thePatch, 0) ==
           ((LINE_PATCH_CID1 (thePatch, 0) + 1) % (2 * DIM_OF_BND)));
        o1 =
          (LINE_PATCH_CID0 (thePatch, 1) ==
           ((LINE_PATCH_CID1 (thePatch, 1) + 1) % (2 * DIM_OF_BND)));
        if (o0 != o1)
        {
          if ((PARAM_PATCH_LEFT (sides[s0]) !=
               PARAM_PATCH_LEFT (sides[s1]))
              || ((PARAM_PATCH_RIGHT (sides[s0]) !=
                   PARAM_PATCH_RIGHT (sides[s1]))))
          {
            PRINTDEBUG (dom, 0, ("patch %d and patch %d:"
                                 "orientation not maches\n", s0, s1));
            err++;
          }
        }
        else
        {
          if ((PARAM_PATCH_LEFT (sides[s0]) !=
               PARAM_PATCH_RIGHT (sides[s1]))
              || ((PARAM_PATCH_RIGHT (sides[s0]) !=
                   PARAM_PATCH_LEFT (sides[s1]))))
          {
            PRINTDEBUG (dom, 0, ("patch %d and patch %d:"
                                 "orientation not maches\n", s0, s1));
            err++;
          }
        }
      }
      ENDDEBUG if (free == k)
        PATCH_STATE (thePatch) = PATCH_FREE;
      else if (free == 0)
        PATCH_STATE (thePatch) = PATCH_FIXED;
      else
        PATCH_STATE (thePatch) = PATCH_BND_OF_FREE;
      lines[nlines++] = thePatch;
      PRINTDEBUG (dom, 1, ("lines id %d type %d n %d\n",
                           PATCH_ID (thePatch), PATCH_TYPE (thePatch),
                           LINE_PATCH_N (thePatch)));
    }
  ASSERT (err == 0);
#endif

  m = ncorners + nlines;
  theBVP->sideoffset = m;
  n = m + nsides;
  theBVP->patches = (PATCH **) GetFreelistMemory (Heap, n * sizeof (PATCH *));
  n = 0;
  for (i = 0; i < ncorners; i++)
  {
    thePatch = corners[i];
    for (j = 0; j < POINT_PATCH_N (thePatch); j++)
      POINT_PATCH_PID (thePatch, j) += m;
    theBVP->patches[n++] = thePatch;
  }
#ifdef __THREEDIM__
  for (i = 0; i < nlines; i++)
  {
    thePatch = lines[i];
    PATCH_ID (thePatch) = n;
    for (j = 0; j < LINE_PATCH_N (thePatch); j++)
      LINE_PATCH_PID (thePatch, j) += m;
    theBVP->patches[n++] = thePatch;
  }
#endif
  for (i = 0; i < nsides; i++)
  {
    thePatch = sides[i];
    PATCH_ID (thePatch) = n;
    theBVP->patches[n++] = thePatch;
  }

  PRINTDEBUG (dom, 1, ("ncorners %d\n", theBVP->ncorners));
  for (i = 0; i < theBVP->ncorners; i++)
    PRINTDEBUG (dom, 1, ("   id %d\n", PATCH_ID (theBVP->patches[i])));

  if (Mesh != NULL)
  {
    Mesh->mesh_status = MESHSTAT_CNODES;
    Mesh->nBndP = theBVP->ncorners;
    Mesh->nInnP = 0;
    Mesh->nElements = NULL;
    Mesh->VertexLevel = NULL;
    Mesh->VertexPrio = NULL;
    Mesh->ElementLevel = NULL;
    Mesh->ElementPrio = NULL;
    Mesh->ElemSideOnBnd = NULL;
    Mesh->theBndPs =
      (BNDP **) GetTmpMem (Heap, n * sizeof (BNDP *), MarkKey);
    if (Mesh->theBndPs == NULL)
      return (NULL);

    if (CreateCornerPoints (Heap, theBVP, Mesh->theBndPs))
      return (NULL);

    PRINTDEBUG (dom, 1, ("mesh n %d\n", Mesh->nBndP));
    for (i = 0; i < theBVP->ncorners; i++)
      PRINTDEBUG (dom, 1, (" id %d\n",
                           BND_PATCH_ID ((BND_PS *) (Mesh->theBndPs[i]))));
  }

  /* allocate s2p table */
  STD_BVP_NDOMPART (theBVP) = DOMAIN_NPARTS (theDomain);
  STD_BVP_S2P_PTR (theBVP) =
    (INT *) GetFreelistMemory (Heap,
                               (1 + STD_BVP_NSUBDOM (theBVP)) * sizeof (INT));
  if (STD_BVP_S2P_PTR (theBVP) == NULL)
    return (NULL);

  /* fill number of parts */
  if (DOMAIN_NPARTS (theDomain) > 1)
  {
    const DOMAIN_PART_INFO *dpi;

    if (DOMAIN_NPARTS (theDomain) >= (1 << VPART_LEN))
    {
      printf ("too many parts for control entry in vector\n");
      ASSERT (FALSE);
      return (NULL);
    }

    /* transfer from part info (NB: STD_BVP_NSUBDOM only counts inner subdomains) */
    dpi = DOMAIN_PARTINFO (theDomain);
    for (i = 0; i <= STD_BVP_NSUBDOM (theBVP); i++)
      STD_BVP_S2P (theBVP, i) = DPI_SD2P (dpi, i);
  }
  else
  {
    /* 0 for each subdomnain by default */
    for (i = 0; i < STD_BVP_NSUBDOM (theBVP); i++)
      STD_BVP_S2P (theBVP, i) = 0;
  }

  return ((BVP *) theBVP);
}

/* domain interface function: for description see domain.h */
INT NS_PREFIX
BVP_Dispose (BVP * theBVP)
{
  return (0);
}

/* domain interface function: for description see domain.h */
BVP *NS_PREFIX
BVP_GetFirst (void)
{
  ENVDIR *theSBVPDir;
  BVP *theBVP;

  theSBVPDir = ChangeEnvDir ("/STD_BVP");
  assert (theSBVPDir != NULL);
  theBVP = (BVP *) ENVDIR_DOWN (theSBVPDir);

  return (theBVP);
}

/* domain interface function: for description see domain.h */
BVP *NS_PREFIX
BVP_GetNext (BVP * theBVP)
{
  if (theBVP == NULL)
    return (NULL);
  return ((BVP *) NEXT_ENVITEM (theBVP));
}

/* domain interface function: for description see domain.h */
BVP *NS_PREFIX
BVP_GetByName (char *name)
{
  return ((BVP *) SearchEnv (name, "/BVP", theBVPDirID, theBVPDirID));
}

INT NS_PREFIX
BVP_SetBVPDesc (BVP * aBVP, BVP_DESC * theBVPDesc)
{
  STD_BVP *theBVP;
  INT i;

  if (aBVP == NULL)
    return (1);

  /* cast */
  theBVP = GetSTD_BVP (aBVP);

  /* general part */
  strcpy (BVPD_NAME (theBVPDesc), ENVITEM_NAME (theBVP));

  /* the domain part */
  for (i = 0; i < DIM; i++)
    BVPD_MIDPOINT (theBVPDesc)[i] = theBVP->MidPoint[i];
  BVPD_RADIUS (theBVPDesc) = theBVP->radius;
  BVPD_CONVEX (theBVPDesc) = theBVP->domConvex;
  BVPD_NSUBDOM (theBVPDesc) = theBVP->numOfSubdomains;
  BVPD_NPARTS (theBVPDesc) = theBVP->nDomainParts;
  BVPD_S2P_PTR (theBVPDesc) = STD_BVP_S2P_PTR (theBVP);
  BVPD_NCOEFFF (theBVPDesc) = theBVP->numOfCoeffFct;
  BVPD_NUSERF (theBVPDesc) = theBVP->numOfUserFct;
  BVPD_CONFIG (theBVPDesc) = theBVP->ConfigProc;

  currBVP = theBVP;

  return (0);
}

/* domain interface function: for description see domain.h */
INT NS_PREFIX
BVP_SetCoeffFct (BVP * aBVP, INT n, CoeffProcPtr * CoeffFct)
{
  STD_BVP *theBVP;
  INT i;

  theBVP = GetSTD_BVP (aBVP);

  /* check */
  if (n < -1 || n >= theBVP->numOfCoeffFct)
    return (1);

  if (n == -1)
    for (i = 0; i < theBVP->numOfCoeffFct; i++)
      CoeffFct[i] = (CoeffProcPtr) theBVP->CU_ProcPtr[i];
  else
    CoeffFct[0] = (CoeffProcPtr) theBVP->CU_ProcPtr[n];

  return (0);
}

/* domain interface function: for description see domain.h */
INT NS_PREFIX
BVP_SetUserFct (BVP * aBVP, INT n, UserProcPtr * UserFct)
{
  STD_BVP *theBVP;
  INT i;

  theBVP = GetSTD_BVP (aBVP);

  /* check */
  if (n < -1 || n >= theBVP->numOfUserFct)
    return (1);

  if (n == -1)
    for (i = 0; i < theBVP->numOfUserFct; i++)
      UserFct[i] =
        (UserProcPtr) theBVP->CU_ProcPtr[i + theBVP->numOfCoeffFct];
  else
    UserFct[0] = (UserProcPtr) theBVP->CU_ProcPtr[n + theBVP->numOfCoeffFct];

  return (0);
}

/* domain interface function: for description see domain.h */
INT NS_PREFIX
BVP_Check (BVP * aBVP)
{
  UserWrite ("BVP_Check: not implemented\n");

  return (0);
}

static INT
GetNumberOfCommonPatches (PATCH * p0, PATCH * p1, INT * Pid)
{
  INT i, j, cnt, np0, np1, pid;

  cnt = 0;
  np0 = GetNumberOfPatches (p0);
  np1 = GetNumberOfPatches (p1);
  for (i = 0; i < np0; i++)
  {
    pid = GetPatchId (p0, i);
    for (j = 0; j < np1; j++)
      if (pid == GetPatchId (p1, j))
      {
        if (!cnt)
          *Pid = pid;
        cnt++;
      }
  }

  return (cnt);
}

static INT
GetCommonPatchId (PATCH * p0, PATCH * p1, INT k)
{
  INT i, j, cnt;

  cnt = 0;
  for (i = 0; i < GetNumberOfPatches (p0); i++)
    for (j = 0; j < GetNumberOfPatches (p1); j++)
      if (GetPatchId (p0, i) == GetPatchId (p1, j))
        if (k == cnt++)
          return (GetPatchId (p1, j));

  return (-1);
}

#ifdef __THREEDIM__
static INT
GetCommonLinePatchId (PATCH * p0, PATCH * p1)
{
  INT i, k, l, cnt, cnt1;
  PATCH *p;

  if (PATCH_TYPE (p0) == LINE_PATCH_TYPE)
    return (PATCH_ID (p0));
  else if (PATCH_TYPE (p1) == LINE_PATCH_TYPE)
    return (PATCH_ID (p1));

  cnt = GetNumberOfCommonPatches (p0, p1, &i);

  if (cnt < 1)
    return (-1);

  for (k = currBVP->ncorners; k < currBVP->sideoffset; k++)
  {
    p = currBVP->patches[k];
    if (LINE_PATCH_N (p) != cnt)
      continue;
    cnt1 = 0;
    for (i = 0; i < cnt; i++)
      for (l = 0; l < LINE_PATCH_N (p); l++)
        if (GetCommonPatchId (p0, p1, i) == LINE_PATCH_PID (p, l))
          cnt1++;
    if (cnt == cnt1)
      return (k);
  }

  return (-1);
}

static BNDP *
CreateBndPOnLine (HEAP * Heap, PATCH * p0, PATCH * p1, DOUBLE lcoord)
{
  BND_PS *bp;
  PATCH *p, *pp;
  DOUBLE local0[DIM_OF_BND];
  DOUBLE local1[DIM_OF_BND];
  INT k, l, cnt;

  if (PATCH_TYPE (p0) != POINT_PATCH_TYPE)
    return (NULL);
  if (PATCH_TYPE (p1) != POINT_PATCH_TYPE)
    return (NULL);

  PRINTDEBUG (dom, 1, ("    p0 p1 %d %d\n", PATCH_ID (p0), PATCH_ID (p1)));
  for (l = 0; l < GetNumberOfPatches (p0); l++)
    PRINTDEBUG (dom, 1, ("    bp pid %d\n", GetPatchId (p0, l)));
  for (l = 0; l < GetNumberOfPatches (p1); l++)
    PRINTDEBUG (dom, 1, ("    bp pid %d\n", GetPatchId (p1, l)));

  cnt = GetNumberOfCommonPatches (p0, p1, &k);

  if (cnt < 2)
    return (NULL);

  bp =
    (BND_PS *) GetFreelistMemory (Heap,
                                  (cnt - 1) * sizeof (COORD_BND_VECTOR) +
                                  sizeof (BND_PS));
  if (bp == NULL)
    return (NULL);
  bp->n = cnt;

  k = GetCommonLinePatchId (p0, p1);
  if ((k < currBVP->ncorners) || (k >= currBVP->sideoffset))
    return (NULL);
  p = currBVP->patches[k];
  bp->patch_id = k;

  PRINTDEBUG (dom, 1, (" Create BNDP line %d cnt %d\n", k, cnt));
  for (l = 0; l < GetNumberOfPatches (p0); l++)
    PRINTDEBUG (dom, 1, ("    bp pid %d\n", GetPatchId (p0, l)));
  for (l = 0; l < GetNumberOfPatches (p1); l++)
    PRINTDEBUG (dom, 1, ("    bp pid %d\n", GetPatchId (p1, l)));

  for (l = 0; l < LINE_PATCH_N (p); l++)
  {
    pp = currBVP->patches[LINE_PATCH_PID (p, l)];
    switch (LINE_PATCH_CID0 (p, l))
    {
    case 0 :
      local0[0] = PARAM_PATCH_RANGE (pp)[0][0];
      local0[1] = PARAM_PATCH_RANGE (pp)[0][1];
      break;
    case 1 :
      local0[0] = PARAM_PATCH_RANGE (pp)[1][0];
      local0[1] = PARAM_PATCH_RANGE (pp)[0][1];
      break;
    case 2 :
      local0[0] = PARAM_PATCH_RANGE (pp)[1][0];
      local0[1] = PARAM_PATCH_RANGE (pp)[1][1];
      break;
    case 3 :
      local0[0] = PARAM_PATCH_RANGE (pp)[0][0];
      local0[1] = PARAM_PATCH_RANGE (pp)[1][1];
      break;
    }
    switch (LINE_PATCH_CID1 (p, l))
    {
    case 0 :
      local1[0] = PARAM_PATCH_RANGE (pp)[0][0];
      local1[1] = PARAM_PATCH_RANGE (pp)[0][1];
      break;
    case 1 :
      local1[0] = PARAM_PATCH_RANGE (pp)[1][0];
      local1[1] = PARAM_PATCH_RANGE (pp)[0][1];
      break;
    case 2 :
      local1[0] = PARAM_PATCH_RANGE (pp)[1][0];
      local1[1] = PARAM_PATCH_RANGE (pp)[1][1];
      break;
    case 3 :
      local1[0] = PARAM_PATCH_RANGE (pp)[0][0];
      local1[1] = PARAM_PATCH_RANGE (pp)[1][1];
      break;
    }

    /* TODO: Why this? */
    if ((LINE_PATCH_CID0 (p, l) == 2) && (LINE_PATCH_CID1 (p, l) == 3))
      lcoord = 1.0 - lcoord;
    if ((LINE_PATCH_CID0 (p, l) == 1) && (LINE_PATCH_CID1 (p, l) == 0))
      lcoord = 1.0 - lcoord;
    if ((LINE_PATCH_CID0 (p, l) == 3) && (LINE_PATCH_CID1 (p, l) == 0))
      lcoord = 1.0 - lcoord;
    if ((LINE_PATCH_CID0 (p, l) == 2) && (LINE_PATCH_CID1 (p, l) == 1))
      lcoord = 1.0 - lcoord;

    bp->local[l][0] = (1.0 - lcoord) * local0[0] + lcoord * local1[0];
    bp->local[l][1] = (1.0 - lcoord) * local0[1] + lcoord * local1[1];

    if ((LINE_PATCH_CID0 (p, l) == 2) && (LINE_PATCH_CID1 (p, l) == 3))
      lcoord = 1.0 - lcoord;
    if ((LINE_PATCH_CID0 (p, l) == 1) && (LINE_PATCH_CID1 (p, l) == 0))
      lcoord = 1.0 - lcoord;
    if ((LINE_PATCH_CID0 (p, l) == 3) && (LINE_PATCH_CID1 (p, l) == 0))
      lcoord = 1.0 - lcoord;
    if ((LINE_PATCH_CID0 (p, l) == 2) && (LINE_PATCH_CID1 (p, l) == 1))
      lcoord = 1.0 - lcoord;


    PRINTDEBUG (dom, 1,
                (" Create bndp %d line %d  C0 %d C1 %d l %d %f %f\n",
                 bp->patch_id, LINE_PATCH_PID (p, l), LINE_PATCH_CID0 (p,
                                                                       l),
                 LINE_PATCH_CID1 (p, l), l, bp->local[l][0],
                 bp->local[l][1]));
    PRINTDEBUG (dom, 1, (" lcoord %f\n", lcoord));
  }

  if (!PATCH_IS_FIXED (p))
  {
    /* store global coordinates */
    BND_DATA (bp) = GetFreelistMemory (Heap, DIM * sizeof (DOUBLE));
    if (BND_DATA (bp) == NULL)
      return (NULL);

    if (BndPointGlobal ((BNDP *) bp, (DOUBLE *) BND_DATA (bp)))
      return (NULL);
  }

  return ((BNDP *) bp);
}
#endif

#define BN_RES          100

static int
DropPerpendicularOnSegment (PATCH * patch, double range[][DIM_OF_BND],
                            const double global[], double local[],
                            double *mindist2)
{
  double sa = range[0][0];
  double sb = range[1][0];
  double ds = (sb - sa) / ((double) BN_RES);
  int k;
#if (DIM==3)
  double ta = range[0][1];
  double tb = range[1][1];
  double dt = (tb - ta) / ((double) BN_RES);
  int l;
#endif

  for (k = 0; k <= BN_RES; k++)
  {
    DOUBLE lambda[DIM_OF_BND];
    DOUBLE_VECTOR point, diff;
    double dist2;

    lambda[0] = (k == BN_RES) ? sb : sa + k * ds;

#if (DIM==3)
    for (l = 0; l <= BN_RES; l++)
    {

      lambda[1] = (l == BN_RES) ? tb : ta + l * dt;
#endif

    if (PatchGlobal (patch, lambda, point))
      return 1;
    V_DIM_SUBTRACT (point, global, diff);
    dist2 = V_DIM_SCAL_PROD (diff, diff);
    if (*mindist2 > dist2)
    {
      *mindist2 = dist2;
      V_BDIM_COPY (lambda, local);
    }
#if (DIM==3)
  }
#endif
  }
  return 0;
}

static int
ResolvePointOnSegment (PATCH * patch, int depth, double resolution2,
                       double range[][DIM_OF_BND], const double global[],
                       double local[])
{
  double ds = (range[1][0] - range[0][0]) / ((double) BN_RES);
#if (DIM==3)
  double dt = (range[1][1] - range[0][1]) / ((double) BN_RES);
#endif
  double new_range[2][DIM_OF_BND];
  DOUBLE mindist2 = MAX_D;

  new_range[0][0] = local[0] - ds;
  new_range[1][0] = local[0] + ds;
#if (DIM==3)
  new_range[0][1] = local[1] - dt;
  new_range[1][1] = local[1] + dt;
#endif

  if (DropPerpendicularOnSegment (patch, new_range, global, local, &mindist2))
    return 1;
  if (mindist2 > resolution2)
  {
    if (depth > 0)
    {
      /* recursive call */
      if (ResolvePointOnSegment
            (patch, depth - 1, resolution2, new_range, global, local))
        return 1;
    }
    else
      return 2;
  }
  return 0;
}

/* domain interface function: for description see domain.h */
/* TODO: syntax for manpages??? */
BNDP *NS_PREFIX
BVP_InsertBndP (HEAP * Heap, BVP * aBVP, INT argc, char **argv)
{
  STD_BVP *theBVP;
  BND_PS *ps;
  PATCH *p;
  INT j, pid;
  int i;
  float pos[2];
#       ifdef __THREEDIM__
  DOUBLE lc;
#       endif

  theBVP = GetSTD_BVP (aBVP);

  if (ReadArgvOption ("g", argc, argv))
  {
    DOUBLE resolution2;
    /* double global[DIM]; extension to constant 3 components for sscanf; Ch. Wrobel 981002 */
    double global[3];

    /* insert bn from global coordinates */
    if (sscanf (argv[0], "bn %lf %lf %lf", global, global +1, global +2) !=
        DIM)
    {
      PrintErrorMessageF ('E', "BVP_InsertBndP",
                          "g option specified but could not scan\n"
                          "global coordinates from '%s'\n", argv[0]);
      return (NULL);
    }

    if (ReadArgvDOUBLE ("r", &resolution2, argc, argv) != 0)
      resolution2 = 1e-2;       /* default */
    resolution2 *= resolution2;

    /* find segment id and local coordinates (on any segment) */
    {
      DOUBLE mindist2 = MAX_D;
      int seg;
      DOUBLE lambda[DIM_OF_BND];

      for (seg = 0; seg < STD_BVP_NSIDES (theBVP); seg++)
      {
        PATCH *patch =
          STD_BVP_PATCH (theBVP, seg + STD_BVP_SIDEOFFSET (theBVP));
        DOUBLE seg_mindist2 = mindist2;

        if (DropPerpendicularOnSegment
              (patch, PARAM_PATCH_RANGE (patch), global, lambda,
              &seg_mindist2))
          return NULL;

        if (mindist2 > seg_mindist2)
        {
          mindist2 = seg_mindist2;
          i = seg;
          V_BDIM_COPY (lambda, pos);
        }
        if (mindist2 <= resolution2)
          break;
      }
      if (mindist2 > resolution2)
      {
        /* refine search */
        PATCH *patch =
          STD_BVP_PATCH (theBVP, i + STD_BVP_SIDEOFFSET (theBVP));

        V_BDIM_COPY (pos, lambda);
        if (ResolvePointOnSegment
              (patch, 2, resolution2, PARAM_PATCH_RANGE (patch), global,
              lambda))
          return NULL;
        V_BDIM_COPY (lambda, pos);
      }
    }

  }
  else
  {
    if (sscanf (argv[0], "bn %d %f %f", &i, pos, pos + 1) != DIM_OF_BND + 1)
    {
      PrintErrorMessageF ('E', "BVP_InsertBndP",
                          "could not scan segment id and\n"
                          "local coordinates on segment from '%s'\n",
                          argv[0]);
      return (NULL);
    }
  }
  pid = i + theBVP->sideoffset;
  p = theBVP->patches[pid];

#ifdef __THREEDIM__
  /* check point on line or on point patch */
  if (ABS (pos[0] - PARAM_PATCH_RANGE (p)[0][0]) < SMALL_DIFF)
  {
    lc = (pos[1] - PARAM_PATCH_RANGE (p)[0][1])
         / (PARAM_PATCH_RANGE (p)[1][1] - PARAM_PATCH_RANGE (p)[0][1]);
    if (ABS (lc) < SMALL_DIFF)
      return (CreateBndPOnPoint
                (Heap, currBVP->patches[PARAM_PATCH_POINTS (p, 0)]));
    else if (ABS (lc - 1.) < SMALL_DIFF)
      return (CreateBndPOnPoint
                (Heap, currBVP->patches[PARAM_PATCH_POINTS (p, 3)]));
    return (CreateBndPOnLine
              (Heap, currBVP->patches[PARAM_PATCH_POINTS (p, 0)],
              currBVP->patches[PARAM_PATCH_POINTS (p, 3)], lc));
  }
  else if (ABS (pos[0] - PARAM_PATCH_RANGE (p)[1][0]) < SMALL_DIFF)
  {
    lc = (pos[1] - PARAM_PATCH_RANGE (p)[0][1])
         / (PARAM_PATCH_RANGE (p)[1][1] - PARAM_PATCH_RANGE (p)[0][1]);
    if (ABS (lc) < SMALL_DIFF)
      return (CreateBndPOnPoint
                (Heap, currBVP->patches[PARAM_PATCH_POINTS (p, 1)]));
    else if (ABS (lc - 1.) < SMALL_DIFF)
      return (CreateBndPOnPoint
                (Heap, currBVP->patches[PARAM_PATCH_POINTS (p, 2)]));
    return (CreateBndPOnLine
              (Heap, currBVP->patches[PARAM_PATCH_POINTS (p, 1)],
              currBVP->patches[PARAM_PATCH_POINTS (p, 2)], lc));
  }
  else if (ABS (pos[1] - PARAM_PATCH_RANGE (p)[0][1]) < SMALL_DIFF)
  {
    lc = (pos[0] - PARAM_PATCH_RANGE (p)[0][0])
         / (PARAM_PATCH_RANGE (p)[1][0] - PARAM_PATCH_RANGE (p)[0][0]);
    if (ABS (lc) < SMALL_DIFF)
      return (CreateBndPOnPoint
                (Heap, currBVP->patches[PARAM_PATCH_POINTS (p, 0)]));
    else if (ABS (lc - 1.) < SMALL_DIFF)
      return (CreateBndPOnPoint
                (Heap, currBVP->patches[PARAM_PATCH_POINTS (p, 1)]));
    return (CreateBndPOnLine
              (Heap, currBVP->patches[PARAM_PATCH_POINTS (p, 0)],
              currBVP->patches[PARAM_PATCH_POINTS (p, 1)], lc));
  }
  else if (ABS (pos[1] - PARAM_PATCH_RANGE (p)[1][1]) < SMALL_DIFF)
  {
    lc = (pos[0] - PARAM_PATCH_RANGE (p)[0][0])
         / (PARAM_PATCH_RANGE (p)[1][0] - PARAM_PATCH_RANGE (p)[0][0]);
    if (ABS (lc) < SMALL_DIFF)
      return (CreateBndPOnPoint
                (Heap, currBVP->patches[PARAM_PATCH_POINTS (p, 3)]));
    else if (ABS (lc - 1.) < SMALL_DIFF)
      return (CreateBndPOnPoint
                (Heap, currBVP->patches[PARAM_PATCH_POINTS (p, 2)]));
    return (CreateBndPOnLine
              (Heap, currBVP->patches[PARAM_PATCH_POINTS (p, 3)],
              currBVP->patches[PARAM_PATCH_POINTS (p, 2)], lc));
  }
#endif
#ifdef __TWODIM__
  /* check point on point patch */
  if (ABS (pos[0] - PARAM_PATCH_RANGE (p)[0][0]) < SMALL_DIFF)
    return (CreateBndPOnPoint
              (Heap, currBVP->patches[PARAM_PATCH_POINTS (p, 0)]));
  else if (ABS (pos[0] - PARAM_PATCH_RANGE (p)[0][1]) < SMALL_DIFF)
    return (CreateBndPOnPoint
              (Heap, currBVP->patches[PARAM_PATCH_POINTS (p, 1)]));
#endif

  if (PATCH_TYPE (p) == PARAMETRIC_PATCH_TYPE)
  {
    PRINTDEBUG (dom, 1, (" id %d i %d ns %d %s pos %f\n",
                         pid, i, currBVP->sideoffset, argv[0], pos[0]));
    ps = (BND_PS *) GetFreelistMemory (Heap, sizeof (BND_PS));
    if (ps == NULL)
      return (NULL);
    ps->patch_id = pid;
    ps->n = 1;
    for (j = 0; j < DIM_OF_BND; j++)
      ps->local[0][j] = pos[j];
  }
  else
    return (NULL);

  if (!PATCH_IS_FIXED (p))
  {
    /* store global coordinates */
    BND_DATA (ps) = GetFreelistMemory (Heap, DIM * sizeof (DOUBLE));
    if (BND_DATA (ps) == NULL)
      return (NULL);

    if (BndPointGlobal ((BNDP *) ps, (DOUBLE *) BND_DATA (ps)))
      return (NULL);
  }

  return ((BNDP *) ps);
}

static DOUBLE
LengthOfSide (PATCH * p)
{
  DOUBLE length, step, s;
  DOUBLE lambda[DIM_OF_BND], x[2][DIM];
  INT i;

  length = 0.0;

  lambda[0] = PARAM_PATCH_RANGE (p)[0][0];
  step = (PARAM_PATCH_RANGE (p)[1][0]
          - PARAM_PATCH_RANGE (p)[0][0]) / RESOLUTION;
  if ((*PARAM_PATCH_BS (p))(PARAM_PATCH_BSD (p), lambda, x[0]))
    return (0.0);
  for (i = 1; i < RESOLUTION; i++)
  {
    lambda[0] += step;
    if ((*PARAM_PATCH_BS (p))(PARAM_PATCH_BSD (p), lambda, x[i % 2]))
      return (0.0);
    V_DIM_EUKLIDNORM_OF_DIFF (x[0], x[1], s);
    length += s;
  }
  lambda[0] = PARAM_PATCH_RANGE (p)[1][0];
  if ((*PARAM_PATCH_BS (p))(PARAM_PATCH_BSD (p), lambda, x[RESOLUTION % 2]))
    return (0.0);
  V_DIM_EUKLIDNORM_OF_DIFF (x[0], x[1], s);
  length += s;

  return (length);
}

static DOUBLE
MeshSize (CoeffProcPtr coeff, PATCH * p, DOUBLE * lambda)
{
  DOUBLE step;
  DOUBLE global[DIM];

  if ((*PARAM_PATCH_BS (p))(PARAM_PATCH_BSD (p), lambda, global))
    return (0.0);
  (*coeff)(global, &step);

  PRINTDEBUG (dom, 1, (" c:lambda %f x %f %f step %f\n",
                       lambda[0], global[0], global[1], step));


  return (step);
}

static INT
GenerateBnodes (HEAP * Heap, STD_BVP * theBVP, BNDP ** bndp,
                INT * sides, INT *** corners, CoeffProcPtr coeff)
{
  INT i, j, n, nside, left, right;
  DOUBLE length, plength, step, step1;
  DOUBLE lambda[DIM_OF_BND], lambda1;
  PATCH *p;
  BND_PS *ps;

  n = theBVP->ncorners;

#ifdef __THREEDIM__
  return (n);
#endif

  for (i = 0; i <= theBVP->numOfSubdomains; i++)
    sides[i] = 0;

  for (i = theBVP->sideoffset; i < theBVP->sideoffset + theBVP->nsides; i++)
  {
    nside = n;
    p = theBVP->patches[i];
    length = LengthOfSide (p);
    if (length == 0.0)
      return (-1);
    plength = ABS (PARAM_PATCH_RANGE (p)[1][0]
                   - PARAM_PATCH_RANGE (p)[0][0]);
    lambda[0] = MIN (PARAM_PATCH_RANGE (p)[0][0],
                     PARAM_PATCH_RANGE (p)[1][0]);
    lambda1 = MAX (PARAM_PATCH_RANGE (p)[0][0],
                   PARAM_PATCH_RANGE (p)[1][0]);

    PRINTDEBUG (dom, 1,
                (" side i %d lenght %f pl %f\n", i, length, plength));

    step = MeshSize (coeff, p, lambda) * plength / length;
    if (step == 0.0)
      return (-1);
    lambda[0] += step;
    while (lambda[0] < lambda1)
    {
      if (lambda[0] + step > lambda1)
        lambda[0] = 0.5 * (lambda1 + lambda[0] - step);
      else
      {
        step1 = MeshSize (coeff, p, lambda) * plength / length;
        if (step1 == 0.0)
          return (-1);
        if (step1 < step)
        {
          lambda[0] += step1 - step;
          step = MeshSize (coeff, p, lambda) * plength / length;
        }
        else
          step = step1;
      }
      if (bndp != NULL)
      {
        ps = (BND_PS *) GetFreelistMemory (Heap, sizeof (BND_PS));
        if (ps == NULL)
          return (0);
        ps->n = 1;
        bndp[n] = (BNDP *) ps;
        ps->patch_id = i;
        ps->local[0][0] = lambda[0];

        if (!PATCH_IS_FIXED (p))
        {
          /* store global coordinates */
          BND_DATA (ps) =
            GetFreelistMemory (Heap, DIM * sizeof (DOUBLE));
          if (BND_DATA (ps) == NULL)
            return (1);

          if (BndPointGlobal ((BNDP *) ps, (DOUBLE *) BND_DATA (ps)))
            return (1);
        }
      }
      n++;
      lambda[0] += step;

      PRINTDEBUG (dom, 1, ("  lam %f sp %f\n", lambda[0], step));

    }
    if (n == nside)
    {
      if (bndp != NULL)
      {
        ps = (BND_PS *) bndp[n];
        ps->patch_id = i;
        ps->local[0][0] = 0.5 * (PARAM_PATCH_RANGE (p)[0][0] +
                                 PARAM_PATCH_RANGE (p)[1][0]);
      }
      n++;
    }
    left = PARAM_PATCH_LEFT (p);
    right = PARAM_PATCH_RIGHT (p);
    if (corners != NULL)
    {
      if (left > 0)
      {
        if (PARAM_PATCH_RANGE (p)[1][0] > PARAM_PATCH_RANGE (p)[0][0])
        {
          corners[left][sides[left]][0] = PARAM_PATCH_POINTS (p, 0);
          for (j = nside; j < n; j++)
          {
            corners[left][sides[left]++][1] = j;
            corners[left][sides[left]][0] = j;
          }
          corners[left][sides[left]++][1] = PARAM_PATCH_POINTS (p, 1);
        }
        else
        {
          corners[left][sides[left]][1] = PARAM_PATCH_POINTS (p, 0);
          for (j = n - 1; j >= nside; j--)
          {
            corners[left][sides[left]++][0] = j;
            corners[left][sides[left]][1] = j;
          }
          corners[left][sides[left]++][0] = PARAM_PATCH_POINTS (p, 1);
        }
      }
      if (right > 0)
      {
        if (PARAM_PATCH_RANGE (p)[1][0] > PARAM_PATCH_RANGE (p)[0][0])
        {
          corners[right][sides[right]][1] = PARAM_PATCH_POINTS (p, 0);
          for (j = nside; j < n; j++)
          {
            corners[right][sides[right]++][0] = j;
            corners[right][sides[right]][1] = j;
          }
          corners[right][sides[right]++][0] =
            PARAM_PATCH_POINTS (p, 1);
        }
        else
        {
          corners[right][sides[right]][0] = PARAM_PATCH_POINTS (p, 0);
          for (j = n - 1; j >= nside; j--)
          {
            corners[right][sides[right]++][1] = j;
            corners[right][sides[right]][0] = j;
          }
          corners[right][sides[right]++][1] =
            PARAM_PATCH_POINTS (p, 1);
        }
      }
    }
    else
    {
      if (left > 0)
        sides[left] += n - nside + 1;
      if (right > 0)
        sides[right] += n - nside + 1;
    }
  }

  return (n);
}

#ifdef __TWODIM__
static INT
GenerateBnodes_h (HEAP * Heap, STD_BVP * theBVP, BNDP ** bndp,
                  INT * sides, INT *** corners, DOUBLE h, INT MarkKey)
{
  INT i, j, m, n, nside, left, right;
  DOUBLE length, plength, step;
  DOUBLE lambda[DIM_OF_BND], lambda1;
  PATCH *p;
  BND_PS *ps;

  n = theBVP->ncorners;
  for (i = 0; i <= theBVP->numOfSubdomains; i++)
    sides[i] = 0;

  for (i = theBVP->sideoffset; i < theBVP->sideoffset + theBVP->nsides; i++)
  {
    nside = n;
    p = theBVP->patches[i];
    length = LengthOfSide (p);
    if (length == 0.0)
      return (-1);
    plength = ABS (PARAM_PATCH_RANGE (p)[1][0]
                   - PARAM_PATCH_RANGE (p)[0][0]);
    lambda[0] = MIN (PARAM_PATCH_RANGE (p)[0][0],
                     PARAM_PATCH_RANGE (p)[1][0]);
    lambda1 = MAX (PARAM_PATCH_RANGE (p)[0][0],
                   PARAM_PATCH_RANGE (p)[1][0]);


    PRINTDEBUG (dom, 1,
                (" h:side n %d lenght %f pl %f\n", i, length, plength));

    m = length / h;
    if (m < 2)
      m = 2;
    step = plength / m;
    for (j = 1; j < m; j++)
    {
      lambda[0] += step;
      if (bndp != NULL)
      {
        ps = (BND_PS *) GetFreelistMemory (Heap, sizeof (BND_PS));
        if (ps == NULL)
          return (0);
        ps->n = 1;
        bndp[n] = (BNDP *) ps;
        ps->patch_id = i;
        ps->local[0][0] = lambda[0];

        if (!PATCH_IS_FIXED (p))
        {
          /* store global coordinates */
          BND_DATA (ps) =
            GetFreelistMemory (Heap, DIM * sizeof (DOUBLE));
          if (BND_DATA (ps) == NULL)
            return (1);

          if (BndPointGlobal ((BNDP *) ps, (DOUBLE *) BND_DATA (ps)))
            return (1);
        }
      }
      n++;
    }
    left = PARAM_PATCH_LEFT (p);
    right = PARAM_PATCH_RIGHT (p);
    if (corners != NULL)
    {
      if (left > 0)
      {
        if (PARAM_PATCH_RANGE (p)[1][0] > PARAM_PATCH_RANGE (p)[0][0])
        {
          corners[left][sides[left]][0] = PARAM_PATCH_POINTS (p, 0);
          for (j = nside; j < n; j++)
          {
            corners[left][sides[left]++][1] = j;
            corners[left][sides[left]][0] = j;
          }
          corners[left][sides[left]++][1] = PARAM_PATCH_POINTS (p, 1);
        }
        else
        {
          corners[left][sides[left]][1] = PARAM_PATCH_POINTS (p, 0);
          for (j = n - 1; j >= nside; j--)
          {
            corners[left][sides[left]++][0] = j;
            corners[left][sides[left]][1] = j;
          }
          corners[left][sides[left]++][0] = PARAM_PATCH_POINTS (p, 1);
        }
      }
      if (right > 0)
      {
        if (PARAM_PATCH_RANGE (p)[1][0] > PARAM_PATCH_RANGE (p)[0][0])
        {
          corners[right][sides[right]][1] = PARAM_PATCH_POINTS (p, 0);
          for (j = nside; j < n; j++)
          {
            corners[right][sides[right]++][0] = j;
            corners[right][sides[right]][1] = j;
          }
          corners[right][sides[right]++][0] =
            PARAM_PATCH_POINTS (p, 1);
        }
        else
        {
          corners[right][sides[right]][0] = PARAM_PATCH_POINTS (p, 0);
          for (j = n - 1; j >= nside; j--)
          {
            corners[right][sides[right]++][1] = j;
            corners[right][sides[right]][0] = j;
          }
          corners[right][sides[right]++][1] =
            PARAM_PATCH_POINTS (p, 1);
        }
      }
    }
    else
    {
      if (left > 0)
        sides[left] += n - nside + 1;
      if (right > 0)
        sides[right] += n - nside + 1;
    }
  }

  return (n);
}
#endif

/****************************************************************************/
/** \brief  Decompose a strip into triangles
 *
 * \todo Which function does this doc block belong to?
 *
 * @param  n,m - stripe with n+1 nodes on the bottom and m+1 nodes on the top
 * @param  c0,c1,c2,c3 - corner node ids
 * @param  s0,s1,s2,s3 - side node ids
 *
 * This function splits a stripe into triangles and calls AddBoundaryElements().
 *
 * @return <ul>
 *   <li>    0 if ok </li>
 *   <li>    1 if error occured </li>
 * </ul>
 */
/****************************************************************************/

static INT
AddBoundaryElement (INT n, INT * nodelist,
                    INT left, INT right, INT *** corners, INT * sides)
{
  if (left > 0)
  {
    if (corners != NULL)
    {
      corners[left][sides[left]][0] = nodelist[0];
      corners[left][sides[left]][1] = nodelist[1];
      corners[left][sides[left]][2] = nodelist[2];
    }
    sides[left]++;
  }
  if (right > 0)
  {
    if (corners != NULL)
    {
      corners[right][sides[right]][0] = nodelist[0];
      corners[right][sides[right]][1] = nodelist[2];
      corners[right][sides[right]][2] = nodelist[1];
    }
    sides[right]++;
  }

  return (0);
}

static INT
AddBoundaryElements (INT n, INT m,
                     INT c0, INT c1, INT c2, INT c3,
                     INT s0, INT s1, INT s2, INT s3,
                     INT left, INT right, INT *** corners, INT * sides)
{
  INT nodelist[3];

  PRINTDEBUG (dom, 1, ("    add n %d m %d c0 %d c1 %d s0 %d s1 %d\n",
                       n, m, c0, c1, s0, s1));
  if (m < n)
  {
    if (n == 1)
    {
      nodelist[0] = c0;
      nodelist[1] = c2;
      nodelist[2] = c1;
      AddBoundaryElement (3, nodelist, left, right, corners, sides);
    }
    else
    {
      nodelist[0] = c0;
      nodelist[1] = c2;
      nodelist[2] = s0;
      AddBoundaryElement (3, nodelist, left, right, corners, sides);
      c0 = s0;
      if (s0 < s1)
        s0++;
      else
        s0--;
      AddBoundaryElements (n - 1, m, c0, c1, c2, c3, s0, s1, s2, s3,
                           left, right, corners, sides);
    }
  }
  else
  {
    if (m == 1)
    {
      nodelist[0] = c0;
      nodelist[1] = c2;
      nodelist[2] = c1;
      AddBoundaryElement (3, nodelist, left, right, corners, sides);
      nodelist[0] = c1;
      nodelist[1] = c2;
      nodelist[2] = c3;
      AddBoundaryElement (3, nodelist, left, right, corners, sides);
    }
    else
    {
      nodelist[0] = c2;
      nodelist[1] = s2;
      nodelist[2] = c0;
      AddBoundaryElement (3, nodelist, left, right, corners, sides);
      c2 = s2;
      if (s2 < s3)
        s2++;
      else
        s2--;
      AddBoundaryElements (n, m - 1, c0, c1, c2, c3, s0, s1, s2, s3,
                           left, right, corners, sides);
    }
  }

  return (0);
}

static INT nc, nodeid;

/****************************************************************************/
/** \brief Decompose a patch into stripes
 *
 * @param  h - maximal length of an edge of the boundary triangles
 * @param  thePatch - poiter to a patch
 * @param  npc - number of corner nodes of the patch
 * @param  cornerid - ids of the corner nodes
 * @param  local - local coordinates of the corners
 * @param  sideid - ids of the nodes on the edges of the patch
 * @param  siden - number of nodes on the edges of the patch
 *
 * This function splits the patch into stripes and calls
 * AddBoundaryElements() to decompose the stripes into triangles.
 *
 * @return <ul>
 *   <li>    0 if ok </li>
 *   <li>    1 if error occured </li>
 * </ul>
 */
/****************************************************************************/


static INT
TriangulatePatch (HEAP * Heap, PATCH * p, BNDP ** bndp,
                  INT * sides, INT *** corners, DOUBLE h,
                  INT npc, INT * cornerid,
                  DOUBLE local[CORNERS_OF_BND_SEG][DIM - 1],
                  INT sideid[CORNERS_OF_BND_SEG][2], INT * siden)
{
  BND_PS *ps;
  INT i, k, left, right;
  DOUBLE gvect[DIM], gvect1[DIM];
  DOUBLE next_local[CORNERS_OF_BND_SEG][DIM - 1];
  DOUBLE lambda, dist, step;
  INT next_cornerid[CORNERS_OF_BND_SEG];
  INT next_sideid[CORNERS_OF_BND_SEG][2];
  INT next_siden[CORNERS_OF_BND_SEG];

  left = PARAM_PATCH_LEFT (p);
  right = PARAM_PATCH_RIGHT (p);


  PRINTDEBUG (dom, 1, ("Triang  nid %d sn %d %d %d %d\n", nodeid,
                       siden[0], siden[1], siden[2], siden[3]));

  if ((siden[npc - 1] > 1) && (siden[1] > 1))
  {
    for (k = 2; k < npc; k++)
      for (i = 0; i < DIM - 1; i++)
        next_local[k][i] = local[k][i];
    next_siden[0] = siden[0];
    next_siden[1] = siden[1] - 1;
    next_siden[2] = siden[2];
    next_siden[npc - 1] = siden[npc - 1] - 1;
    next_sideid[2][0] = sideid[2][0];
    next_sideid[2][1] = sideid[2][1];
    for (k = 2; k < npc; k++)
      next_cornerid[k] = cornerid[k];
    next_cornerid[0] = sideid[npc - 1][1];
    next_sideid[npc - 1][0] = sideid[npc - 1][0];
    if (sideid[npc - 1][0] < sideid[npc - 1][1])
      next_sideid[npc - 1][1] = sideid[npc - 1][1] - 1;
    else
      next_sideid[npc - 1][1] = sideid[npc - 1][1] + 1;
    lambda = (siden[npc - 1] - 1.0) / siden[npc - 1];
    V2_LINCOMB (lambda, local[0], (1.0 - lambda), local[npc - 1],
                next_local[0]);
    next_cornerid[1] = sideid[1][0];
    next_sideid[1][1] = sideid[1][1];
    if (sideid[1][0] < sideid[1][1])
      next_sideid[1][0] = sideid[1][0] + 1;
    else
      next_sideid[1][0] = sideid[1][0] - 1;
    lambda = (siden[1] - 1.0) / siden[1];
    V2_LINCOMB (lambda, local[1], (1.0 - lambda), local[2], next_local[1]);
    next_siden[0] = siden[0];
    if (h > 0)
    {
      if ((*PARAM_PATCH_BS (p))
          (PARAM_PATCH_BSD (p), next_local[0], gvect))
        return (-1);
      if ((*PARAM_PATCH_BS (p))
          (PARAM_PATCH_BSD (p), next_local[1], gvect1))
        return (-1);
      V_DIM_EUKLIDNORM_OF_DIFF (gvect, gvect1, dist);
      if (((INT) (dist / h)) < siden[0])
        next_siden[0] -= 1;
      else if (((INT) (dist / h)) > siden[0])
        next_siden[0] += 1;
    }
    next_sideid[0][0] = nodeid;
    next_sideid[0][1] = nodeid + next_siden[0] - 2;
    step = 1.0 / next_siden[0];
    if (bndp != NULL)
      for (i = 1; i < next_siden[0]; i++)
      {
        ps = (BND_PS *) GetFreelistMemory (Heap, sizeof (BND_PS));
        if (ps == NULL)
          return (0);
        ps->n = 1;
        ps->patch_id = PATCH_ID (p);
        lambda = i * step;
        V2_LINCOMB (lambda, next_local[1], (1.0 - lambda), next_local[0],
                    ps->local[0]);
        if (!PATCH_IS_FIXED (p))
        {
          /* store global coordinates */
          BND_DATA (ps) =
            GetFreelistMemory (Heap, DIM * sizeof (DOUBLE));
          if (BND_DATA (ps) == NULL)
            return (1);

          if (BndPointGlobal ((BNDP *) ps, (DOUBLE *) BND_DATA (ps)))
            return (1);
        }
        bndp[nodeid++] = (BNDP *) ps;


        PRINTDEBUG (dom, 1, ("    lambda nid %d %f %f\n",
                             nodeid - 1, ps->local[0][0],
                             ps->local[0][1]));

      }
    else
      nodeid += next_siden[0] - 1;

    AddBoundaryElements (siden[0], next_siden[0],
                         cornerid[0], cornerid[1],
                         next_cornerid[0], next_cornerid[1],
                         sideid[0][0], sideid[0][1],
                         next_sideid[0][0], next_sideid[0][1],
                         left, right, corners, sides);

    return (TriangulatePatch
              (Heap, p, bndp, sides, corners, h, npc, next_cornerid,
              next_local, next_sideid, next_siden));
  }
  else if ((siden[npc - 1] == 1) && (siden[1] == 1))
  {
    if (npc == 3)
      return (AddBoundaryElements (siden[0], 0,
                                   cornerid[0], cornerid[1], cornerid[2], 0,
                                   sideid[0][0], sideid[0][1], 0, 0,
                                   left, right, corners, sides));
    else
      return (AddBoundaryElements (siden[0], siden[2],
                                   cornerid[0], cornerid[1],
                                   cornerid[3], cornerid[2],
                                   sideid[0][0], sideid[0][1],
                                   sideid[2][1], sideid[2][0],
                                   left, right, corners, sides));
  }
  else if (((siden[npc - 1] > 1) && (siden[1] == 1)) ||
           ((siden[npc - 1] == 1) && (siden[1] > 1)))
  {
    if ((siden[npc - 2] > 1) && (siden[1] == 1) && (siden[0] == 1))
    {
      UserWrite ("TriangulatePatch: this case is not implemented\n");
      return (1);
    }

    /* swap sides */
    for (k = 0; k < npc; k++)
    {
      for (i = 0; i < DIM - 1; i++)
        next_local[k][i] = local[(k + 1) % npc][i];
      next_siden[k] = siden[(k + 1) % npc];
      next_sideid[k][0] = sideid[(k + 1) % npc][0];
      next_sideid[k][1] = sideid[(k + 1) % npc][1];
      next_cornerid[k] = cornerid[(k + 1) % npc];
    }
    return (TriangulatePatch
              (Heap, p, bndp, sides, corners, h, npc, next_cornerid,
              next_local, next_sideid, next_siden));
  }

  return (0);
}

#define INDEX_IN_LIST(from,to,nc)  (from < to ? from*nc+to : to*nc+from)

#ifdef __THREEDIM__
static INT
GenerateBnodes_h (HEAP * Heap, STD_BVP * theBVP, BNDP ** bndp,
                  INT * sides, INT *** corners, DOUBLE h, INT MarkKey)
{
  INT i, j, n, from, to, k;
  DOUBLE step;
  DOUBLE lambda;
  DOUBLE dist, global[CORNERS_OF_BND_SEG][DIM];
  DOUBLE local[CORNERS_OF_BND_SEG][DIM - 1];
  PATCH *p;
  BNDP *bp;
  INT *vlist;
  INT cornerid[CORNERS_OF_BND_SEG], npc;
  INT sideid[CORNERS_OF_BND_SEG][2], siden[CORNERS_OF_BND_SEG];

  nc = theBVP->ncorners;
  nodeid = nc;
  if (bndp == NULL)
  {
    vlist = (INT *) GetTmpMem (Heap, nc * nc * sizeof (INT), MarkKey);
    if (vlist == NULL)
      return (nc);
  }

  for (i = 0; i < nc; i++)
    for (j = 0; j < nc; j++)
      vlist[INDEX_IN_LIST (i, j, nc)] = 0;

  for (i = 0; i <= theBVP->numOfSubdomains; i++)
    sides[i] = 0;

  for (i = theBVP->sideoffset; i < theBVP->sideoffset + theBVP->nsides; i++)
  {
    p = theBVP->patches[i];
    npc = 4;
    local[0][0] = PARAM_PATCH_RANGE (p)[0][0];
    local[0][1] = PARAM_PATCH_RANGE (p)[0][1];
    local[1][0] = PARAM_PATCH_RANGE (p)[1][0];
    local[1][1] = PARAM_PATCH_RANGE (p)[0][1];
    local[2][0] = PARAM_PATCH_RANGE (p)[1][0];
    local[2][1] = PARAM_PATCH_RANGE (p)[1][1];
    local[3][0] = PARAM_PATCH_RANGE (p)[0][0];
    local[3][1] = PARAM_PATCH_RANGE (p)[1][1];
    for (k = 0; k < npc; k++)
      if ((*PARAM_PATCH_BS (p))(PARAM_PATCH_BSD (p), local[k], global[k]))
        return (-1);

    for (k = 0; k < npc; k++)
      PRINTDEBUG (dom, 1, (" gl %d %f %f %f\n",
                           PARAM_PATCH_POINTS (p, k),
                           global[k][0], global[k][1], global[k][2]));

    /* sorry for that */
    local[1][0] = PARAM_PATCH_RANGE (p)[0][0];
    local[1][1] = PARAM_PATCH_RANGE (p)[0][1];
    local[2][0] = PARAM_PATCH_RANGE (p)[1][0];
    local[2][1] = PARAM_PATCH_RANGE (p)[0][1];
    local[3][0] = PARAM_PATCH_RANGE (p)[1][0];
    local[3][1] = PARAM_PATCH_RANGE (p)[1][1];
    local[0][0] = PARAM_PATCH_RANGE (p)[0][0];
    local[0][1] = PARAM_PATCH_RANGE (p)[1][1];

    from = PARAM_PATCH_POINTS (p, npc - 1);
    for (k = 0; k < npc; k++)
    {
      to = PARAM_PATCH_POINTS (p, k);
      cornerid[k] = from;
      V_DIM_EUKLIDNORM_OF_DIFF (global[(k + npc - 1) % npc], global[k],
                                dist);
      if (h > 0)
        n = MAX (1, 1.00001 * dist / h);
      else
        n = MAX (1, -h);
      siden[k] = n;
      sideid[k][0] = vlist[INDEX_IN_LIST (from, to, nc)];
      if (sideid[k][0] > 0)
      {
        if (from < to)
          sideid[k][1] = sideid[k][0] + n - 2;
        else
        {
          sideid[k][1] = sideid[k][0];
          sideid[k][0] = sideid[k][1] + n - 2;
        }
      }
      else
      {
        vlist[INDEX_IN_LIST (from, to, nc)] = nodeid;
        if (from < to)
        {
          sideid[k][0] = nodeid;
          sideid[k][1] = nodeid + n - 2;
        }
        else
        {
          sideid[k][1] = nodeid;
          sideid[k][0] = nodeid + n - 2;
        }
        if (bndp != NULL)
        {
          step = 1.0 / n;
          lambda = 0.0;
          for (j = 1; j < n; j++)
          {
            lambda += step;
            bp = CreateBndPOnLine (Heap, theBVP->patches[from],
                                   theBVP->patches[to], lambda);
            if (bp == NULL)
              return (-1);
            bndp[nodeid++] = bp;
          }
        }
        else
          nodeid += (n - 1);
      }

      PRINTDEBUG (dom, 1, (" VID %d %d n %d dist %f sideid %d %d\n",
                           from, to, n, dist,
                           sideid[k][0], sideid[k][1]));

      from = to;
    }
    if (TriangulatePatch (Heap, p, bndp, sides, corners, h, npc,
                          cornerid, local, sideid, siden))
      return (-1);
  }

  return (nodeid);
}
#endif

MESH *NS_PREFIX
BVP_GenerateMesh (HEAP * Heap, BVP * aBVP, INT argc, char **argv, INT MarkKey)
{
  STD_BVP *theBVP;
  INT i, j, m, n;
  MESH *mesh;
  CoeffProcPtr coeff;
  float h;
  int ic;

  theBVP = GetSTD_BVP (aBVP);

  PRINTDEBUG (dom, 1, (" bvp nsubcf %x\n", theBVP->numOfSubdomains));
  for (i = currBVP->sideoffset; i < currBVP->sideoffset + theBVP->nsides; i++)
    PRINTDEBUG (dom, 1, (" addr %x\n", PARAM_PATCH_BS (currBVP->patches[i])));

  mesh = (MESH *) GetMem (Heap, sizeof (MESH), FROM_BOTTOM);
  if (mesh == NULL)
    return (NULL);

  coeff = NULL;
  h = 0.0;
  for (i = 1; i < argc; i++)
    if (argv[i][0] == 'h')
    {
      if (sscanf (argv[i], "h %f", &h) != 1)
        h = 0.0;
    }
    else if (argv[i][0] == 'm')
    {
      if (sscanf (argv[i], "m %d", &ic) == 1)
        if (BVP_SetCoeffFct ((BVP *) theBVP, ic, &coeff))
          coeff = NULL;
    }

  mesh->nInnP = 0;
  mesh->nElements = NULL;
  mesh->Element_corners = NULL;
  mesh->Element_corner_ids = NULL;
  mesh->nSubDomains = theBVP->numOfSubdomains;
  mesh->nSides =
    (INT *) GetMem (Heap, (theBVP->numOfSubdomains + 1) * sizeof (INT),
                    FROM_BOTTOM);
  if (mesh->nSides == NULL)
    return (NULL);
  for (i = 0; i <= mesh->nSubDomains; i++)
    mesh->nSides[i] = 0;
  mesh->Side_corners =
    (INT **) GetMem (Heap, (theBVP->numOfSubdomains + 1) * sizeof (INT *),
                     FROM_BOTTOM);
  if (mesh->Side_corners == NULL)
    return (NULL);
  mesh->Side_corner_ids =
    (INT ***) GetMem (Heap, (theBVP->numOfSubdomains + 1) * sizeof (INT **),
                      FROM_BOTTOM);
  if (mesh->Side_corner_ids == NULL)
    return (NULL);

  n = theBVP->ncorners;
  if (coeff != NULL)
    n = GenerateBnodes (Heap, theBVP, NULL, mesh->nSides, NULL, coeff);
  else if (h > 0)
    n =
      GenerateBnodes_h (Heap, theBVP, NULL, mesh->nSides, NULL, (DOUBLE) h,
                        MarkKey);

  if (n == -1)
    return (NULL);
  mesh->nBndP = n;
  mesh->theBndPs = (BNDP **) GetMem (Heap, n * sizeof (BNDP *), FROM_BOTTOM);
  if (mesh->theBndPs == NULL)
    return (NULL);

  for (i = 0; i <= mesh->nSubDomains; i++)
    PRINTDEBUG (dom, 1, ("mesh sd i %d m %d\n", i, mesh->nSides[i]));
  PRINTDEBUG (dom, 1, ("mesh n %d\n", mesh->nBndP));
  PRINTDEBUG (dom, 1, ("ncorners %d\n", theBVP->ncorners));
  for (i = 0; i < theBVP->ncorners; i++)
    PRINTDEBUG (dom, 1, (" id %d\n", PATCH_ID (theBVP->patches[i])));

  if (CreateCornerPoints (Heap, theBVP, mesh->theBndPs))
    return (NULL);

  for (i = 0; i < theBVP->ncorners; i++)
    PRINTDEBUG (dom, 1, ("   i %d  patch id %d\n", i,
                         ((BND_PS *) (mesh->theBndPs[i]))->patch_id));

  for (i = 0; i <= mesh->nSubDomains; i++)
  {
    m = mesh->nSides[i];
    if (m == 0)
    {
      mesh->Side_corners[i] = NULL;
      mesh->Side_corner_ids[i] = NULL;
    }
    else
    {
      mesh->Side_corners[i] =
        (INT *) GetMem (Heap, m * sizeof (INT), FROM_BOTTOM);
      if (mesh->Side_corners[i] == NULL)
        return (NULL);
      mesh->Side_corner_ids[i] =
        (INT **) GetMem (Heap, m * sizeof (INT *), FROM_BOTTOM);
      if (mesh->Side_corner_ids[i] == NULL)
        return (NULL);
    }
    for (j = 0; j < m; j++)
    {
      mesh->Side_corners[i][j] = DIM;

      PRINTDEBUG (dom, 1, ("  i %d m %d j %d size %d\n", i, m, j,
                           mesh->Side_corners[i][j]));

      mesh->Side_corner_ids[i][j] =
        (INT *) GetMem (Heap, DIM * sizeof (INT), FROM_BOTTOM);
      if (mesh->Side_corner_ids[i][j] == NULL)
        return (NULL);
    }
  }

  if (coeff != NULL)
    n = GenerateBnodes (Heap, theBVP, mesh->theBndPs,
                        mesh->nSides, mesh->Side_corner_ids, coeff);
  else if (h > 0)
    n = GenerateBnodes_h (Heap, theBVP, mesh->theBndPs,
                          mesh->nSides, mesh->Side_corner_ids, (DOUBLE) h,
                          MarkKey);
  if (n == -1)
    return (NULL);

  for (i = 0; i <= mesh->nSubDomains; i++)
  {
    m = mesh->nSides[i];
    PRINTDEBUG (dom, 1, ("mesh sd i %d m %d\n", i, m));
    for (j = 0; j < m; j++)
      PRINTDEBUG (dom, 1, ("mesh face j %d (%d,%d,%d)\n", j,
                           mesh->Side_corner_ids[i][j][0],
                           mesh->Side_corner_ids[i][j][1],
                           mesh->Side_corner_ids[i][j][2]));
  }

  for (i = 0; i < mesh->nBndP; i++)
    PRINTDEBUG (dom, 1, ("   i %d  patch id %d\n", i,
                         ((BND_PS *) (mesh->theBndPs[i]))->patch_id));

  mesh->VertexLevel = NULL;
  mesh->VertexPrio = NULL;

  return (mesh);
}

/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/* functions for BNDS                                                       */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/

static INT
PatchGlobal (PATCH * p, DOUBLE * lambda, DOUBLE * global)
{
  if (PATCH_TYPE (p) == PARAMETRIC_PATCH_TYPE)
    return ((*PARAM_PATCH_BS (p))(PARAM_PATCH_BSD (p), lambda, global));
  else if (PATCH_TYPE (p) == LINEAR_PATCH_TYPE)
  {
#ifdef __TWODIM__
    global[0] = (1 - lambda[0]) * LINEAR_PATCH_POS (p, 0)[0]
                + lambda[0] * LINEAR_PATCH_POS (p, 1)[0];
    global[1] = (1 - lambda[0]) * LINEAR_PATCH_POS (p, 0)[1]
                + lambda[0] * LINEAR_PATCH_POS (p, 1)[1];
#endif
#ifdef __THREEDIM__
    global[0] = (1 - lambda[0] - lambda[1]) * LINEAR_PATCH_POS (p, 0)[0]
                + lambda[0] * LINEAR_PATCH_POS (p, 1)[0]
                + lambda[1] * LINEAR_PATCH_POS (p, 2)[0];
    global[1] = (1 - lambda[0] - lambda[1]) * LINEAR_PATCH_POS (p, 0)[1]
                + lambda[0] * LINEAR_PATCH_POS (p, 1)[1]
                + lambda[1] * LINEAR_PATCH_POS (p, 2)[1];
    global[2] = (1 - lambda[0] - lambda[1]) * LINEAR_PATCH_POS (p, 0)[2]
                + lambda[0] * LINEAR_PATCH_POS (p, 1)[2]
                + lambda[1] * LINEAR_PATCH_POS (p, 2)[2];
#endif
    return (0);
  }

  return (1);
}

static INT
FreeBNDS_Global (BND_PS * ps, DOUBLE * local, DOUBLE * global)
{
  BND_PS **ppt;
  PATCH *p;
  DOUBLE *pos[4];
  INT i;

  p = currBVP->patches[ps->patch_id];
  if (p == NULL)
    return (1);

  /* get corner coordinates */
  ppt = (BND_PS **) BND_DATA (ps);
  ASSERT (BND_N (ps) <= 4);
  for (i = 0; i < BND_N (ps); i++)
  {
    ASSERT (ppt != NULL);
    pos[i] = (DOUBLE *) BND_DATA (*(ppt++));
  }

  /* claculate global coordinates */
#ifdef __TWODIM__
  for (i = 0; i < DIM; i++)
    global[i] = (1.0 - local[0]) * pos[0][i] + local[0] * pos[1][i];
#endif
#ifdef __THREEDIM__
  switch (ps->n)
  {
  case 3 :
    for (i = 0; i < DIM; i++)
      global[i] = (1.0 - local[0] - local[1]) * pos[0][i]
                  + local[0] * pos[1][i] + local[1] * pos[2][i];
    break;
  case 4 :
    for (i = 0; i < DIM; i++)
      global[0] = (1.0 - local[0]) * (1.0 - local[1]) * pos[0][i]
                  + local[0] * (1.0 - local[1]) * pos[1][i]
                  + local[0] * local[1] * pos[2][i]
                  + (1.0 - local[0]) * local[1] * pos[3][i];
    break;
  }
#endif

  return (0);
}

static INT
local2lambda (BND_PS * ps, DOUBLE local[], DOUBLE lambda[])
{
  PATCH *p;

  p = currBVP->patches[ps->patch_id];

  if ((PATCH_TYPE (p) == PARAMETRIC_PATCH_TYPE)
      || (PATCH_TYPE (p) == LINEAR_PATCH_TYPE))
#ifdef __TWODIM__
    lambda[0] =
      (1.0 - local[0]) * ps->local[0][0] + local[0] * ps->local[1][0];
#endif
#ifdef __THREEDIM__
    switch (ps->n)
    {
    case 3 :
      lambda[0] = (1.0 - local[0] - local[1]) * ps->local[0][0]
                  + local[0] * ps->local[1][0] + local[1] * ps->local[2][0];
      lambda[1] = (1.0 - local[0] - local[1]) * ps->local[0][1]
                  + local[0] * ps->local[1][1] + local[1] * ps->local[2][1];
      break;
    case 4 :
      lambda[0] = (1.0 - local[0]) * (1.0 - local[1]) * ps->local[0][0]
                  + local[0] * (1.0 - local[1]) * ps->local[1][0]
                  + local[0] * local[1] * ps->local[2][0]
                  + (1.0 - local[0]) * local[1] * ps->local[3][0];
      lambda[1] = (1.0 - local[0]) * (1.0 - local[1]) * ps->local[0][1]
                  + local[0] * (1.0 - local[1]) * ps->local[1][1]
                  + local[0] * local[1] * ps->local[2][1]
                  + (1.0 - local[0]) * local[1] * ps->local[3][1];
      break;
    }
#endif
    else
      return (1);

  return (0);
}

/* domain interface function: for description see domain.h */
INT NS_PREFIX
BNDS_Global (BNDS * aBndS, DOUBLE * local, DOUBLE * global)
{
  BND_PS *ps;
  PATCH *p;
  DOUBLE lambda[DIM_OF_BND];

  ps = (BND_PS *) aBndS;
  p = currBVP->patches[ps->patch_id];
  if (p == NULL)
    return (1);

  IF_MARC (aBndS) return (M_BNDS_Global (aBndS, local, global));

  PRINTDEBUG (dom, 1, (" Bnds global loc %f pid %d\n",
                       local[0], PATCH_ID (p)));

  if (PATCH_IS_FREE (p))
    return (FreeBNDS_Global (ps, local, global));

  if (local2lambda (ps, local, lambda))
    return (1);

  return (PatchGlobal (p, lambda, global));
}

static INT
SideIsCooriented (BND_PS * ps)
{
#       ifdef __TWODIM__
  if (BND_LOCAL (ps, 1)[0] > BND_LOCAL (ps, 0)[0])
    return (YES);
  else
    return (NO);
#       endif

#       ifdef __THREEDIM__
  DOUBLE vp, x0[2], x1[2];

  ASSERT (BND_N (ps) >= 3);

  /* check whether an (arbitrary) angle of the side is > 180 degree */
  V2_SUBTRACT (BND_LOCAL (ps, 1), BND_LOCAL (ps, 0), x0);
  V2_SUBTRACT (BND_LOCAL (ps, 2), BND_LOCAL (ps, 0), x1);
  V2_VECTOR_PRODUCT (x1, x0, vp);

  ASSERT (fabs (vp) > SMALL_C);

  if (vp > SMALL_C)
    return (YES);
  else
    return (NO);
#       endif
}

/* domain interface function: for description see domain.h */
INT NS_PREFIX
BNDS_BndCond (BNDS * aBndS, DOUBLE * local, DOUBLE * in, DOUBLE * value,
              INT * type)
{
  BND_PS *ps;
  PATCH *p;
  DOUBLE lambda[DOM_N_IN_PARAMS], global[DOM_N_IN_PARAMS];
  INT i;

  PRINTDEBUG (dom, 1, (" BndCond loc %f\n", local[0]));

  ps = (BND_PS *) aBndS;
  if (ps == NULL)
    return (1);

  IF_MARC (aBndS) return (M_BNDS_BndCond (aBndS, local, in, value, type));

  p = currBVP->patches[ps->patch_id];

  PRINTDEBUG (dom, 1, (" BndCond %d %x\n", PATCH_TYPE (p), p));

  if (currBVP->GeneralBndCond != NULL)
  {
    type[0] = PATCH_ID (p) - currBVP->sideoffset;
    if (PATCH_IS_FREE (p))
    {
      if (FreeBNDS_Global (ps, local, global))
        return (1);
    }
    else
    {
      if (local2lambda (ps, local, lambda))
        return (1);
      if (PatchGlobal (p, lambda, global))
        return (1);
    }
    /* besides gobal coordinates return also whether element lies left or right of boundary */
    global[DOM_EVAL_FOR_SD] =
      (SideIsCooriented (ps)) ? PARAM_PATCH_LEFT (p) :
      PARAM_PATCH_RIGHT (p);
    if (in == NULL)
      return ((*(currBVP->GeneralBndCond))
                (NULL, NULL, global, value, type));

    for (i = 0; i < DOM_N_IN_PARAMS; i++)
      in[i] = global[i];

    return ((*(currBVP->GeneralBndCond))(NULL, NULL, in, value, type));
  }

  /* give segment-id information similar to GeneralBndCond */
  type[0] = PATCH_ID (p) - currBVP->sideoffset;

  if (local2lambda (ps, local, lambda))
    return (1);

  /* besides parametric coordinates return also whether element lies left or right of boundary */
  lambda[DOM_EVAL_FOR_SD] =
    (SideIsCooriented (ps)) ? PARAM_PATCH_LEFT (p) : PARAM_PATCH_RIGHT (p);
  if (in == NULL)
    return ((*PARAM_PATCH_BC (p))
            (PARAM_PATCH_BCD (p), PARAM_PATCH_BSD (p), lambda, value, type));

  for (i = 0; i < DOM_N_IN_PARAMS; i++)
    in[i] = lambda[i];
  return ((*PARAM_PATCH_BC (p))
          (PARAM_PATCH_BCD (p), PARAM_PATCH_BSD (p), in, value, type));
}

/* domain interface function: for description see domain.h */
INT NS_PREFIX
BNDS_BndSDesc (BNDS * theBndS, INT * id, INT * nbid, INT * part)
{
  BND_PS *ps;
  PATCH *p;
  INT left, right;

  IF_MARC (theBndS) return (M_BNDS_BndSDesc (theBndS, id, nbid, part));

  ps = (BND_PS *) theBndS;
  p = currBVP->patches[ps->patch_id];

  /* fill part from segment */
  if (STD_BVP_NDOMPART (currBVP) > 1)
  {
    *part = DPI_SG2P (DOMAIN_PARTINFO (STD_BVP_DOMAIN (currBVP)),
                      PATCH_ID (p) - STD_BVP_SIDEOFFSET (currBVP));
    /* this expression yields the segment id */
  }
  else
    /* default is 0 */
    *part = 0;

  if (PATCH_TYPE (p) == PARAMETRIC_PATCH_TYPE)
  {
    left = PARAM_PATCH_LEFT (p);
    right = PARAM_PATCH_RIGHT (p);
  }
  else if (PATCH_TYPE (p) == LINEAR_PATCH_TYPE)
  {
    left = LINEAR_PATCH_LEFT (p);
    right = LINEAR_PATCH_RIGHT (p);
  }
  else
    return (1);

  /* check orientation */
  if (SideIsCooriented (ps))
  {
    /* patch and side are co-oriented */
    *id = left;
    *nbid = right;
  }
  else
  {
    /* patch and side are anti-oriented */
    *id = right;
    *nbid = left;
  }

  return (0);
}

/* domain interface function: for description see domain.h */
BNDP *NS_PREFIX
BNDS_CreateBndP (HEAP * Heap, BNDS * aBndS, DOUBLE * local)
{
  BND_PS *ps, *pp;
  PATCH *p;

  if (aBndS == NULL)
    return (NULL);

  IF_MARC (aBndS) return (M_BNDS_CreateBndP (Heap, aBndS, local));

  ps = (BND_PS *) aBndS;
  p = currBVP->patches[ps->patch_id];

  pp = (BND_PS *) GetFreelistMemory (Heap, sizeof (BND_PS));
  if (pp == NULL)
    return (NULL);

  pp->patch_id = ps->patch_id;
  pp->n = 1;

  if (local2lambda (ps, local, pp->local[0]))
    return (NULL);

  if (!PATCH_IS_FIXED (p))
  {
    /* store global coordinates */
    BND_DATA (pp) = GetFreelistMemory (Heap, DIM * sizeof (DOUBLE));
    if (BND_DATA (pp) == NULL)
      return (NULL);

    if (FreeBNDS_Global (ps, pp->local[0], (DOUBLE *) BND_DATA (pp)))
      return (NULL);
  }

  PRINTDEBUG (dom, 1, (" BNDP s %d\n", pp->patch_id));

  return ((BNDP *) pp);
}

/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/* functions for BNDP                                                       */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/

static INT
BndPointGlobal (BNDP * aBndP, DOUBLE * global)
{
  BND_PS *ps;
  PATCH *p, *s;
  INT j, k;
  DOUBLE pglobal[DIM];

  ps = (BND_PS *) aBndP;
  p = currBVP->patches[ps->patch_id];

  PRINTDEBUG (dom, 1, (" bndp pid %d %d %d\n", ps->patch_id,
                       PATCH_ID (p), PATCH_TYPE (p)));

  switch (PATCH_TYPE (p))
  {
  case PARAMETRIC_PATCH_TYPE :
  case LINEAR_PATCH_TYPE :
    return (PatchGlobal (p, ps->local[0], global));
  case POINT_PATCH_TYPE :
    s = currBVP->patches[POINT_PATCH_PID (p, 0)];
    PRINTDEBUG (dom, 1, (" bndp n %d %d loc %f %f gl \n",
                         POINT_PATCH_N (p),
                         POINT_PATCH_PID (p, 0),
                         ps->local[0][0], ps->local[0][1]));
    if ((*PARAM_PATCH_BS (s))(PARAM_PATCH_BSD (s), ps->local[0], global))
      REP_ERR_RETURN (1);
    PRINTDEBUG (dom, 1, (" bndp n %d %d loc %f %f gl %f %f %f\n",
                         POINT_PATCH_N (p),
                         POINT_PATCH_PID (p, 0),
                         ps->local[0][0],
                         ps->local[0][1], global[0], global[1], global[2]));
    for (j = 1; j < POINT_PATCH_N (p); j++)
    {
      s = currBVP->patches[POINT_PATCH_PID (p, j)];
      if ((*PARAM_PATCH_BS (s))
          (PARAM_PATCH_BSD (s), ps->local[j], pglobal))
        REP_ERR_RETURN (1);
      PRINTDEBUG (dom, 1, (" bndp    j %d %d loc %f %f gl %f %f %f\n", j,
                           POINT_PATCH_PID (p, j),
                           ps->local[j][0],
                           ps->local[j][1],
                           pglobal[0], pglobal[1], pglobal[2]));
      for (k = 0; k < DIM; k++)
        if (ABS (pglobal[k] - global[k]) > SMALL_DIFF)
          REP_ERR_RETURN (1);
    }
    return (0);
#ifdef __THREEDIM__
  case LINE_PATCH_TYPE :
    s = currBVP->patches[LINE_PATCH_PID (p, 0)];
    if ((*PARAM_PATCH_BS (s))(PARAM_PATCH_BSD (s), ps->local[0], global))
      REP_ERR_RETURN (1);
    PRINTDEBUG (dom, 1, (" bndp    n %d %d loc %f %f gl %f %f %f\n",
                         POINT_PATCH_N (p),
                         LINE_PATCH_PID (p, 0),
                         ps->local[0][0],
                         ps->local[0][1], global[0], global[1], global[2]));
    for (j = 1; j < LINE_PATCH_N (p); j++)
    {
      s = currBVP->patches[LINE_PATCH_PID (p, j)];
      if ((*PARAM_PATCH_BS (s))
          (PARAM_PATCH_BSD (s), ps->local[j], pglobal))
        return (1);
      PRINTDEBUG (dom, 1, (" bndp    j %d %d loc %f %f gl %f %f %f\n", j,
                           LINE_PATCH_PID (p, j),
                           ps->local[j][0],
                           ps->local[j][1],
                           pglobal[0], pglobal[1], pglobal[2]));
      for (k = 0; k < DIM; k++)
        if (ABS (pglobal[k] - global[k]) > SMALL_DIFF)
          REP_ERR_RETURN (1);
    }
    return (0);
#endif
  }

  REP_ERR_RETURN (1);
}

/* domain interface function: for description see domain.h */
INT NS_PREFIX
BNDP_Global (BNDP * aBndP, DOUBLE * global)
{
  BND_PS *ps = (BND_PS *) aBndP;
  DOUBLE *pos;
  INT i;

  IF_MARC (aBndP) return (M_BNDP_Global (aBndP, global));

  if (!PATCH_IS_FIXED (currBVP->patches[ps->patch_id]))
  {
    /* copy globals */
    pos = (DOUBLE *) BND_DATA (ps);
    for (i = 0; i < DIM; i++)
      global[i] = pos[i];
    return (0);
  }

  return (BndPointGlobal (aBndP, global));
}

/* domain interface function: for description see domain.h */
INT NS_PREFIX
BNDP_BndPDesc (BNDP * theBndP, INT * move, INT * part)
{
  BND_PS *ps;
  PATCH *p;

  IF_MARC (theBndP) return (M_BNDP_BndPDesc (theBndP, move, part));

  ps = (BND_PS *) theBndP;
  p = STD_BVP_PATCH (currBVP, ps->patch_id);

  /* default part is 0 */
  *part = 0;

  switch (PATCH_TYPE (p))
  {
  case PARAMETRIC_PATCH_TYPE :
  case LINEAR_PATCH_TYPE :
    if (STD_BVP_NDOMPART (currBVP) > 1)
      *part = DPI_SG2P (DOMAIN_PARTINFO (STD_BVP_DOMAIN (currBVP)), PATCH_ID (p) - STD_BVP_SIDEOFFSET (currBVP));       /* <-- this expression yields the segment id */
    *move = PATCH_IS_FREE (p) ? DIM : DIM_OF_BND;
    return (0);

  case POINT_PATCH_TYPE :
    if (STD_BVP_NDOMPART (currBVP) > 1)
      *part = DPI_PT2P (DOMAIN_PARTINFO (STD_BVP_DOMAIN (currBVP)), PATCH_ID (p));      /* <-- this expression yields the corner id */
    *move = PATCH_IS_FREE (p) ? DIM : 0;
    return (0);

#               ifdef __THREEDIM__
  case LINE_PATCH_TYPE :
    if (STD_BVP_NDOMPART (currBVP) > 1)
      *part = DPI_LN2P (DOMAIN_PARTINFO (STD_BVP_DOMAIN (currBVP)),
                        LINE_PATCH_C0 (p), LINE_PATCH_C1 (p));
    *move = PATCH_IS_FREE (p) ? DIM : 1;
    return (0);
#               endif
  }

  return (1);
}

/* domain interface function: for description see domain.h */
INT NS_PREFIX
BNDP_BndEDesc (BNDP * aBndP0, BNDP * aBndP1, INT * part)
{
  BND_PS *bp0, *bp1;
  PATCH *p, *p0, *p1;
  INT pid, cnt;

  bp0 = (BND_PS *) aBndP0;
  p0 = currBVP->patches[bp0->patch_id];
  bp1 = (BND_PS *) aBndP1;
  p1 = currBVP->patches[bp1->patch_id];

  if ((bp0 == NULL) || (bp1 == NULL))
    REP_ERR_RETURN (1);

  IF_MARC (aBndP0) return (M_BNDP_BndEDesc (aBndP0, aBndP1, part));

  /* default part is 0 */
  *part = 0;

  if (STD_BVP_NDOMPART (currBVP) == 1)
    return (0);

  /* find common patch(es) of boundary points */
  cnt = GetNumberOfCommonPatches (p0, p1, &pid);
  if (cnt == 0)
    return (1);

#ifdef __THREEDIM__
  if (cnt > 1)
  {
    pid = GetCommonLinePatchId (p0, p1);
    ASSERT ((pid < currBVP->ncorners) || (pid >= currBVP->sideoffset));
    p = STD_BVP_PATCH (currBVP, pid);

    *part = DPI_LN2P (DOMAIN_PARTINFO (STD_BVP_DOMAIN (currBVP)),
                      LINE_PATCH_C0 (p), LINE_PATCH_C1 (p));
    return (0);
  }
#endif

  p = STD_BVP_PATCH (currBVP, pid);
  switch (PATCH_TYPE (p))
  {
  case PARAMETRIC_PATCH_TYPE :
  case LINEAR_PATCH_TYPE :
    *part = DPI_SG2P (DOMAIN_PARTINFO (STD_BVP_DOMAIN (currBVP)), PATCH_ID (p) - STD_BVP_SIDEOFFSET (currBVP));         /* <-- this expression yields the segment id */
    break;
  default :
    REP_ERR_RETURN (1);
  }

  return (0);
}

/* domain interface function: for description see domain.h */
BNDS *NS_PREFIX
BNDP_CreateBndS (HEAP * Heap, BNDP ** aBndP, INT n)
{
  BND_PS *bp[4], *bs, **pps;
  PATCH *p[4];
  DOUBLE *lambda[4];
  INT i, j, l, pid;
#       ifdef __THREEDIM__
  INT k;
#       endif

  IF_MARC (aBndP[0]) return (M_BNDP_CreateBndS (Heap, aBndP, n));

  PRINTDEBUG (dom, 1, ("Create BNDS:\n"));
  for (i = 0; i < n; i++)
  {
    bp[i] = (BND_PS *) aBndP[i];
    p[i] = currBVP->patches[bp[i]->patch_id];

    PRINTDEBUG (dom, 1, (" bp %d p %d n %d\n",
                         bp[i]->patch_id, PATCH_ID (p[i]), n));
    for (l = 0; l < GetNumberOfPatches (p[i]); l++)
      PRINTDEBUG (dom, 1, ("    bp pid %d\n", GetPatchId (p[i], l)));
  }

  pid = -1;
#ifdef __TWODIM__
  if (n != 2)
    return (NULL);
  /* find common patch of boundary points */
  for (i = 0; i < GetNumberOfPatches (p[0]); i++)
    for (j = 0; j < GetNumberOfPatches (p[1]); j++)
    {
      PRINTDEBUG (dom, 1, (" pid i, j %d %d %d %d\n", i, j,
                           GetPatchId (p[0], i), GetPatchId (p[1], j)));
      if (GetPatchId (p[0], i) == GetPatchId (p[1], j))
      {
        pid = GetPatchId (p[0], i);
        lambda[0] = bp[0]->local[i];
        lambda[1] = bp[1]->local[j];
        break;
        PRINTDEBUG (dom, 1, (" pid %d \n", pid));
      }
    }
#endif
#ifdef __THREEDIM__
  switch (n)
  {
  case 3 :
    for (i = 0; i < GetNumberOfPatches (p[0]); i++)
      for (j = 0; j < GetNumberOfPatches (p[1]); j++)
        if (GetPatchId (p[0], i) == GetPatchId (p[1], j))
          for (k = 0; k < GetNumberOfPatches (p[2]); k++)
            if (GetPatchId (p[0], i) == GetPatchId (p[2], k))
            {
              pid = GetPatchId (p[0], i);
              lambda[0] = bp[0]->local[i];
              lambda[1] = bp[1]->local[j];
              lambda[2] = bp[2]->local[k];
              break;
            }
    break;
  case 4 :
    for (i = 0; i < GetNumberOfPatches (p[0]); i++)
      for (j = 0; j < GetNumberOfPatches (p[1]); j++)
        if (GetPatchId (p[0], i) == GetPatchId (p[1], j))
          for (k = 0; k < GetNumberOfPatches (p[2]); k++)
            if (GetPatchId (p[0], i) == GetPatchId (p[2], k))
              for (l = 0; l < GetNumberOfPatches (p[3]); l++)
                if (GetPatchId (p[0], i) == GetPatchId (p[3], l))
                {
                  pid = GetPatchId (p[0], i);
                  lambda[0] = bp[0]->local[i];
                  lambda[1] = bp[1]->local[j];
                  lambda[2] = bp[2]->local[k];
                  lambda[3] = bp[3]->local[l];
                  break;
                }
    break;
  }
#endif

  if (pid == -1)
    return (NULL);

  bs = (BND_PS *) GetFreelistMemory (Heap, (n - 1) * sizeof (COORD_BND_VECTOR)
                                     + sizeof (BND_PS));
  if (bs == NULL)
    return (NULL);
  bs->n = n;
  bs->patch_id = pid;
  for (i = 0; i < n; i++)
    for (j = 0; j < DIM_OF_BND; j++)
      bs->local[i][j] = lambda[i][j];

  for (i = 0; i < n; i++)
    for (j = 0; j < DIM_OF_BND; j++)
      PRINTDEBUG (dom, 1, (" bnds i, j %d %d %f\n", i, j, lambda[i][j]));

  PRINTDEBUG (dom, 1, (" Create BNDS %x %d\n", bs, pid));

  if (!PATCH_IS_FIXED (currBVP->patches[pid]))
  {
    /* store corner patch pointers */
    BND_DATA (bs) = GetFreelistMemory (Heap, n * sizeof (BND_PS *));
    if (BND_DATA (bs) == NULL)
      return (NULL);

    pps = (BND_PS **) BND_DATA (bs);
    for (i = 0; i < n; i++)
      *(pps++) = bp[i];
  }

  return ((BNDS *) bs);
}

/* domain interface function: for description see domain.h */
BNDP *NS_PREFIX
BNDP_CreateBndP (HEAP * Heap, BNDP * aBndP0, BNDP * aBndP1, DOUBLE lcoord)
{
  BND_PS *bp0, *bp1, *bp;
  PATCH *p0, *p1;
  INT i, j, k, l, cnt;

  bp0 = (BND_PS *) aBndP0;
  bp1 = (BND_PS *) aBndP1;

  if ((bp0 == NULL) || (bp1 == NULL))
    return (NULL);

  IF_MARC (aBndP0) return (M_BNDP_CreateBndP (Heap, aBndP0, aBndP1, lcoord));

  p0 = currBVP->patches[bp0->patch_id];
  p1 = currBVP->patches[bp1->patch_id];

  PRINTDEBUG (dom, 1, ("   bp0 %d pid %d\n", bp0->patch_id, PATCH_ID (p0)));
  for (l = 0; l < GetNumberOfPatches (p0); l++)
    PRINTDEBUG (dom, 1, ("    bp pid %d\n", GetPatchId (p0, l)));
  PRINTDEBUG (dom, 1, ("   bp1 %d pid %d\n", bp1->patch_id, PATCH_ID (p1)));
  for (l = 0; l < GetNumberOfPatches (p1); l++)
    PRINTDEBUG (dom, 1, ("    bp pid %d\n", GetPatchId (p1, l)));

  cnt = GetNumberOfCommonPatches (p0, p1, &i);
  if (cnt == 0)
    return (NULL);

  bp =
    (BND_PS *) GetFreelistMemory (Heap,
                                  (cnt - 1) * sizeof (COORD_BND_VECTOR) +
                                  sizeof (BND_PS));
  if (bp == NULL)
    return (NULL);
  bp->n = cnt;

#ifdef __THREEDIM__
  if (cnt > 1)
  {
    PATCH *p;
    k = GetCommonLinePatchId (p0, p1);
    if ((k < currBVP->ncorners) || (k >= currBVP->sideoffset))
      return (NULL);
    p = currBVP->patches[k];
    bp->patch_id = k;

    PRINTDEBUG (dom, 1, (" Create BNDP line %d cnt %d\n", k, cnt));
    PRINTDEBUG (dom, 1, ("   bp0 %d pid %d\n",
                         bp0->patch_id, PATCH_ID (p0)));
    for (l = 0; l < GetNumberOfPatches (p0); l++)
      PRINTDEBUG (dom, 1, ("    bp pid %d\n", GetPatchId (p0, l)));
    PRINTDEBUG (dom, 1, ("   bp1 %d pid %d\n",
                         bp1->patch_id, PATCH_ID (p1)));
    for (l = 0; l < GetNumberOfPatches (p1); l++)
      PRINTDEBUG (dom, 1, ("    bp pid %d\n", GetPatchId (p1, l)));

    for (l = 0; l < LINE_PATCH_N (p); l++)
    {
      for (i = 0; i < GetNumberOfPatches (p0); i++)
        if (GetPatchId (p0, i) == LINE_PATCH_PID (p, l))
          for (j = 0; j < GetNumberOfPatches (p1); j++)
            if (GetPatchId (p1, j) == LINE_PATCH_PID (p, l))
              for (k = 0; k < DIM_OF_BND; k++)
                bp->local[l][k] = (1.0 - lcoord) * bp0->local[i][k]
                                  + lcoord * bp1->local[j][k];
    }

    for (l = 0; l < LINE_PATCH_N (p); l++)
      PRINTDEBUG (dom, 1, (" Create BNDP line %d l %d %f %f\n",
                           LINE_PATCH_PID (p, l), l,
                           bp->local[l][0], bp->local[l][1]));

    if (!PATCH_IS_FIXED (p))
    {
      /* store global coordinates */
      BND_DATA (bp) = GetFreelistMemory (Heap, DIM * sizeof (DOUBLE));
      if (BND_DATA (bp) == NULL)
        return (NULL);

      if (BNDP_Global ((BNDP *) bp, (DOUBLE *) BND_DATA (bp)))
        return (NULL);
    }
    return ((BNDP *) bp);
  }
#endif

  for (i = 0; i < GetNumberOfPatches (p0); i++)
    for (j = 0; j < GetNumberOfPatches (p1); j++)
      if (GetPatchId (p0, i) == GetPatchId (p1, j))
      {
        bp->patch_id = GetPatchId (p0, i);
        for (k = 0; k < DIM_OF_BND; k++)
          bp->local[0][k] = (1.0 - lcoord) * bp0->local[i][k]
                            + lcoord * bp1->local[j][k];
        PRINTDEBUG (dom, 1,
                    (" Create BNDP param %d \n", GetPatchId (p0, i)));

        break;
      }

  if (!PATCH_IS_FIXED (currBVP->patches[bp->patch_id]))
  {
    DOUBLE *x, *a, *b;

    /* store global coordinates */
    BND_DATA (bp) = GetFreelistMemory (Heap, DIM * sizeof (DOUBLE));
    if (BND_DATA (bp) == NULL)
      return (NULL);

    x = (DOUBLE *) BND_DATA (bp);
    a = (DOUBLE *) BND_DATA (bp0);
    b = (DOUBLE *) BND_DATA (bp1);
    ASSERT (a != NULL);
    ASSERT (b != NULL);
    for (i = 0; i < DIM; i++)
      x[i] = a[i] * (1.0 - lcoord) + b[i] * lcoord;
  }

  return ((BNDP *) bp);
}

/* domain interface function: for description see domain.h */
INT NS_PREFIX
BNDP_Move (BNDP * aBndP, const DOUBLE global[])
{
  BND_PS *ps;
  DOUBLE *pos;
  INT i;

#ifdef ModelP
  /* TODO: parallel version */
  PrintErrorMessage ('E', "BNDP_Move", "parallel not implemented");
  ASSERT (FALSE);
#endif

  ps = (BND_PS *) aBndP;
  if (PATCH_IS_FREE (currBVP->patches[ps->patch_id]))
  {
    /* copy globals */
    pos = (DOUBLE *) BND_DATA (ps);
    for (i = 0; i < DIM; i++)
      pos[i] = global[i];
    return (0);
  }

  return (1);
}

/* domain interface function: for description see domain.h */
INT NS_PREFIX
BNDP_SaveInsertedBndP (BNDP * theBndP, char *data, INT max_data_size)
{
  BND_PS *bp;
  PATCH *p;
  INT pid;

  bp = (BND_PS *) theBndP;

  if (bp == NULL)
    return (1);

  IF_MARC (theBndP)
  return (M_BNDP_SaveInsertedBndP (theBndP, data, max_data_size));

  pid = bp->patch_id;
  p = currBVP->patches[pid];

  switch (PATCH_TYPE (p))
  {
  case PARAMETRIC_PATCH_TYPE :
  case LINEAR_PATCH_TYPE :
    pid -= currBVP->sideoffset;
    break;
  case POINT_PATCH_TYPE :
    pid = POINT_PATCH_PID (p, 0) - currBVP->sideoffset;
    break;
#ifdef __THREEDIM__
  case LINE_PATCH_TYPE :
    pid = LINE_PATCH_PID (p, 0) - currBVP->sideoffset;
    break;
#endif
  }

  PRINTDEBUG (dom, 1, (" Insert pid %d %d\n", bp->patch_id, pid));

#ifdef __TWODIM__
  if (sprintf (data, "bn %d %f", pid, (float) bp->local[0][0]) >
      max_data_size)
    return (1);
#endif

#ifdef __THREEDIM__
  if (sprintf (data, "bn %d %f %f", (int) pid,
               (float) bp->local[0][0],
               (float) bp->local[0][1]) > max_data_size)
    return (1);
#endif

  return (0);
}

/* domain interface function: for description see domain.h */
INT
BNDP_BndCond (BNDP * aBndP, INT * n, INT i, DOUBLE * in, DOUBLE * value,
              INT * type)
{
  BND_PS *ps;
  PATCH *p;
  DOUBLE global[DOM_N_IN_PARAMS];
  DOUBLE *local;

  if (i < 0)
    return (1);

  ps = (BND_PS *) aBndP;
  if (ps == NULL)
    return (1);

  IF_MARC (aBndP) return (M_BNDP_BndCond (aBndP, n, i, in, value, type));

  p = currBVP->patches[ps->patch_id];

  switch (PATCH_TYPE (p))
  {
  case PARAMETRIC_PATCH_TYPE :
  case LINEAR_PATCH_TYPE :
    n[0] = 1;
    local = ps->local[0];
    break;
  case POINT_PATCH_TYPE :
    n[0] = POINT_PATCH_N (p);
    if (i >= n[0])
      return (1);
    p = currBVP->patches[POINT_PATCH_PID (p, i)];
    local = ps->local[i];
    break;
#ifdef __THREEDIM__
  case LINE_PATCH_TYPE :
    n[0] = LINE_PATCH_N (p);
    if (i >= n[0])
      return (1);
    p = currBVP->patches[LINE_PATCH_PID (p, i)];
    local = ps->local[i];
    break;
#endif
  }

  PRINTDEBUG (dom, 1, (" BndCond %d loc %f %f\n",
                       PATCH_ID (p), local[0], local[1]));

  if (PATCH_TYPE (p) != PARAMETRIC_PATCH_TYPE)
    return (1);

  if (currBVP->GeneralBndCond != NULL)
  {
    type[0] = PATCH_ID (p) - currBVP->sideoffset;
    if (PATCH_IS_FREE (p))
    {
      DOUBLE *pos = (DOUBLE *) BND_DATA (ps);

      /* copy globals */
      for (i = 0; i < DIM; i++)
        global[i] = pos[i];
    }
    else if (PatchGlobal (p, local, global))
      return (1);

    global[DOM_EVAL_FOR_SD] = DOM_EVAL_SD_UNKNOWN;
    if (in == NULL)
      return ((*(currBVP->GeneralBndCond))
                (NULL, NULL, global, value, type));
    for (i = 0; i < DIM; i++)
      in[i] = global[i];
    /* maybe the user has set in[DOM_EVAL_FOR_SD]: don't erase it */
    return ((*(currBVP->GeneralBndCond))(NULL, NULL, in, value, type));
  }

  if (in == NULL)
  {
    DOUBLE loc[DOM_N_IN_PARAMS];
    for (i = 0; i < DIM_OF_BND; i++)
      loc[i] = local[i];
    loc[DOM_EVAL_FOR_SD] = DOM_EVAL_SD_UNKNOWN;
    return ((*PARAM_PATCH_BC (p))
            (PARAM_PATCH_BCD (p), PARAM_PATCH_BSD (p), local, value, type));
  }

  for (i = 0; i < DIM_OF_BND; i++)
    in[i] = local[i];
  /* maybe the user has set in[DOM_EVAL_FOR_SD]: don't erase it */
  return ((*PARAM_PATCH_BC (p))
          (PARAM_PATCH_BCD (p), PARAM_PATCH_BSD (p), in, value, type));
}

/* domain interface function: for description see domain.h */
INT
BNDP_SurfaceId (BNDP * aBndP, INT * n, INT i)
{
  BND_PS *ps;
  PATCH *p;
  DOUBLE global[DOM_N_IN_PARAMS];
  DOUBLE *local;

  if (i < 0)
    return (1);

  ps = (BND_PS *) aBndP;
  if (ps == NULL)
    return (-1);

  return ps->patch_id;
}

/* domain interface function: for description see domain.h */
INT NS_PREFIX
BNDP_Dispose (HEAP * Heap, BNDP * theBndP)
{
  BND_PS *ps;

  if (theBndP == NULL)
    return (0);

  IF_MARC (theBndP) return (M_BNDP_Dispose (Heap, theBndP));

  ps = (BND_PS *) theBndP;
  if (!PATCH_IS_FIXED (currBVP->patches[ps->patch_id]))
    if (PutFreelistMemory (Heap, BND_DATA (ps), DIM * sizeof (DOUBLE)))
      return (1);
  return (PutFreelistMemory (Heap, ps, BND_SIZE (ps)));
}

/* domain interface function: for description see domain.h */
INT NS_PREFIX
BNDS_Dispose (HEAP * Heap, BNDS * theBndS)
{
  BND_PS *ps;

  if (theBndS == NULL)
    return (0);

  IF_MARC (theBndS) return (M_BNDS_Dispose (Heap, theBndS));

  ps = (BND_PS *) theBndS;
  if (!PATCH_IS_FIXED (currBVP->patches[ps->patch_id]))
    if (PutFreelistMemory (Heap, BND_DATA (ps), BND_N (ps) * sizeof (BNDP *)))
      return (1);
  return (PutFreelistMemory (Heap, ps, BND_SIZE (ps)));
}

/* the following interface functions are not available in std_domain.c */
INT NS_PREFIX
BVP_Save (BVP * theBVP, char *name, char *mgname, HEAP * theHeap, INT argc,
          char **argv)
{
  UserWrite ("ERROR: std domain cannot be saved\n");
  return (1);
}

BVP *NS_PREFIX
BVP_Load (char *name, INT argc, char **argv)
{
  return (NULL);
}

/* domain interface function: for description see domain.h */
INT NS_PREFIX
BNDP_SaveBndP (BNDP * BndP)
{
  BND_PS *bp;
  INT i, j;
  int iList[2];
  double dList[DIM];

  IF_MARC (BndP) return (M_BNDP_SaveBndP (BndP));

  iList[0] = BND_PATCH_ID (BndP);
  iList[1] = BND_N (BndP);
  if (Bio_Write_mint (2, iList))
    return (1);

  bp = (BND_PS *) BndP;
  for (i = 0; i < BND_N (BndP); i++)
  {
    for (j = 0; j < DIM - 1; j++)
      dList[j] = bp->local[i][j];
    if (Bio_Write_mdouble (DIM - 1, dList))
      return (1);
  }
  if (!PATCH_IS_FIXED (currBVP->patches[BND_PATCH_ID (BndP)]))
  {
    DOUBLE *pos = (DOUBLE *) BND_DATA (bp);

    /* save free boundary point coordinates */
    for (j = 0; j < DIM; j++)
      dList[j] = pos[j];
    if (Bio_Write_mdouble (DIM, dList))
      return (1);
  }

  return (0);
}


#define IO_MARC


INT NS_PREFIX
BNDP_SaveBndP_Ext (BNDP * BndP)
{
  BND_PS *bp;
  INT i, j;
  int iList[2];
  double dList[DIM - 1];

#ifdef IO_MARC
  {
    M_BNDP *p = (M_BNDP *) BndP;

    iList[0] = p->patch_id;
    if (Bio_Write_mint (1, iList))
      return (1);

    for (j = 0; j < DIM; j++)
      dList[j] = p->pos[j];
    if (Bio_Write_mdouble (DIM, dList))
      return (1);
    return (0);
  }
#endif

  /* TODO: save free boundary points */
  iList[0] = BND_PATCH_ID (BndP);
  iList[1] = BND_N (BndP);
  if (Bio_Write_mint (2, iList))
    return (1);

  bp = (BND_PS *) BndP;
  for (i = 0; i < BND_N (BndP); i++)
  {
    for (j = 0; j < DIM - 1; j++)
      dList[j] = bp->local[i][j];
    if (Bio_Write_mdouble (DIM - 1, dList))
      return (1);
  }
  if (!PATCH_IS_FIXED (currBVP->patches[BND_PATCH_ID (BndP)]))
  {
    DOUBLE *pos = (DOUBLE *) BND_DATA (bp);

    /* save free boundary point coordinates */
    for (j = 0; j < DIM; j++)
      dList[j] = pos[j];
    if (Bio_Write_mdouble (DIM, dList))
      return (1);
  }

  return (0);
}

/* domain interface function: for description see domain.h */
BNDP *NS_PREFIX
BNDP_LoadBndP (BVP * theBVP, HEAP * Heap)
{
  BND_PS *bp;
  int i, j, pid, n;
  int iList[2];
  double dList[DIM];

  if (((STD_BVP *) theBVP)->type == BVP_MARC)
    return (M_BNDP_LoadBndP (theBVP, Heap));

  if (Bio_Read_mint (2, iList))
    return (NULL);
  pid = iList[0];
  n = iList[1];
  bp =
    (BND_PS *) GetFreelistMemory (Heap,
                                  (n - 1) * sizeof (COORD_BND_VECTOR) +
                                  sizeof (BND_PS));
  bp->n = n;
  bp->patch_id = pid;
  for (i = 0; i < n; i++)
  {
    if (Bio_Read_mdouble (DIM - 1, dList))
      return (NULL);
    for (j = 0; j < DIM - 1; j++)
      bp->local[i][j] = dList[j];
  }
  /* load free boundary points properly */
  if (!PATCH_IS_FIXED (currBVP->patches[pid]))
  {
    DOUBLE *pos;

    /* read global coordinates */
    BND_DATA (bp) = GetFreelistMemory (Heap, DIM * sizeof (DOUBLE));
    if (BND_DATA (bp) == NULL)
      return (NULL);

    if (Bio_Read_mdouble (DIM, dList))
      return (NULL);
    pos = (DOUBLE *) BND_DATA (bp);
    for (j = 0; j < DIM; j++)
      pos[j] = dList[j];
  }

  return ((BNDP *) bp);
}

BNDP *NS_PREFIX
BNDP_LoadBndP_Ext (void)
{
  BND_PS *bp;
  int i, j, pid, n;
  int iList[2];
  double dList[DIM - 1];

#ifdef IO_MARC
  {
    M_BNDP *p = (M_BNDP *) malloc (sizeof (M_BNDP));

    if (Bio_Read_mint (1, iList))
      return (NULL);
    p->patch_id = iList[0];
    if (Bio_Read_mdouble (DIM, dList))
      return (NULL);

    for (j = 0; j < DIM; j++)
      p->pos[j] = dList[j];

    return ((BNDP *) p);
  }
#endif

  if (Bio_Read_mint (2, iList))
    return (NULL);
  pid = iList[0];
  n = iList[1];
  bp =
    (BND_PS *) malloc ((n - 1) * sizeof (COORD_BND_VECTOR) + sizeof (BND_PS));
  bp->n = n;
  bp->patch_id = pid;
  for (i = 0; i < n; i++)
  {
    if (Bio_Read_mdouble (DIM - 1, dList))
      return (NULL);
    for (j = 0; j < DIM - 1; j++)
      bp->local[i][j] = dList[j];
  }

  return ((BNDP *) bp);
}

INT NS_PREFIX
ReadAndPrintArgvPosition (char *name, INT argc, char **argv, DOUBLE * pos)
{
  INT i;
  char option[OPTIONLEN];
  double x[DIM];

  for (i = 0; i < argc; i++)
    if (argv[i][0] == name[0])
    {
#ifdef __TWODIM__
      if (sscanf (argv[i], "%s %lf %lf", option, x, x + 1) != 3)
        continue;
#endif
#ifdef __THREEDIM__
      if (sscanf (argv[i], "%s %lf %lf %lf", option, x, x + 1, x + 2) != 4)
        continue;
#endif
      if (strcmp (option, name) == 0)
      {
        pos[0] = x[0];
        pos[1] = x[1];
#ifdef __TWODIM__
        UserWriteF ("set %s to (%lf,%lf)\n", name, x[0], x[1]);
#endif
#ifdef __THREEDIM__
        pos[2] = x[2];
        UserWriteF ("set %s to (%lf,%lf,%lf)\n", name, x[0], x[1], x[2]);
#endif
        return (0);
      }
    }

  return (1);
}

/****************************************************************************/
/** \brief Create and initialize the std_domain
 *
 * This function creates the environments domain, problem and BVP.
 *
 * @return <ul>
 *   <li>   0 if ok
 *   <li>   1 when error occured.
 * </ul>
 */
/****************************************************************************/

INT NS_PREFIX
InitDom (void)
{

  /* change to root directory */
  if (ChangeEnvDir ("/") == NULL)
  {
    PrintErrorMessage ('F', "InitDom", "could not changedir to root");
    return (__LINE__);
  }

  /* get env dir/var IDs for the problems */
  theProblemDirID = GetNewEnvDirID ();
  theBdryCondVarID = GetNewEnvVarID ();

  /* install the /Domains directory */
  theDomainDirID = GetNewEnvDirID ();
  if (MakeEnvItem ("Domains", theProblemDirID, sizeof (ENVDIR)) == NULL)
  {
    PrintErrorMessage ('F', "InitDom", "could not install '/Domains' dir");
    return (__LINE__);
  }
  theBdrySegVarID = GetNewEnvVarID ();
  theLinSegVarID = GetNewEnvVarID ();

  /* install the /BVP directory */
  theBVPDirID = GetNewEnvDirID ();
  if (MakeEnvItem ("BVP", theBVPDirID, sizeof (ENVDIR)) == NULL)
  {
    PrintErrorMessage ('F', "InitDom", "could not install '/BVP' dir");
    return (__LINE__);
  }

  return (0);
}

/** @} */
