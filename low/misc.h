// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*	                                                                        */
/* File:      misc.h                                                        */
/*                                                                            */
/* Purpose:   header for misc.c                                             */
/*                                                                            */
/* Author:      Henrik Rentz-Reichert                                         */
/*              Institut fuer Computeranwendungen                             */
/*              Universitaet Stuttgart                                        */
/*              Pfaffenwaldring 27                                            */
/*              70569 Stuttgart                                                */
/*              internet: ug@ica3.uni-stuttgart.de                        */
/*                                                                            */
/* History:   23.02.95 ug3-version                                            */
/*                                                                            */
/* Revision:  07.09.95                                                      */
/*                                                                            */
/****************************************************************************/

/****************************************************************************/
/*                                                                            */
/* auto include mechanism and other include files                            */
/*                                                                            */
/****************************************************************************/

#ifndef __MISC__
#define __MISC__


#ifndef __COMPILER__
#include "compiler.h"
#endif

/****************************************************************************/
/*                                                                            */
/* defines in the following order                                            */
/*                                                                            */
/*          compile time constants defining static data size (i.e. arrays)    */
/*          other constants                                                    */
/*          macros                                                            */
/*                                                                            */
/****************************************************************************/

#define PI                       3.141592653589793238462643383279

#define ABS(i)                   (((i)<0) ? (-(i)) : (i))
#define MIN(x,y)                 (((x)<(y)) ? (x) : (y))
#define MAX(x,y)                 (((x)>(y)) ? (x) : (y))
#define POW2(i)                  (1<<(i))
#define ABSDIFF(a,b)             (fabs((a)-(b)))
#define SIGNUM(x)                (((x)>0) ? 1 : ((x)<0) ? -1 : 0)
#define EVEN(i)                  (!(i%2))
#define ODD(i)                   ((i%2))

#define HiWrd(aLong)             (((aLong) >> 16) & 0xFFFF)
#define LoWrd(aLong)             ((aLong) & 0xFFFF)

#define SetHiWrd(aLong,n)        aLong = (((n)&0xFFFF)<<16)|((aLong)&0xFFFF)
#define SetLoWrd(aLong,n)        aLong = ((n)&0xFFFF)|((aLong)&0xFFFF0000)

/* concatenation macros with one level of indirection to allow argument expansion */
#define CONCAT3(a,b,c)            CONCAT3_AUX(a,b,c)
#define CONCAT3_AUX(a,b,c)        a ## b ## c
#define CONCAT4(a,b,c,d)          CONCAT4_AUX(a,b,c,d)
#define CONCAT4_AUX(a,b,c,d)      a ## b ## c ## d
#define CONCAT5(a,b,c,d,e)        CONCAT5_AUX(a,b,c,d,e)
#define CONCAT5_AUX(a,b,c,d,e)    a ## b ## c ## d ## e

#define YES             1
#define ON              1

#define NO              0
#define OFF             0

#ifndef TRUE
    #define TRUE        1
    #define FALSE       0
#endif

/****************************************************************************/
/*																			*/
/* definition of exported global variables									*/
/*																			*/
/****************************************************************************/

#ifndef COMPILE_MISC_H
extern int UG_math_error;
#endif

/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/

/* general routines */
char       *StrTok              (char *s, const char *ct);
char       *expandfmt           (const char *fmt);
const char *strntok             (const char *str, const char *sep, int n, char *token);
void        QSort               (void *base, INT n, INT size, int (*cmp)(const void *, const void *));
void        SelectionSort       (void *base, INT n, INT size, int (*cmp)(const void *, const void *));
#ifdef __MWCW__
/* this function is no external for the MetroWerks CodeWarrior: so just define it */
int                     matherr                         (struct exception *x);
#endif

#endif
