// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  evm.c                                                                                                                 */
/*																			*/
/* Purpose:   elementary vector manipulations								*/
/*																			*/
/* Author:	  Klaus Johannsen												*/
/*			  Institut fuer Computeranwendungen                                                     */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  internet: ug@ica3.uni-stuttgart.de						*/
/*																			*/
/* History:   8.12.94 begin, ug3-version									*/
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

#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>

#include "compiler.h"
#include "misc.h"
#include "evm.h"

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

/* data for CVS */
static char rcsid[] = "$Header$";

/****************************************************************************/
/*																			*/
/* forward declarations of macros											*/
/*																			*/
/****************************************************************************/

#define MIN_DETERMINANT                                 0.0001*SMALL_C

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****																	 ****/
/****		general routines											 ****/
/****																	 ****/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/*D
   ClipRectangleAgainstRectangle - Clip a rectangle against a rectangle

   SYNOPSIS:
   INT ClipRectangleAgainstRectangle (const COORD *r1min, const COORD *r1max,
   COORD *r2min, COORD *r2max);

   PARAMETERS:
   .  r1min - lower left corner of rectangle 1
   .  r1max - upper right corner of rectangle 1
   .  r2min - lower left corner of rectangle 2
   .  r2max - upper right corner of rectangle 2

   DESCRIPTION:
   This function clips the rectangle given by r2 against the rectangle r1,
   i.e. r2 is modified such that it is completely inside of r1.


   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if window is collapsed.
   D*/
/****************************************************************************/

INT ClipRectangleAgainstRectangle (const COORD *r1min, const COORD *r1max, COORD *r2min, COORD *r2max)
{
  if (r2min[0] < r1min[0]) r2min[0] = r1min[0];
  if (r2min[1] < r1min[1]) r2min[1] = r1min[1];
  if (r2max[0] > r1max[0]) r2max[0] = r1max[0];
  if (r2max[1] > r1max[1]) r2max[1] = r1max[1];

  if (r2min[0] >= r2max[0] || r2min[1] >= r2max[1])
    return(1);
  return(0);
}

/****************************************************************************/
/*D
   CheckRectagleIntersection - Check if two rectangles intersect

   SYNOPSIS:
   INT CheckRectagleIntersection (const COORD *r1min, const COORD *r1max,
   const COORD *r2min, const COORD *r2max);

   PARAMETERS:
   .  r1min - lower left corner of rectangle 1
   .  r1max - upper right corner of rectangle 1
   .  r2min - lower left corner of rectangle 2
   .  r2max - upper right corner of rectangle 2

   DESCRIPTION:
   This function clips a rectangle against a rectangle.

   RETURN VALUE:
   INT
   .n    0 if no intersection
   .n    1 if intersection.
   D*/
/****************************************************************************/

INT CheckRectagleIntersection (const COORD *r1min, const COORD *r1max, const COORD *r2min, const COORD *r2max)
{
  if (r1max[0] < r2min[0]) return(0);
  if (r2max[0] < r1min[0]) return(0);
  if (r1max[1] < r2min[1]) return(0);
  if (r2max[1] < r1min[1]) return(0);

  return(1);
}

/****************************************************************************/
/*D
   CheckRectangle - Check if rectangle has a minimum size

   SYNOPSIS:
   INT CheckRectangle (const COORD *rmin, const COORD *rmax,
   const COORD minsize);

   PARAMETERS:
   .  rmin - lower left corner
   .  rmax - upper right corner
   .  minsize - minimal size of rect in x and y direction

   DESCRIPTION:
   This function checks if a rectangle has at least size 'minsize' in `x` and
   `y`-direction.

   RETURN VALUE:
   INT
   .n     0 if ok
   .n     1 if rectangle is smaller than 'minsize' in some direction
   D*/
/****************************************************************************/

INT CheckRectangle (const COORD *rmin, const COORD *rmax, const COORD minsize)
{
  if (rmax[0] <= rmin[0]+minsize) return(1);
  if (rmax[1] <= rmin[1]+minsize) return(1);
  return(0);
}

