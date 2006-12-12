// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      compiler.h                                                    */
/*                                                                          */
/* Purpose:   define simple data types and standard include files           */
/*                                                                          */
/* Author:    Peter Bastian, Klaus Birken, Stefan Lang                      */
/*            Institut fuer Computeranwendungen 3 ( ICA3 )                  */
/*            Pfaffenwaldring 27                                            */
/*            Universitaet Stuttgart                                        */
/*            70569 Stuttgart                                               */
/*                                                                          */
/* History:   29.01.92 begin, ug version 2.0                                */
/*            19.08.92 begin PARIX version                                  */
/*            02.06.93 begin PARAGON version                                */
/*            02.12.94 begin CRAY_T3D version                               */
/*            05.10.95 added GC (Explorer PowerPC) version                  */
/*                                                                          */
/****************************************************************************/

/* RCS_ID
   $Header$
 */

#ifndef __COMPILER__
#define __COMPILER__

/* try to import stuff found by autoconf. This is the first time I can
   use that C-compilers don't fail compilation if a header doesn't
   exist... */
#include "config.h"

/* Include the custom type declarations.  This should eventually be included
   directly by the headers who need it and not via compiler.h */
#include "ugtypes.h"

#ifdef AUTOTOOLS_BUILD
/* include all headers providing the compiler.h functionality */
  #include "architecture.h"
  #include "ugmemory.h"
  #include "ugstrings.h"
  #include "ugtime.h"
#else

#include <limits.h>
#include <float.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
/*                                                                          */
/* predefines, may be overridden below                                      */
/*                                                                          */
/****************************************************************************/

/* SMALL..: least number s.t. 1 + SMALL../SMALL_FAC != 1 */
#define SMALL_FAC            10

/* current time as DOUBLE value
   CURRENT_TIME should be the most accurate time (usually in micro seconds)
   CURRENT_TIME_LONG should be a time which measures some days without overflow
 */
/* define LONG_TIMER for measuring intervals above 30 min,
   resolution in seconds
 #define ARCH_LONG_TIMER */
#define CURRENT_TIME            ((DOUBLE)0.0)
#define CURRENT_TIME_LONG       CURRENT_TIME
#define ARCH_DIFF_TIMER(x,y) ((x)-(y))

/* ANSI-printf does not support %lX, where x is eEgGf */
#define _fmt_le                 "le"
#define _fmt_lE                 "lE"
#define _fmt_lg                 "lg"
#define _fmt_lG                 "lG"
#define _fmt_lf                 "lf"

/* Define to 1 if you have the <malloc.h> header file. */
#define HAVE_MALLOC_H 1

/****************************************************************************/
/*                                                                          */
/* #define exactly one of the following constants: (in Makefile)            */
/*                                                                          */
/*          __MPW32__    Apple MacIntosh Programmers Workshop version 3.2   */
/*          __SGI__      IRIS Indigo version                                */
/*          __PARIX__    PARIX Transputer version                           */
/*          __AIX__      AIX version (IBM)                                  */
/*          __PARAGON__  Intel Paragon version                              */
/*          __SUN4GCC__  Sun station 4 version                              */
/*          __HP__       HP Workstations                                    */
/*          __PC__       IBM compatible PC                                  */
/*          __T3D__      CRAY T3D version                                   */
/*          __T3E__      CRAY T3E version                                   */
/*          __POWERGC__  XPLORER (PowerPC)                                  */
/*          __CC__       CC-Parsytec (PowerPC)                              */
/*          __C90__      CRAY 90 version                                    */
/*          __YMP__      CRAY YMP version                                   */
/*          __NECSX4__   NEC SX4                                            */
/*          __SR2201__   Hitachi SR2201                                     */
/*          __SR8K__     Hitachi SR8000                                     */
/*          __MACOSXSERVER__   MacOS X Server                               */
/*          __MACOSX__   Mac OS X                                           */
/*          __MWCW__     Apple Power Macintosh  (Metrowerks CodeWarrior)    */
/*          __MKLINUX__  Mikrokernel Linux (PowerPC)                        */
/*          __LINUXPPC__                                                    */
/*          __LINUXAXP__                                                    */
/*          __LINUXIA64__                                                   */
/*          __CYGWIN__                                                      */
/*          __AMD64__                                                       */
/*                                                                          */
/* #define this if you are using NXLib                                      */
/*          __NXLIB__    NXLIB Paragon Library                              */
/*                                                                          */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* Definitions for Apple MacIntosh                                          */
/*                                                                          */
/****************************************************************************/

