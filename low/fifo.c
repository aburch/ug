// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*	                                                                        */
/* File:      fifo.c                                                        */
/*                                                                          */
/* Purpose:   general purpose first in first out queue                      */
/*                                                                          */
/* Author:      Peter Bastian                                               */
/*              Interdisziplinaeres Zentrum fuer Wissenschaftliches Rechnen */
/*              Universitaet Heidelberg                                     */
/*              Im Neuenheimer Feld 368                                     */
/*              6900 Heidelberg                                             */
/*                                                                          */
/* History:   30.01.92 begin, ug version 2.0                                */
/*                                                                          */
/* Revision:  07.09.95                                                      */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* include files                                                            */
/*              system include files                                        */
/*              application include files                                   */
/*                                                                          */
/****************************************************************************/

#include <stdio.h>

#include "compiler.h"
#include "general.h"
#include "fifo.h"

USING_UG_NAMESPACES

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/

/* RCS string */
static char RCS_ID("$Header$",UG_RCS_STRING);

/****************************************************************************/
/*D
   fifo_init - Initialize fifo data structure

   SYNOPSIS:
   INT fifo_init (FIFO *myfifo, void *buffer, INT size)

   PARAMETERS:
   .  myfifo - pointer to a fifo record
   .  buffer - pointer to a memory area for the fifo
   .  size -   size of the buffer in bytes

   DESCRIPTION:
   This function initializes 'fifo' (first in, first out) data structure.

   RETURN VALUE:
   INT
   .n     size of fifo record
   .n     0 if size lower or equal to zero
   D*/
/****************************************************************************/

INT NS_PREFIX fifo_init (FIFO *myfifo, void *buffer, INT size)
{
  myfifo->size = size / sizeof(void *);
  if (myfifo->size<=0) return(0);
  myfifo->elements = (void **) buffer;
  myfifo->start = myfifo->end = myfifo->used = 0;
  return(myfifo->size);
}

/****************************************************************************/
/*D
   fifo_clear - Reset a previously initialized FIFO structure

   SYNOPSIS:
   void fifo_clear (FIFO *myfifo)

   PARAMETERS:
   .  myfifo - pointer to FIFO structure

   DESCRIPTION:
   This function resets a previously initialized FIFO structure.

   RETURN VALUE:
   void
   D*/
/****************************************************************************/

void NS_PREFIX fifo_clear (FIFO *myfifo)
{
  myfifo->start = myfifo->end = myfifo->used = 0;
}

/****************************************************************************/
/*D
   fifo_empty - Test if fifo is empty

   SYNOPSIS:
   INT fifo_empty (const FIFO *myfifo)

   PARAMETERS:
   .  myfifo - pointer to fifo structure

   DESCRIPTION:
   This function tests if fifo is empty.

   RETURN VALUE:
   INT
   .n     0 if fifo is not empty
   .n     1 if fifo is empty
   D*/
/****************************************************************************/

INT NS_PREFIX fifo_empty (const FIFO *myfifo)
{
  if (myfifo->used==0)
    return(1);
  else
    return(0);
}

/****************************************************************************/
/*D
   fifo_full - Test if fifo is full

   SYNOPSIS:
   INT fifo_full (const FIFO *myfifo)

   PARAMETERS:
   .  myfifo - pointer to FIFO structure

   DESCRIPTION:
   This function tests if fifo is full.

   RETURN VALUE:
   INT
   .n     0 if fifo is not empty
   .n     1 if fifo is empty
   D*/
/****************************************************************************/

INT NS_PREFIX fifo_full (const FIFO *myfifo)
{
  if (myfifo->used==myfifo->size)
    return(1);
  else
    return(0);
}

/****************************************************************************/
/*D
   fifo_in - Insert an element in the fifo

   SYNOPSIS:
   INT fifo_in (FIFO *myfifo, void *newelement)

   PARAMETERS:
   .  myfifo - pointer to FIFO structure
   .  newelement - pointer to the new element (any type!)

   DESCRIPTION:
   This function inserts an element in the fifo.

   RETURN VALUE:
   INT
   .n     1 if error occurred.
   .n     0 if OK
   D*/
/****************************************************************************/

INT NS_PREFIX fifo_in (FIFO *myfifo, void *newelement)
{
  if (myfifo->used<myfifo->size)
  {
    (myfifo->elements)[myfifo->end] = newelement;
    myfifo->end = (myfifo->end+1)%myfifo->size;
    myfifo->used++;
    return(0);
  }
  else
    return(1);
}

/****************************************************************************/
/*D
   fifo_out - Get an element from the fifo

   SYNOPSIS:
   void *fifo_out (FIFO *myfifo)

   PARAMETERS:
   .  myfifo - pointer to fifo structure

   DESCRIPTION:
   This function gets an element from the fifo.

   RETURN VALUE:
   void *
   .n        pointer to the element to take out from the fifo
   .n        NULL if fifo is empty
   D*/
/****************************************************************************/

void *NS_PREFIX fifo_out (FIFO *myfifo)
{
  INT i;

  if (myfifo->used==0)
    return(NULL);
  else
  {
    i = myfifo->start;
    myfifo->start = (myfifo->start+1)%myfifo->size;
    myfifo->used--;
    return (myfifo->elements[i]);
  }
}