/****************************************************************************/
/*D
   PointInTriangle - Decide if Point lies in the triangle of Points

   SYNOPSIS:
   INT PointInTriangle (const COORD_POINT *Points,
   const COORD_POINT Point);

   PARAMETERS:
   .  Points - Array of three 'COORD_POINT' structures
   .  Point - Point to check

   STRUCTURES:

   .vb
   struct coord_point
   {
    COORD x;
    COORD y;
   };

   typedef struct coord_point COORD_POINT;
   .ve

   DESCRIPTION:
   This function decides if 'Point' lies in the triangle given by 'Points'.

   RETURN VALUE:
   INT
   .n   0 if it lies not in the triangle
   .n   1 if it lies in the triangle.
   D*/
/****************************************************************************/

INT PointInTriangle (const COORD_POINT *Points, const COORD_POINT Point)
{
  COORD M[9], Inverse[9], rhs[3], lambda[3];

  /* invert a 3x3 system */
  M[0]=Points[0].x, M[3]=Points[1].x, M[6] =Points[2].x;
  M[1]=Points[0].y, M[4]=Points[1].y, M[7] =Points[2].y;
  M[2]=1.0,                 M[5]=1.0,             M[8] =1.0;

  if (M3_Invert(Inverse, M)) return (0);
  rhs[0] = Point.x;
  rhs[1] = Point.y;
  rhs[2] = 1.0;
  M3_TIMES_M3(Inverse,rhs,lambda);

  /* decide if Point lies in the interior of Points */
  if (lambda[0]>=0.0 && lambda[1]>=0.0 && lambda[2]>=0.0)
    return (1);
  return (0);
}

/****************************************************************************/
/*D
   PointInPolygon - Decide if Point lies in the polygon of Points

   SYNOPSIS:
   INT PointInPolygon (const COORD_POINT *Points, INT n,
   const COORD_POINT Point);

   PARAMETERS:
   .  Points - polygon given by array of 'COORD_POINT' structures
   .  n - number of corners
   .  Point - Point in question

   STRUCTURES:

   .vb
   struct coord_point
   {
    COORD x;
    COORD y;
   };
   .ve

   DESCRIPTION:
   This function decides if 'Point' lies in the polygon of 'Points'.

   The number of corners of the polygon must be less than or equal
   than 4 in the current implementation!

   RETURN VALUE:
   INT
   .n     0 when lies not in the polygon
   .n     1 when lies in the polygon.
   D*/
/****************************************************************************/

#define POLYMAX         8

INT PointInPolygon (const COORD_POINT *Points, INT n, COORD_POINT Point)
{
  COORD D[POLYMAX] ,tau[POLYMAX],xa,ya,xe,ye;
  int i, left, right;

  assert (n<=POLYMAX);
  if (n<=2) return (0);

  xa = Points[0].x;
  ya = Points[0].y;
  for (i=1; i<=n; i++)
  {
    xe = Points[i%n].x;
    ye = Points[i%n].y;
    D[i-1] = (xe-xa)*(xe-xa)+(ye-ya)*(ye-ya);
    tau[i-1] = (-(ye-ya)*(Point.x-xa)+(xe-xa)*(Point.y-ya));
    xa = xe;
    ya = ye;
  }
  left = right = 0;
  for (i=0; i<n; i++)
  {
    if (tau[i]>=0.0) left++;
    if (tau[i]<=0.0) right++;
    /*	if (tau[i]>=D[i]*SMALL_C) left++;
            if (tau[i]<=-D[i]*SMALL_C) right++;		*/
  }
  if (left==n || right==n)
    return(1);
  return(0);
}

/****************************************************************************/
/*																			*/
/* Function:  PointInPolygonC												*/
/*																			*/
/* Purpose:   decide if Point lies in the polygon of Points with COORD-desc	*/
/*																			*/
/* input:	  const COORD_VECTOR_2D *Points: polygon						*/
/*			  INT n: number of corners										*/
/*			  const COORD_VECTOR_2D Point									*/
/*																			*/
/* return:	  INT 0: lies not in the polygon								*/
/*				  1: lies in the polygon									*/
/*																			*/
/****************************************************************************/