#ifdef __MPW32__
#undef __MWCW__

#define ARCHNAME        "MPW32"

/* memory */
#include <Memory.h>
#define malloc(n) ((void *) NewPtr((Size) n))
#define free(p) DisposPtr((Ptr) p)
#define ALIGNMENT 4                     /* power of 2 and >= sizeof(int) ! */
#define ALIGNMASK 0xFFFFFFFC            /* compatible to alignment */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym

#endif


/****************************************************************************/
/*                                                                          */
/* Definitions for Silicon Graphics Workstations (old 32 bit version)       */
/*                                                                          */
/****************************************************************************/

#ifdef __SGI__
#undef __MWCW__

#define ARCHNAME        "SGI"

#include <time.h>

/* memory */
#define ALIGNMENT 8                     /* power of 2 and >= sizeof(int) ! */
#define ALIGNMASK 0xFFFFFFF8            /* compatible to alignment */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym ## _

/* current time as DOUBLE value */
#undef CURRENT_TIME
#define CURRENT_TIME   (((DOUBLE)clock())/((DOUBLE)CLOCKS_PER_SEC))

#endif

/****************************************************************************/
/*                                                                          */
/* Definitions for Silicon Graphics Workstations (new 64 bit version)       */
/*                                                                          */
/****************************************************************************/

#ifdef __SGI10__
#undef __MWCW__

#define ARCHNAME        "SGI10"

#include <time.h>

/* memory */
#define ALIGNMENT 8                     /* power of 2 and >= sizeof(int) ! */
#define ALIGNMASK 0xFFFFFFF8            /* compatible to alignment */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym ## _

/* current time as DOUBLE value */
#undef CURRENT_TIME
#define CURRENT_TIME   (((DOUBLE)clock())/((DOUBLE)CLOCKS_PER_SEC))

#endif


/****************************************************************************/
/*                                                                          */
/* Definitions for PARIX Transputer version                                 */
/*                                                                          */
/****************************************************************************/

#ifdef __PARIX__
#undef __MWCW__

#define ARCHNAME        "PARIX"

#define __SWAPBYTES__ 1

/* memory */
#define ALIGNMENT 4                                         /* power of 2 and >= sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFFC                    /* compatible to alignment			*/

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym

/* current time as DOUBLE value */
#undef CURRENT_TIME
#define CURRENT_TIME   (((DOUBLE)TimeNow())/((DOUBLE)CLOCK_TICK))

#endif


/****************************************************************************/
/*                                                                          */
/* Definitions for AIX                                                      */
/*                                                                          */
/****************************************************************************/


#ifdef __AIX__
#undef __MWCW__

#define ARCHNAME        "AIX"

#define __SWAPBYTES__ 1

/* memory */
#define ALIGNMENT 4                     /* power of 2 and >= sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFFC            /* compatible to alignment          */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym

/* current time as DOUBLE value */
/* special high resolution time system for AIX */
DOUBLE aix_highres_clock( void );               /* implementation in misc.c */
#undef CURRENT_TIME
#define CURRENT_TIME        aix_highres_clock() /* see misc.c */

#endif


/****************************************************************************/
/*                                                                          */
/* Definitions for Intel Paragon version                                    */
/*                                                                          */
/****************************************************************************/

#ifdef __PARAGON__
#undef __MWCW__

#define ARCHNAME        "PARAGON"

#include <nx.h>

#define __SWAPBYTES__ 1

/* memory */
#define ALIGNMENT 8                                         /* power of 2 and >= sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFF8                    /* compatible to alignment			*/

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym ## _

/* current time as DOUBLE value */
#undef CURRENT_TIME
#define CURRENT_TIME   ((DOUBLE)dclock())

#endif


/****************************************************************************/
/*                                                                          */
/* Definitions for Solaris Version                                          */
/*                                                                          */
/****************************************************************************/

#ifdef __SOLARIS__
#undef __MWCW__

#define ARCHNAME        "SOLARIS"

#include <stddef.h>

/* memory */
#define ALIGNMENT 8                     /* power of 2 and >= sizeof(int) ! */
#define ALIGNMASK 0xFFFFFFF8            /* compatible to alignment */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym ## _

