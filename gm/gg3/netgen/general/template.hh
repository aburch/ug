// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#ifndef FILE_TEMPLATE
#define FILE_TEMPLATE

/**************************************************************************/
/* File:   template.hh                                                    */
/* Author: Joachim Schoeberl                                              */
/* Date:   01. Jun. 95                                                    */
/**************************************************************************/

/*
   templates, global types, defines and variables
 */


class ostream;
extern ostream * testout;
extern void MyError (char * ch);

// #define MYGRAPH
// #define SOLIDGEOM


typedef int INDEX;
typedef int BOOL;

class twoint {
public: int i1, i2; twoint() {};
  twoint(int ii1, int ii2) {i1 = ii1; i2 = ii2;}
};
class threeint { public: int i1, i2, i3; threeint() {}; };
class fourint { public: int i1, i2, i3, i4; fourint() {}; };


template <class T>
inline T ng_min (T a, T b)
{
  return (a < b) ? a : b;
}
template <class T>
inline T ng_max (T a, T b)
{
  return (a > b) ? a : b;
}
template <class T>
inline T ng_min (T a, T b, T c)
{
  return (a < b) ? (a < c) ? a : c
         : (b < c) ? b : c;
}
template <class T>
inline T ng_max (T a, T b, T c)
{
  return (a > b) ? ((a > c) ? a : c)
         : ((b > c) ? b : c);
}

template <class T>
inline void ng_swap (T & a, T & b)
{
  T temp = a;
  a = b;
  b = temp;
}

template <class T>
inline int sgn (T a)
{
  return (a > 0) ? 1 : (   ( a < 0) ? 0 : -1 );
}

template <class T>
inline T sqr (T a)
{
  return a * a;
}

#endif