INT PointInPolygonC (const COORD_VECTOR_2D *Points, INT n, const COORD_VECTOR_2D Point)
{
  COORD tau[POLYMAX],xa,ya,xe,ye;
  int i, left, right;

  assert (n<=POLYMAX);
  if (n<=2) return (0);

  xa = Points[0][0];
  ya = Points[0][1];
  for (i=1; i<=n; i++)
  {
    xe = Points[i%n][0];
    ye = Points[i%n][1];
    tau[i-1] = (-(ye-ya)*(Point[0]-xa)+(xe-xa)*(Point[1]-ya));
    xa = xe;
    ya = ye;
  }
  left = right = 0;
  for (i=0; i<n; i++)
  {
    if (tau[i]>=0.0) left++;
    if (tau[i]<=0.0) right++;
  }
  if (left==n || right==n)
    return(1);
  return(0);
}

/****************************************************************************/
/*																			*/
/* Function:  PolyArea														*/
/*																			*/
/* Purpose:   determine area of polygon								                */
/*																			*/
/* input:	  INT n: nb of corners of polygon								*/
/*			  COORD_VECTOR_2D *Polygon: polygon								*/
/*																			*/
/* output:	  COORD *Area: area												*/
/*																			*/
/* return:	  INT 0: ok														*/
/*				  1: error													*/
/*																			*/
/****************************************************************************/