#endif

/****************************************************************************/
/*                                                                          */
/* Definitions for Sun station 4 version                                    */
/*                                                                          */
/****************************************************************************/

#ifdef __SUN4GCC__
#undef __MWCW__

#define ARCHNAME        "SUN4GCC"

#include <stddef.h>

/* memory */
#define ALIGNMENT 4                     /* power of 2 and >= sizeof(int) ! */
#define ALIGNMASK 0xFFFFFFFC            /* compatible to alignment */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym

#endif


/****************************************************************************/
/*                                                                          */
/* Definitions for Hewlett Packard HP700 series                             */
/*                                                                          */
/****************************************************************************/

#ifdef __HP__
#undef __MWCW__

#define ARCHNAME        "HP"

#include <time.h>

/* memory */
#define ALIGNMENT 8                     /* power of 2 and >= sizeof(int) ! */
#define ALIGNMASK 0xFFFFFFF8            /* compatible to alignment */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym

/* current time as DOUBLE value */
#undef CURRENT_TIME
#define CURRENT_TIME   (((DOUBLE)clock())/((DOUBLE)CLOCKS_PER_SEC))

#endif


/****************************************************************************/
/*                                                                          */
/* Definitions for Sun                                                      */
/*                                                                          */
/****************************************************************************/

#ifdef __SUN__
#undef __MWCW__

#define ARCHNAME        "SUN"

/* memory */
#define ALIGNMENT 8                     /* power of 2 and >= sizeof(int) ! */
#define ALIGNMASK 0xFFFFFFF8            /* compatible to alignment */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym

#endif


/****************************************************************************/
/*                                                                          */
/* Definitions for DEC                                                      */
/*                                                                          */
/****************************************************************************/

#ifdef __DEC__
#undef __MWCW__

#define ARCHNAME        "DEC"

/* memory */
#define ALIGNMENT   8                   /* power of 2 and >= sizeof(int) ! */
#define ALIGNMASK 0xFFFFFFF8            /* compatible to alignment */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym

#endif


/****************************************************************************/
/*                                                                          */
/* Definitions for IBM compatible Personal Computer                         */
/*                                                                          */
/****************************************************************************/

#ifdef __PC__
#undef __MWCW__

#define ARCHNAME        "PC"

#include <time.h>

#define __SWAPBYTES__ 1

/* memory */
#define ALIGNMENT 4                     /* power of 2 and >= sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFFC            /* compatible to alignment          */

/* fortran interfacing */
#define F77SYM(lsym,usym) lsym

/* current time as DOUBLE value */
#undef CURRENT_TIME

#if 0
/*
 * Use Time Stamp Counter (Pentium and later) for time measurement
 * Resolution: one clock cycle, Wraparound: after hundreds of years
 */
#define TICKS_PER_SEC 400919000 /* Adapt this!! (mean value for speedo's nodes) */
static inline double x86_highres_clock(void)
{
  unsigned long long t;
  __asm__ __volatile__ ("rdtsc" : "=A" (t));
  return (double)t/TICKS_PER_SEC;
}
#define CURRENT_TIME x86_highres_clock()

#else
#ifndef ARCH_LONG_TIMER
#define CURRENT_TIME   (((DOUBLE)clock())/((DOUBLE)CLOCKS_PER_SEC))
#else
#define CURRENT_TIME   ((double)time(NULL))
#undef ARCH_DIFF_TIMER
#define ARCH_DIFF_TIMER(x,y) (difftime((time_t)(x),(time_t)(y)))
#endif
#endif

#endif


/****************************************************************************/
/*                                                                          */
/* Definitions for Microkernel Linux (on Apple PowerMacs)                   */
/*                                                                          */
/****************************************************************************/

#ifdef __MKLINUX__
#undef __MWCW__

#define ARCHNAME        "MKLINUX"

#define __SWAPBYTES__ 1

/* memory */
#define ALIGNMENT 4                     /* power of 2 and >= sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFFC            /* compatible to alignment          */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym

#endif

/****************************************************************************/
/*                                                                          */
/* Definitions for LINUXAXP                                                 */
/*                                                                          */
/****************************************************************************/

#ifdef __LINUXAXP__
#undef __MWCW__

#define ARCHNAME        "LINUXAXP"

#include <time.h>

