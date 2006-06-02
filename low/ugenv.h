// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                              */
/* File:      ugenv.h                                                       */
/*                                                                          */
/* Purpose:   header file for ug environment manager                        */
/*                                                                          */
/* Author:      Peter Bastian                                               */
/*              Interdisziplinaeres Zentrum fuer Wissenschaftliches Rechnen */
/*              Universitaet Heidelberg                                     */
/*              Im Neuenheimer Feld 368                                     */
/*              6900 Heidelberg                                             */
/*                                                                          */
/* History:   06.02.92 begin, ug version 2.0                                */
/*                                                                          */
/* Revision:  08.09.95                                                      */
/*                                                                          */
/****************************************************************************/


/* RCS_ID
   $Header$
 */

/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __UGENV__
#define __UGENV__


#include "ugtypes.h"

#include "namespace.h"

START_UG_NAMESPACE

/*****************************************************************************/
/*                                                                           */
/* defines in the following order                                            */
/*                                                                           */
/*          compile time constants defining static data size (i.e. arrays)   */
/*          other constants                                                  */
/*          macros                                                           */
/*                                                                           */
/*****************************************************************************/

/* CAUTION: when changing NAMESIZE also change scanf sizes!!! */
enum {NAMESIZE = 128};                 /* max length of name string            */
enum {NAMELEN  = 127};                 /* NAMESIZE-1                            */
#define NAMELENSTR             "127"    /* NAMESIZE-1 as string                    */

enum {SEARCHALL = -1};                 /*!< Scan through all directories         */
#define DIRSEP                "/"     /* character separating directories     */

/* directories with odd numbers */
#define ROOT_DIR            1        /* indicates root directory             */

/** \brief Return pointer to the first 'ENVITEM' contained in the directory. */
#define ENVITEM_DOWN(p)         (((ENVITEM *)(p))->d.down)

/** \brief Return pointer to the first 'ENVDIR' contained in the directory. */
#define ENVDIR_DOWN(p)            ((p)->down)

/** \brief Return pointer to the next 'ENVITEM' in the doubly linked list */
#define NEXT_ENVITEM(p)         (((ENVITEM *)(p))->v.next)

/** \brief Return pointer to the previous 'ENVITEM' in the doubly linked list. */
#define PREV_ENVITEM(p)         (((ENVITEM *)(p))->v.previous)

/** \brief Return the type of the 'ENVITEM' (odd: 'ENVDIR', even: 'ENVVAR'). */
#define ENVITEM_TYPE(p)         (((ENVITEM *)(p))->v.type)

#define IS_ENVDIR(p)            (ENVITEM_TYPE(p)%2==1)

/** \brief This macro returns a pointer to the name string of the 'ENVITEM'. */
#define ENVITEM_NAME(p)         (((ENVITEM *)(p))->v.name)

/** \brief 'RemoveEnvItem' checks this and returns an error if TRUE. */
#define ENVITEM_LOCKED(p)         (((ENVITEM *)(p))->v.locked)

/****************************************************************************/
/*                                                                          */
/* data structures exported by the corresponding source file                */
/*                                                                          */
/****************************************************************************/

/** \brief User-defined variable */
typedef struct {

  /** \brief One of the variable types above            */
  INT type;

  /** \brief May not be changed or deleted            */
  INT locked;

  /** \brief Doubly linked list of environment items    */
  union envitem *next;
  union envitem *previous;

  /** \brief Name of that item. May be longer, but of no interest for env*/
  char name[NAMESIZE];

} ENVVAR;

/** \brief Directory */
typedef struct {

  /** \brief One of the directory types above         */
  INT type;

  /** \brief May not be changed or deleted            */
  INT locked;

  /** \brief Doubly linked list of environment items    */
  union envitem *next;
  union envitem *previous;

  /** \brief Name of that item                        */
  char name[NAMESIZE];

  /** \brief One level down in the tree                */
  union envitem *down;
} ENVDIR;

union envitem {
  ENVVAR v;
  ENVDIR d;
};

typedef union envitem ENVITEM;

/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/

/* initialize environment with following heapSize */
INT      InitUgEnv        (INT heapSize);

/* Free all memory allocated for the environment */
INT      ExitUgEnv();

/* change directory allows /, .., etc */
ENVDIR    *ChangeEnvDir    (const char *s);

/* get the current working directory */
ENVDIR    *GetCurrentDir    (void);

/* get path name of the current directory */
void     GetPathName    (char *s);

/* create a new environment item with user defined size */
ENVITEM *MakeEnvItem    (const char *name, const INT type, const INT size);

/* remove an item */
INT      RemoveEnvItem    (ENVITEM *theItem);

/* remove an complete directory */
INT      RemoveEnvDir     (ENVITEM *theItem);

/* move an envitem to a new directory */
INT              MoveEnvItem      (ENVITEM *item, ENVDIR *oldDir, ENVDIR *newDir);

/* search the environment for an item */
ENVITEM *SearchEnv        (const char *name, const char *where, INT type, INT dirtype);

/* allocate memory from the environment heap */
void    *AllocEnvMemory (INT size);

/* deallocate memory from the environment heap */
void     FreeEnvMemory    (void *buffer);

/* print used and size of environment heap */
void     EnvHeapInfo     (char *s);

/* return a unique ID for a new ENVDIR/ENVVAR type */
INT      GetNewEnvDirID (void);
INT      GetNewEnvVarID (void);


END_NAMESPACE

#endif