INT PolyArea (INT n, COORD_VECTOR_2D *Polygon, COORD *Area)
{
  INT i;
  COORD c;
  COORD_VECTOR_2D a, b;


  *Area = 0.0;
  if (n<3) return (0);
  for (i=1; i<n-1; i++)
  {
    V2_SUBTRACT(Polygon[i],Polygon[0],a)
    V2_SUBTRACT(Polygon[i+1],Polygon[0],b)
    V2_VECTOR_PRODUCT(a,b,c)
      (*Area) += ABS(c);
  }
  (*Area) *= 0.5;

  return (0);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****																	 ****/
/****		2D routines                                                                                              ****/
/****																	 ****/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/*D
   M2_Invert - Calculate inverse of a 2x2 COORD matrix

   SYNOPSIS:
   INT M2_Invert (COORD *Inverse, const COORD *Matrix);

   PARAMETERS:
   .  Inverse - inverse of matrix
   .  Matrix - matrix

   DESCRIPTION:
   This function  calculates inverse of a 2x2 COORD matrix.
   The entries of the matrices are given in a linear array with the
   following order -

   .vb
 | Matrix[0] Matrix[1] |
 | Matrix[2] Matrix[3] |
   .ve

   RETURN VALUE:
   INT
   .n   0 if ok
   .n   1 if matrix is nearly singular.
   D*/
/****************************************************************************/

INT M2_Invert (COORD *Inverse, const COORD *Matrix)
{
  COORD det;

  det = Matrix[0]*Matrix[3]-Matrix[1]*Matrix[2];
  if (ABS(det)<SMALL_C) return (1);
  Inverse[0] = Matrix[3]/det;
  Inverse[1] = -Matrix[1]/det;
  Inverse[2] = -Matrix[2]/det;
  Inverse[3] = Matrix[0]/det;

  return (0);
}

/****************************************************************************/
/*D
   M3_Invert - Calculate inverse of a 3x3 COORD matrix

   SYNOPSIS:
   INT M3_Invert (COORD *Inverse, const COORD *Matrix);

   PARAMETERS:
   .  Inverse - inverse of matrix
   .  Matrix - matrix

   DESCRIPTION:
   This function calculates inverse of a 3x3 COORD matrix.
   The entries of the matrices are given in a linear array with the
   following order -

   .vb
 | Matrix[0] Matrix[1] Matrix[2]|
 | Matrix[3] Matrix[4] Matrix[5]|
 | Matrix[6] Matrix[7] Matrix[8]|
   .ve

   RETURN VALUE:
   INT
   .n    0 when ok
   .n    1 when matrix is nearly singular.
   D*/
/****************************************************************************/

INT M3_Invert (COORD *Inverse, const COORD *Matrix)
{
  COORD determinant,invdet;
  INT i,i1,i2, j,j1,j2;

  for (i=0; i<3; i++)
  {
    i1 = (i+1)%3;
    i2 = (i+2)%3;
    for ( j=0; j<3; j++)
    {
      j1 = (j+1)%3;
      j2 = (j+2)%3;
      Inverse[j+3*i] = Matrix[i1+3*j1]*Matrix[i2+3*j2] - Matrix[i1+3*j2]*Matrix[i2+3*j1];
    }
  }
  determinant = Inverse[0+3*0]*Matrix[0+3*0] + Inverse[0+3*1]*Matrix[1+3*0] + Inverse[0+3*2]*Matrix[2+3*0];

  /* check the determinant */
  if (fabs(determinant) > MIN_DETERMINANT)
  {
    invdet = 1.0/determinant;
    for (i=0; i<3; i++)
      for (j=0; j<3; j++)
        Inverse[i+3*j] *= invdet;
    return(0);
  }

  return(1);
}

/****************************************************************************/
/*D
   V2_Normalize	- Normalize a 2D vector

   SYNOPSIS:
   INT V2_Normalize (COORD *a);

   PARAMETERS:
   .  a - input 2D vector (a[0],a[1])

   DESCRIPTION:
   This function normalizes the 2D vector a.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured
   .n    2 if vector is nearly 0.
   D*/
/****************************************************************************/

INT V2_Normalize (COORD *a)
{
  COORD norm;

  V2_EUKLIDNORM(a,norm);
  if (norm < SMALL_C) return(2);
  V2_SCALE(1.0/norm,a);

  return(0);
}

/****************************************************************************/
/*D
   V2_Rotate - Rotate vector by angle

   SYNOPSIS:
   INT V2_Rotate (COORD *vector, COORD alpha);

   PARAMETERS:
   .  vector - 2D vector
   .  alpha - angle in radiant

   DESCRIPTION:
   This function rotates vector 'vector' by angle 'alpha'.
   The vector is turned in mathmatical pos. sense.

   RETURN VALUE:
   INT
   .n      0
   D*/
/****************************************************************************/

INT V2_Rotate (COORD *vector, COORD alpha)
{
  COORD help[2];
  COORD calpha, salpha;

  /* rotate vector */
  help[0] = -vector[1]; help[1] = vector[0];
  calpha = (COORD)cos((double)alpha);
  salpha = (COORD)sin((double)alpha);
  V2_LINCOMB(salpha,help,calpha,vector,vector);

  return (0);
}

/****************************************************************************/
/*D
   vp - Return positive number if vector 2 is "left" of vector 1

   SYNOPSIS:
   DOUBLE vp (const DOUBLE x1, const DOUBLE y1, const DOUBLE x2, const DOUBLE y2);

   PARAMETERS:
   .  x1,y1 - coordinates of a 2D vector
   .  x2,y2 - coordinates of a 2D vector

   DESCRIPTION:
   This function returns positive number if vector 2 is "left" of vector 1, i.e.
   the third component of the vector product of (x1,y1,0) and (x2,y2,0).

   RETURN VALUE:
   DOUBLE
   D*/
/****************************************************************************/

DOUBLE vp (const DOUBLE x1, const DOUBLE y1, const DOUBLE x2, const DOUBLE y2)
{
  DOUBLE l1,l2;

  l1 = sqrt(x1*x1+y1*y1);
  l2 = sqrt(x2*x2+y2*y2);
  if ((l1<SMALL_D)||(l2<SMALL_D))
    return(0.0);
  else
    return((x1*y2-y1*x2)/(l1*l2));
}

/****************************************************************************/
/*D
   tarea - Compute area of a triangle

   SYNOPSIS:
   DOUBLE tarea (DOUBLE x0,DOUBLE y0,DOUBLE x1,DOUBLE y1,DOUBLE x2,DOUBLE y2);

   PARAMETERS:
   .  x0,y0 - coordinates of first point
   .  x1,y1 - coordinates of second point
   .  x2,y2 - coordinates of third point

   DESCRIPTION:
   This function computes the area of a triangle.

   RETURN VALUE:
   DOUBLE  area
   D*/
/****************************************************************************/

DOUBLE tarea (DOUBLE x0,DOUBLE y0,DOUBLE x1,DOUBLE y1,DOUBLE x2,DOUBLE y2)
{
  return(0.5*fabs((y1-y0)*(x2-x0)-(x1-x0)*(y2-y0)));
}
/****************************************************************************/
/*D
   qarea - Compute area of a convex quadrilateral

   SYNOPSIS:
   DOUBLE qarea (DOUBLE x0,DOUBLE y0,DOUBLE x1,DOUBLE y1,DOUBLE x2,DOUBLE y2,
   DOUBLE x3,DOUBLE y3);

   PARAMETERS:
   .  x0,y0 - coordinates of first point
   .  x1,y1 - coordinates of second point
   .  x2,y2 - coordinates of third point
   .  x3,y3 - coordinates of fourth point

   DESCRIPTION:
   This function computes the area of a convex quadrilateral.

   RETURN VALUE:
   DOUBLE area
   D*/
/****************************************************************************/
DOUBLE qarea (DOUBLE x0,DOUBLE y0,DOUBLE x1,DOUBLE y1,DOUBLE x2,DOUBLE y2,DOUBLE x3,DOUBLE y3)
{
  return( 0.5*fabs( (y3-y1)*(x2-x0)-(x3-x1)*(y2-y0) ) );
}
/****************************************************************************/
/*D
   c_tarea - Compute area of triangle

   SYNOPSIS:
   DOUBLE c_tarea (const COORD *x0, const COORD *x1, const COORD *x2)

   PARAMETERS:
   .  x0 - Array with coordinates of first point
   .  x1 - Array with coordinates of second point
   .  x2 - Array with coordinates of third point

   DESCRIPTION:
   This function computes area of a triangle.

   RETURN VALUE:
   DOUBLE area
   D*/
/****************************************************************************/
DOUBLE c_tarea (const COORD *x0, const COORD *x1, const COORD *x2)
{
  return(0.5*fabs((x1[_Y_]-x0[_Y_])*(x2[_X_]-x0[_X_])-(x1[_X_]-x0[_X_])*(x2[_Y_]-x0[_Y_])));
}
/****************************************************************************/
/*D
   c_qarea - Compute area of a convex quadrilateral

   SYNOPSIS:
   DOUBLE c_qarea (const COORD *x0, const COORD *x1, const COORD *x2, const COORD *x3);

   PARAMETERS:
   .  x0 - Array with coordinates of first point
   .  x1 - Array with coordinates of second point
   .  x2 - Array with coordinates of third point
   .  x3 - Array with coordinates of fourth point

   DESCRIPTION:
   This function computes area of a convex quadrilateral.

   RETURN VALUE:
   DOUBLE area
   D*/
/****************************************************************************/
DOUBLE c_qarea (const COORD *x0, const COORD *x1, const COORD *x2, const COORD *x3)
{
  return( 0.5*fabs( (x3[_Y_]-x1[_Y_])*(x2[_X_]-x0[_X_])-(x3[_X_]-x1[_X_])*(x2[_Y_]-x0[_Y_]) ) );
}

/****************************************************************************/
/*D
   ctarea - Compute area of element wrt cylinder metric

   SYNOPSIS:
   DOUBLE ctarea (DOUBLE x0,DOUBLE y0,DOUBLE x1,DOUBLE y1,DOUBLE x2,DOUBLE y2)

   PARAMETERS:
   .  x0,y0 - coordinates of first point
   .  x1,y1 - coordinates of second point
   .  x2,y2 - coordinates of third point

   DESCRIPTION:
   This function computes area of element wrt cylinder metric.

   RETURN VALUE:
   DOUBLE
   D*/
/****************************************************************************/

DOUBLE ctarea (DOUBLE x0,DOUBLE y0,DOUBLE x1,DOUBLE y1,DOUBLE x2,DOUBLE y2)
{
  return((y0+y1+y2) * fabs((y1-y0)*(x2-x0)-(x1-x0)*(y2-y0)) / 6);
}
/****************************************************************************/
/*D
   cqarea - Compute area of element wrt cylinder metric

   SYNOPSIS:
   DOUBLE cqarea (DOUBLE x0,DOUBLE y0,DOUBLE x1,DOUBLE y1,DOUBLE x2,DOUBLE y2,
   DOUBLE x3,DOUBLE y3);

   PARAMETERS:
   .  x0,y0 - coordinates of first point
   .  x1,y1 - coordinates of second point
   .  x2,y2 - coordinates of third point
   .  x3,y3 - coordinates of fourth point

   DESCRIPTION:
   This function computes area of element wrt cylinder metric.

   RETURN VALUE:
   DOUBLE
   D*/
/****************************************************************************/
DOUBLE cqarea (DOUBLE x0,DOUBLE y0,DOUBLE x1,DOUBLE y1,DOUBLE x2,DOUBLE y2,DOUBLE x3,DOUBLE y3)
{
  return(
           ((y0+y1+y2) * fabs((y1-y0)*(x2-x0)-(x1-x0)*(y2-y0)) +
            (y0+y2+y3) * fabs((y2-y0)*(x3-x0)-(x2-x0)*(y3-y0))) / 6
           );
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****																	 ****/
/****		3D routines                                                                                              ****/
/****																	 ****/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/*D
   V3_Normalize	- Normalize vector a 3D vector

   SYNOPSIS:
   INT V3_Normalize (COORD *a);

   PARAMETERS:
   .  a - 3D vector

   DESCRIPTION:
   This function normalizes vector a.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    2 if vector is nearly 0.
   D*/
/****************************************************************************/

INT V3_Normalize (COORD *a)
{
  COORD norm;

  V3_EUKLIDNORM(a,norm);
  if (norm < SMALL_C) return(2);
  V3_SCALE(1.0/norm,a);

  return(0);
}

/****************************************************************************/
/*D
   V3_NormVectorProduct - Calculate norm of vector product  a x b

   SYNOPSIS:
   INT V3_NormVectorProduct (const COORD *a, const COORD *b, COORD *result);

   PARAMETERS:
   .  a - input vector (a[0],a[1],a[2])
   .  b - input vector (b[0],b[1],b[2])
   .  result - output scalar result[0]

   DESCRIPTION:
   This function calculates norm of vector product a x b.

   RETURN VALUE:
   INT
   .n     0 if ok
   .n     1 if error occured.
   D*/
/****************************************************************************/

INT V3_NormVectorProduct (const COORD *a, const COORD *b, COORD *result)
{
  COORD VectorPrd[3];

  V3_VECTOR_PRODUCT(a,b,VectorPrd);
  V3_EUKLIDNORM(VectorPrd,*result);

  return(0);
}

/****************************************************************************/
/*D
   V3_Rotate - Rotate vector around axis by a given angle

   SYNOPSIS:
   INT V3_Rotate (COORD *vector, const COORD *axis, COORD alpha);

   PARAMETERS:
   .  vector - vector to rotate
   .  axis - axis around whoch to rotate
   .  alpha - angle in radiant

   DESCRIPTION:
   This function rotates a given vector around an axis by angle (looking from the
   top). The vector is turned in mathmatical pos. sense.

   RETURN VALUE:
   INT
   .n    0 if o.k.
   .n    1 if axis is nearly zero.
   D*/
/****************************************************************************/

INT V3_Rotate (COORD *vector, const COORD *axis, COORD alpha)
{
  COORD RotationAxis[3], help[3];
  COORD scalarprd, calpha, salpha;

  /* normalize axis */
  V3_COPY(axis,RotationAxis);
  if (V3_Normalize(RotationAxis)) return(1);

  /* rotate vector */
  calpha = (COORD)cos((double)alpha);
  salpha = (COORD)sin((double)alpha);
  V3_SCALAR_PRODUCT(RotationAxis,vector,scalarprd);
  V3_VECTOR_PRODUCT(RotationAxis,vector,help);
  V3_LINCOMB(calpha,vector,salpha,help,help);
  V3_LINCOMB(1.0,help,(1.0-calpha)*scalarprd,RotationAxis,vector);

  return (0);
}

/****************************************************************************/
/*D
   V3_Angle - Calculate angle between two vectors

   SYNOPSIS:
   INT V3_Angle (const COORD *a, const COORD *b, COORD *result);

   PARAMETERS:
   .  a - first vector
   .  b - second vector
   .  result - places result here

   DESCRIPTION:
   This function calculates angle between two vectors.

   RETURN VALUE:
   INT
   .n    0 if o.k.
   .n    1 if error occured.
   D*/
/****************************************************************************/

INT V3_Angle (const COORD *a, const COORD *b, COORD *result)
{
  COORD c, sc, n1, n2;

  V3_EUKLIDNORM(a,n1)
  V3_EUKLIDNORM(b,n2)
  c = n1*n2;
  if (ABS(c)<SMALL_C)
  {
    *result = 0.0;
    return (1);
  }
  V3_SCALAR_PRODUCT(a,b,sc)
  c = sc/c;
  if (c>=1.0)
    *result = 0.0;
  else if (c<=-1.0)
    *result = PI;
  else
    *result = (COORD)acos((double)c);

  return (0);
}

/****************************************************************************/
/*D
   V3_Orthogonalize - Orthgonalize a vector w.r.t. to another vector.

   SYNOPSIS:
   INT V3_Orthogonalize (const COORD *a, const COORD *b, COORD *r);

   PARAMETERS:
   .  a - vector to orthogonalize
   .  b - vector where 'a' is orthogonalized to
   .  r - resulting vector

   DESCRIPTION:
   This function orthgonalizes vector 'a' w.r.t. to 'b'.

   RETURN VALUE:
   INT
   .n    0 if o.k.
   .n    1 if error occured.
   D*/
/****************************************************************************/

INT V3_Orthogonalize (const COORD *a, const COORD *b, COORD *r)
{
  COORD normb, scprd;

  V3_EUKLIDNORM(b,normb)
  if (normb < SMALL_C)
    V3_COPY(a,r)
    else
    {
      V3_SCALAR_PRODUCT(a,b,scprd)
      V3_LINCOMB(1.0,a,-scprd/normb/normb,b,r)
    }

  return (0);
}

/****************************************************************************/
/*D
   M4_Invert - Invert a 4X4 Matrix

   SYNOPSIS:
   INT M4_Invert (COORD *Inverse, const COORD *Matrix);

   PARAMETERS:
   .  Inverse - output of inverted matrix
   .  Matrix - input matrix to be inverted

   DESCRIPTION:
   This function inverts a 4X4 Matrix.
   The entries of the matrices are given in a linear array with the
   following order -

   .vb
 | Matrix[0]  Matrix[1]  Matrix[2]  Matrix[3]|
 | Matrix[4]  Matrix[5]  Matrix[6]  Matrix[7]|
 | Matrix[8]  Matrix[9]  Matrix[10] Matrix[11]|
 | Matrix[12] Matrix[13] Matrix[14] Matrix[15]|
   .ve

   RETURN VALUE:
   INT
   .n  0 when ok
   .n  1 when matrix is singular or an error occurred.
   D*/
/****************************************************************************/

INT M4_Invert (COORD *Inverse, const COORD *Matrix)
{
  COORD d,dinv;
  INT i,i1,i2,i3, j,j1,j2,j3,sign;

  sign = 0;                     /* no matter which value!!! */

  /* determine submatrices */
  for ( i=0; i<4; i++ )
  {
    i1 = (i+1) & 3;
    i2 = (i1+1) & 3;
    i3 = (i2+1) & 3;
    for ( j=0; j<4; j++ )
    {
      j1 = (j+1) & 3;
      j2 = (j1+1) & 3;
      j3 = (j2+1) & 3;
      Inverse[j+4*i] =   Matrix[i1+4*j1] * (  Matrix[i2+4*j2] * Matrix[i3+4*j3]
                                              - Matrix[i2+4*j3] * Matrix[i3+4*j2] )
                       + Matrix[i1+4*j2] * (  Matrix[i2+4*j3] * Matrix[i3+4*j1]
                                              - Matrix[i2+4*j1] * Matrix[i3+4*j3] )
                       + Matrix[i1+4*j3] * (  Matrix[i2+4*j1] * Matrix[i3+4*j2]
                                              - Matrix[i2+4*j2] * Matrix[i3+4*j1] );

      if (sign) Inverse[j+4*i] *= -1.0;
      sign = !sign;
    }
    sign = !sign;
  }

  /* determine determinant */
  d = Inverse[0+4*0] * Matrix[0+4*0] + Inverse[0+4*1] * Matrix[1+4*0]
      + Inverse[0+4*2] * Matrix[2+4*0] + Inverse[0+4*3] * Matrix[3+4*0];

  /* check determinant and determine inverse */
  if (ABS(d) > MIN_DETERMINANT)
  {
    dinv = 1.0/d;
    for ( i=0; i<4; i++ )
      for (j=0; j<4; j++ ) Inverse[i+4*j] *= dinv;
    return(0);
  }
  return(1);
}