#define __SWAPBYTES__ 1

/* memory */
#define ALIGNMENT 8                     /* power of 2 and >= sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFF8            /* compatible to alignment          */

/* fortran interfacing */
#define F77SYM(lsym,usym) lsym

/* current time as DOUBLE value */
#undef CURRENT_TIME
#define CURRENT_TIME   (((DOUBLE)clock())/((DOUBLE)CLOCKS_PER_SEC))

#endif

/****************************************************************************/
/*                                                                          */
/* Definitions for LINUXIA64                                                */
/*                                                                          */
/****************************************************************************/

#ifdef __LINUXIA64__
#undef __MWCW__

#define ARCHNAME        "LINUXIA64"

#include <time.h>

#define __SWAPBYTES__ 1

/* memory */
#define ALIGNMENT 8                     /* power of 2 and >= sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFF8            /* compatible to alignment          */

/* fortran interfacing */
#define F77SYM(lsym,usym) lsym

/* current time as DOUBLE value */
#undef CURRENT_TIME
#define CURRENT_TIME   (((DOUBLE)clock())/((DOUBLE)CLOCKS_PER_SEC))

#endif

/****************************************************************************/
/*                                                                          */
/* Definitions for Cygwin                                                   */
/*                                                                          */
/****************************************************************************/

#ifdef __CYGWIN__
#undef __MWCW__

#define ARCHNAME    "CYGWIN"

#include <time.h>

#define __SWAPBYTES__ 1

/* memory */
#define ALIGNMENT 4                     /* power of 2 and >= sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFFC            /* compatible to alignment          */

/* fortran interfacing */
#define F77SYM(lsym,usym) lsym

/* current time as DOUBLE value */
#undef CURRENT_TIME
#define CURRENT_TIME   (((DOUBLE)clock())/((DOUBLE)CLOCKS_PER_SEC))

#endif

/****************************************************************************/
/*                                                                          */
/* Definitions for AMD64                                                    */
/*                                                                          */
/****************************************************************************/

#ifdef __AMD64__
#undef __MWCW__

#define ARCHNAME    "AMD64"

#include <time.h>

#define __SWAPBYTES__ 1

/* memory */
#define ALIGNMENT 8                     /* power of 2 and >= sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFF8            /* compatible to alignment          */

/* fortran interfacing */
#define F77SYM(lsym,usym) lsym

/* current time as DOUBLE value */
#undef CURRENT_TIME
#define CURRENT_TIME   (((DOUBLE)clock())/((DOUBLE)CLOCKS_PER_SEC))

#endif

/****************************************************************************/
/*                                                                          */
/* Definitions for LINUXPPC                                                 */
/*                                                                          */
/****************************************************************************/

#ifdef __LINUXPPC__
#undef __MWCW__

#define ARCHNAME        "LINUXPPC"

#include <time.h>

/* memory */
#define ALIGNMENT 4                     /* power of 2 and >= sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFFC            /* compatible to alignment          */

/* fortran interfacing */
#define F77SYM(lsym,usym) lsym

/* current time as DOUBLE value */
#undef CURRENT_TIME
#define CURRENT_TIME   (((DOUBLE)clock())/((DOUBLE)CLOCKS_PER_SEC))

#endif

/****************************************************************************/
/*                                                                          */
/* Definitions for CRAY T3D                                                 */
/*                                                                          */
/****************************************************************************/

#ifdef __T3D__
#undef __MWCW__

#define ARCHNAME        "T3D"

#include <time.h>       /* for rtclock()   */
#include <unistd.h>     /* for _SC_CLK_TCK */

/* memory */
#define ALIGNMENT 8                     /* power of 2 and >= sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFF8            /* compatible to alignment          */

/* fortran interfacing */
#define F77SYM(lsym,usym)  usym

/* current time as DOUBLE value */
#undef CURRENT_TIME
#define CURRENT_TIME ((DOUBLE)(((double)rtclock())/((double)sysconf(_SC_CLK_TCK))))

#endif



/****************************************************************************/
/*                                                                          */
/* Definitions for CRAY T3E                                                 */
/*                                                                          */
/****************************************************************************/

#ifdef __T3E__
#undef __MWCW__

#define ARCHNAME        "T3E"

#include <time.h>       /* for rtclock()   */
#include <unistd.h>     /* for _SC_CLK_TCK */

/* memory */
#define ALIGNMENT 8                     /* power of 2 and >= sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFF8            /* compatible to alignment          */

/* fortran interfacing */
#define F77SYM(lsym,usym)  usym

/* current time as DOUBLE value */
#undef CURRENT_TIME
#define CURRENT_TIME ((DOUBLE)(((double)rtclock())/((double)sysconf(_SC_CLK_TCK))))

#endif


/****************************************************************************/
/*                                                                          */
/* Definitions for XPLORER (PowerPC)  version                               */
/*                                                                          */
/****************************************************************************/

#if defined(__POWERGC__)
#undef __MWCW__

#define ARCHNAME        "POWERGC"

/* memory */
#define ALIGNMENT 8               /* power of 2 and >=sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFF8     /*  compatible to alignment */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym

#endif




/****************************************************************************/
/*                                                                          */
/* Definitions for CC-Parsytec                                              */
/*                                                                          */
/****************************************************************************/

#ifdef __CC__
#undef __MWCW__

#define ARCHNAME        "CC"

/* memory */
#define ALIGNMENT 4               /* power of 2 and >=sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFFC     /*  compatible to alignment */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym

#endif


/****************************************************************************/
/*                                                                          */
/* Definitions for CRAY 90                                                  */
/*                                                                          */
/****************************************************************************/

#ifdef __C90__
#undef __MWCW__

#define ARCHNAME        "C90"

/* memory */
#define ALIGNMENT 8                     /* power of 2 and >= sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFF8            /* compatible to alignment          */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym

#endif



/****************************************************************************/
/*                                                                          */
/* Definitions for CRAY YMP                                                 */
/*                                                                          */
/****************************************************************************/

#ifdef __YMP__
#undef __MWCW__

#define ARCHNAME        "YMP"

/* memory */
#define ALIGNMENT 8                     /* power of 2 and >= sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFF8            /* compatible to alignment          */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym

#endif


/****************************************************************************/
/*                                                                          */
/* Definitions for NEC SX4                                                  */
/*                                                                          */
/****************************************************************************/

#ifdef __NECSX4__
#undef __MWCW__

#define ARCHNAME        "NECSX4"

#include <time.h>

/* memory */
#define ALIGNMENT 8                     /* power of 2 and >= sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFF8            /* compatible to alignment          */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym ## _

/* current time as DOUBLE value */
/* special high performance time system for NEC SX4 */
DOUBLE nec_clock( void );               /* implementation in mics.c */
#undef CURRENT_TIME
#define CURRENT_TIME nec_clock()        /* see misc.c */

#endif



/****************************************************************************/
/*                                                                          */
/* Definitions for Hitachi SR2201                                           */
/*                                                                          */
/****************************************************************************/

#ifdef __SR2201__
#undef __MWCW__

#define ARCHNAME        "SR2201"

/* memory */
#define ALIGNMENT 8                     /* power of 2 and >= sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFF8            /* compatible to alignment          */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym ## _

/* current time as DOUBLE value */
#undef CURRENT_TIME
#define CURRENT_TIME   (((DOUBLE)clock())/((DOUBLE)CLOCKS_PER_SEC))

#endif


/****************************************************************************/
/*                                                                          */
/* Definitions for Hitachi SR8000                                           */
/*                                                                          */
/****************************************************************************/

#ifdef __SR8K__
#undef __MWCW__

#define ARCHNAME        "SR8K"

/* memory */
#define ALIGNMENT 8                     /* power of 2 and >= sizeof(int) !  */
#define ALIGNMASK 0xFFFFFFF8            /* compatible to alignment          */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym ## _

/* current time as DOUBLE value */
#undef CURRENT_TIME
#define CURRENT_TIME   (((DOUBLE)clock())/((DOUBLE)CLOCKS_PER_SEC))

#endif


/****************************************************************************/
/*                                                                          */
/* Definitions for MACOSXSERVER                                             */
/*                                                                          */
/****************************************************************************/

#ifdef __MACOSXSERVER__
#undef __MWCW__

#define ARCHNAME        "MacOS X Server"

/* memory */
#define ALIGNMENT     4             /* power of 2 and >= sizeof(int) ! */
#define ALIGNMASK     0xFFFFFFFC    /* compatible to alignment */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym

/* current time as DOUBLE value */
#undef CURRENT_TIME
#define CURRENT_TIME   (((DOUBLE)clock())/((DOUBLE)CLOCKS_PER_SEC))

#endif


/****************************************************************************/
/*                                                                          */
/* Definitions for Mac OS X                                                 */
/*                                                                          */
/****************************************************************************/

#ifdef __MACOSX__
#undef __MWCW__

#define ARCHNAME        "Mac OS X"

/* memory */
#define ALIGNMENT     4             /* power of 2 and >= sizeof(int) ! */
#define ALIGNMASK     0xFFFFFFFC    /* compatible to alignment */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym

/* current time as DOUBLE value */
#undef CURRENT_TIME
#define CURRENT_TIME   (((DOUBLE)clock())/((DOUBLE)CLOCKS_PER_SEC))

/* MAcOS does not have malloc.h */
#undef HAVE_MALLOC_H

#endif

#ifdef __PPC64__
#undef __MWCW__

#define ARCHNAME    "PowerPC 64 bit"

/* memory */
#define ALIGNMENT 8                     /* power of 2 and >= sizeof(INT) !  */
#define ALIGNMASK 0xFFFFFFF8            /* compatible to alignment          */

/* fortran interfacing */
#define F77SYM(lsym,usym) lsym

/* current time as DOUBLE value */
#undef CURRENT_TIME
#define CURRENT_TIME   (((DOUBLE)clock())/((DOUBLE)CLOCKS_PER_SEC))

#endif


/****************************************************************************/
/*                                                                          */
/* Definitions for Apple Macintosh with CodeWarrior compiler                */
/*                                                                          */
/****************************************************************************/

#ifdef __MWCW__

#define ARCHNAME        "MWCW"

#define USES_UNIVERSAL_HEADERS

/*
    for older versions of the CW C-compiler you may have to
   #define __MWCW_oldVersion__
    (see MacShell.c)
 */

/* memory */
#include <Memory.h>
/*#define malloc(n) ((void *) NewPtr((Size) n))
 #define free(p) DisposPtr((Ptr) p)*/
#define ALIGNMENT 4                     /* power of 2 and >= sizeof(int) ! */
#define ALIGNMASK 0xFFFFFFFC            /* compatible to alignment */

/* fortran interfacing */
#define F77SYM(lsym,usym)  lsym

/* current time as DOUBLE value */
#undef CURRENT_TIME
#define CURRENT_TIME   (((DOUBLE)clock())/((DOUBLE)CLOCKS_PER_SEC))

#undef _fmt_lg
#undef _fmt_lG
#undef _fmt_le
#undef _fmt_lE
#undef _fmt_lf
#define _fmt_lg                 "%g"
#define _fmt_lG                 "%G"
#define _fmt_le                 "%e"
#define _fmt_lE                 "%E"
#define _fmt_lf                 "%f"

#endif


/****************************************************************************/
/*                                                                          */
/* Definitions for NXLIB 1.1 version                                        */
/*                                                                          */
/****************************************************************************/

#ifdef __NXLIB__

#include <nxmalloc.h>                   /* redefine malloc and related calls*/

#endif


/****************************************************************************/
/*                                                                          */
/* some general definitions						                            */
/*                                                                          */
/****************************************************************************/

#if (defined __MWCW__) || (defined __MPW32__)
#define __MACINTOSH__
#endif

#define ARCH_VERSION "ARCH_1_0"

/* The following is actually a definition (and not just a declaration) in a header
   file.  Since DUNE actually includes compiler.h twice from a single source, this
   definition has to go.  Yes, I know, including a header twice is not nice, but
   I currently see no other way.
 */
#ifndef FOR_DUNE
static char compilerrcs_id[] = "$Id$";
#endif

/* limits of the basic types */
#define MAX_S            SHRT_MAX
#define MAX_I            INT_MAX
#define MAX_F            FLT_MAX
#define SMALL_F         (FLT_EPSILON*SMALL_FAC)
#define MAX_D            DBL_MAX
#define SMALL_D         (DBL_EPSILON*SMALL_FAC)
#define MAX_C            FLT_MAX
#define SMALL_C         (FLT_EPSILON*SMALL_FAC)

#ifdef __cplusplus
}
#endif

/* end of ifdef AUTOTOOLS_BUILD */
#endif

/* end of ifdef __COMPILER__ */
#endif
