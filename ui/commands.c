// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  commands.c													*/
/*																			*/
/* Purpose:   definition of all dimension independent commands of ug		*/
/*																			*/
/* Author:	  Henrik Rentz-Reichert                                                                                 */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de						        */
/*																			*/
/* History:   29.01.92 begin, ug version 2.0								*/
/*			  02.02.95 begin, ug version 3.0								*/
/*																			*/
/* Remarks:   for dimension dependent commands see commands2d/3d.c			*/
/*																			*/
/****************************************************************************/

#ifdef __MPW32__
#pragma segment cmd
#endif

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

/* standard C library */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>

/* low module */
#include "compiler.h"
#include "defaults.h"
#include "misc.h"
#include "ugstruct.h"
#include "fileopen.h"
#include "ugenv.h"
#include "debug.h"
#include "heaps.h"              /* for MEM declaration */
#include "general.h"

/* devices module */
#include "ugdevices.h"

/* grid manager module */
#include "gm.h"
#include "pargm.h"
#include "rm.h"
#include "evm.h"
#include "ugm.h"
#include "algebra.h"
#include "shapes.h"
#include "mgio.h"

/* grid generator module */
#ifdef __TWODIM__
#include "ggm.h"
#include "ggmain.h"
#endif
#if defined __THREEDIM__ && defined _NETGEN
#include "gg3d.h"
#endif

/* numerics module */
#include "np.h"
#include "ugblas.h"
#include "formats.h"
#include "disctools.h"
#include "data_io.h"
#include "npcheck.h"
#include "udm.h"
#include "fvgeom.h"

/* graph module */
#include "wpm.h"
#include "wop.h"
#include "graph.h"
#include "connectuggrape.h"
#ifdef _COVISE
#include "coviseif.h"
#endif

/* user interface module */
#include "uginterface.h"
#include "ugstruct.h"
#include "cmdint.h"
#include "cmdline.h"
#include "helpmsg.h"
#include "mmio.h"
#include "dio.h"

#ifdef ModelP
#include "parallel.h"
#endif

#ifdef __DLB__
#include "dlb.h"
#include "dlb_eval.h"
#endif

#ifdef CHACOT
#include "lb4.h"
#endif

/* own header */
#include "commands.h"

#include "initug.h"

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

#define BUFFERSIZE                              512     /* size of the general purpose text buff*/

#define WHITESPACE                              " \t"

#define LONGSTRSIZE                     256 /* size of some strings                             */
#define LONGSTRLEN                              255 /* len of some strings                                      */
#define LONGSTRLENSTR                   "255" /* LONGSTRLEN as string                           */

/* for ProtoOnCommand */
#define NORENAME_PROTO                  0
#define APPEND_PROTO                    1
#define RENAME_PROTO                    2
#define TRYRENAME_PROTO                 3
#define MAXPATHLENGTH                   255
#define MAXRENAMECHAR                   'z'

/* for the .list commands */
#define DO_ID                                   1
#define DO_SELECTION                    2
#define DO_ALL                                  3

/* for MarkCommand */
#define MARK_ALL                                1
#define AI_MARK_ALL             256
#define MARK_COARSEN                    2
#define MARK_ID                                 3
#define MARK_SELECTION                  4
#define NO_SIDE_SPECIFIED               -1
#define NO_RULE_SPECIFIED               -1
#define NO_OF_RULES                     64

/* for anisotropic refinement */

/* for save command */
#define NO_COMMENT                               "no comment"

/* for array commands */
#define AR_NVAR_MAX                     10
#define AR_NVAR(p)                      ((p)->nVar)
#define AR_VARDIM(p,i)          ((p)->VarDim[i])
#define AR_DATA(p,i)            ((p)->data[i])

/****************************************************************************/
/*																			*/
/* data structures used in this source file (exported data structures are	*/
/*		  in the corresponding include file!)								*/
/*																			*/
/****************************************************************************/

struct MarkRule
{
  char *RuleName;                                       /* what you type in the mark cmdline	*/
  INT RuleId;                                           /* corresponding rule ID for refine     */
};

typedef struct MarkRule MARKRULE;

typedef struct
{
  /* fields for environment directory */
  ENVVAR v;

  /* data */
  INT nVar;
  INT VarDim[AR_NVAR_MAX];
  DOUBLE data[1];

} ARRAY;

/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/

static MULTIGRID *currMG=NULL;                  /* the current multigrid			*/

static NP_BASE *currNumProc=NULL;               /* current numerical procedure		*/

static char buffer[BUFFERSIZE];         /* general purpose text buffer		*/

static FILE     *protocolFile=NULL;     /* for protocol commands			*/

static MARKRULE myMR[NO_OF_RULES]=      /* name and ID of available rules	*/
{{"red",        RED},
 {"no",         NO_REFINEMENT},
 {"blue",       BLUE},
 {"copy",       COPY},
                                         #ifdef __TWODIM__
 {"bi_1",       BISECTION_1},
 {"bi_2q",      BISECTION_2_Q},
 {"bi_2t1", BISECTION_2_T1},
 {"bi_2t2", BISECTION_2_T2},
 {"bi_3",       BISECTION_3},
                                         #endif
                                         #ifdef __THREEDIM__
 /* rules for tetrahedra */
                                         #ifndef TET_RULESET
 {"tet2hex",TETRA_RED_HEX},
                                         #endif
 /* rules for prisms */
 {"pri_quadsect",PRISM_QUADSECT},
 {"pri_bisect_hex0",PRISM_BISECT_HEX0},
 {"pri_bisect_hex1",PRISM_BISECT_HEX1},
 {"pri_bisect_hex2",PRISM_BISECT_HEX2},
 {"pri_rot_l",PRISM_ROTATE_LEFT},
 {"pri_rot_r",PRISM_ROTATE_RGHT},
 /* rules for tetrahedra */
 {"hex_bisect_eins",HEX_BISECT_0_1},
 {"hex_bisect_zwei",HEX_BISECT_0_2},
 {"hex_bisect_drei",HEX_BISECT_0_3},
 {"hex_trisect_eins",HEX_TRISECT_0},
 {"hex_trisect_fuenf",HEX_TRISECT_5},
                                         #endif
 {"coarse", COARSE}};

static DOUBLE Time0;                            /* time offset for readclock		*/

static char userPath[1024];                     /* environment path for ls,cd		*/

static INT untitledCounter=0;                   /* counter for untitled multigrids	*/

/* some variables to transfer info between QualityCommand and QualityElement*/
static DOUBLE min,max,themin,themax,minangle,maxangle;
static INT lessopt,greateropt,selectopt;
static char mintext[32],maxtext[32],minmaxtext[32];

/* counters for windows and pictures */
static INT wincounter=1;
static INT piccounter=1;

/* stuff for the array commands */
static INT theArrayDirID;
static INT theArrayVarID;
static INT arraypathes_set=FALSE;

REP_ERR_FILE;

/* RCS string */
static char RCS_ID("$Header$",UG_RCS_STRING);


/****************************************************************************/
/*
 */
/* forward declarations of functions used before they are defined                       */
/*
 */
/****************************************************************************/

#if defined(CAD) && defined(__THREEDIM__)
MULTIGRID *ConvertCADGrid  (char *theFormat, char *CADOutputFileName,unsigned long heapSize);
#endif


/****************************************************************************/
/*D
   GetCurrentMultigrid - return a pointer to the current multigrid

   SYNOPSIS:
   MULTIGRID *GetCurrentMultigrid (void);

   PARAMETERS:
   .  void -

   DESCRIPTION:
   This function returns a pionter to the current multigrid.

   RETURN VALUE:
   MULTIGRID *
   .n     pointer to multigrid
   .n     NULL if there is no current multigrid.
   D*/
/****************************************************************************/

MULTIGRID *GetCurrentMultigrid (void)
{
  return (currMG);
}

/****************************************************************************/
/*D
   SetCurrentMultigrid - set the current multigrid if it is valid

   SYNOPSIS:
   INT SetCurrentMultigrid (MULTIGRID *theMG);

   PARAMETERS:
   .  theMG - pointer to multigrid

   DESCRIPTION:
   This function sets the current multigrid if it is valid, i. e.
   the function checks whether 'theMG' acually points to a multigrid.
   It can be NULL only if no multigrid is open.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if theMG is not in the multigrid list
   D*/
/****************************************************************************/

INT SetCurrentMultigrid (MULTIGRID *theMG)
{
  MULTIGRID *mg;

  if (ResetPrintingFormat())
    REP_ERR_RETURN(CMDERRORCODE);

  mg = GetFirstMultigrid();
  if (mg==theMG)
  {
    /* possibly NULL */
    currMG = theMG;
    return (0);
  }

  for (; mg!=NULL; mg=GetNextMultigrid(mg))
    if (mg==theMG)
    {
      /* never NULL */
      currMG = theMG;
      return (0);
    }

  return (1);
}

/****************************************************************************/
/*D
   quit - quit command

   DESCRIPTION:
   This command quits the program and closes the shell.

   'quit'

   KEYWORDS:
   exit, terminate, bye
   D*/
/****************************************************************************/

static INT QuitCommand (INT argc, char **argv)
{
  NO_OPTION_CHECK(argc,argv);
  SetDoneFlag();
  return(QUITCODE);
}

/****************************************************************************/
/*D
   exitug - exitug command

   DESCRIPTION:
   This command exits the current application and closes the shell.

   'exitug'

   KEYWORDS:
   terminate, bye, tschuess
   D*/
/****************************************************************************/

static INT ExitUgCommand (INT argc, char **argv)
{
  NO_OPTION_CHECK(argc,argv);

  ExitUg();

  exit(0);

  return 0;
}

/****************************************************************************/
/*D
   mute - set mutelevel

   DESCRIPTION:
   This command sets a mutelevel.
   The default value is 0 and all skript lines will be printed on the shell.
   This will be suppressed by mutelevel -1.
   Smaller muteleveles should reduce the output further.
   A mutelevel of -1000 supresses all output to shell.

   'mute <value>'
   .   <value> - integer which gives the mutelevel

   REMARK:
   Formally, this is not an ug command, 'mute' is checked in
   'InterpretString'.

   KEYWORDS:
   verbose, quiet, silent
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   help - print help for a command or keyword

   DESCRIPTION:
   This command prints help for a given helpitem, e.g. a command. The helpitem
   is looked up case insensitive. Command names can be abbreviated as if they
   where called from the shell window.

   help [[<helpitem>] $k]

   .   no~option      - this is  equivalent to 'help help'
   .   <helpitem>     - print help for <helpitem> (string)
   .   $k             - search for keyword <helpitem> (multiple occurence)

   EXAMPLE:
   'help PlotObj'

   prints help for the plotobject command.

   'help plot $k'

   prints a list of all commands which are relevant for plotting
   (openwindow, setview, zoom ...)
   D*/
/****************************************************************************/

static INT HelpCommand (INT argc, char **argv)
{
  INT i,res,mode,rv;
  COMMAND *Cmd;
  char buf[NAMESIZE];

        #ifdef ModelP
  if (me != master) return(OKCODE);
        #endif

  mode = HELPITEM;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'k' :
      mode = KEYWORD;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("help",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  /* get HelpFor string */
  res = sscanf(argv[0],expandfmt(CONCAT3("help %",NAMELENSTR,"[0-9a-zA-Z_]")),buf);
  if (res==1)
  {
    rv = PrintHelp(buf,mode,NULL);
    if (rv!=HELP_OK)
    {
      /* another try: ug command buf existing? */
      UserWrite("no help found\n"
                "maybe a command matches...\n");
      Cmd = SearchUgCmd(buf);
      if (Cmd!=NULL)
        rv = PrintHelp(ENVITEM_NAME(Cmd),mode,NULL);
    }
  }
  else
    rv = PrintHelp("help",HELPITEM,NULL);

  switch (rv)
  {
  case HELP_OK :
    return (OKCODE);

  case HELP_NOT_FOUND :
    UserWriteF(" no help entry found for '%s'\n",buf);
    return (OKCODE);

  default :
    PrintErrorMessage('E',"help","(unknown)");
  }

  return (CMDERRORCODE);
}

/****************************************************************************/
/*D
   checkhelp - check wether all commands in /menu have a help item

   DESCRIPTION:
   This function checks wether for all commands in /menu a help item exists.
   It also checks wether for all num proc types a help item exists.

   It prints all commands and num proc types for which help does NOT exist.

   It calls the funtion 'CheckHelp'.

   EXAMPLE:
   'checkhelp'

   KEYWORDS:
   check
   D*/
/****************************************************************************/

static INT CheckHelpCommand (INT argc, char **argv)
{
  NO_OPTION_CHECK(argc,argv);

        #ifdef ModelP
  if (me != master) return(OKCODE);
        #endif

  CheckHelp();

  return (OKCODE);
}

/****************************************************************************/
/*D
   cmfn - creates a metafile name

   DESCRIPTION:
   This command creates a metafile names.
   It creates a string containing the name of a metafile name for
   animation by xugv.

   'cmfn <name> <var>'

   .  <name> - first part of the metafile names
   .  <var> - the contents of var will be appended to the name

   EXAMPLE:
   .vb
   frame="
    cmfn film step;
    openwindow 0 0 820 420 $d meta $n @film;
    openpicture $s 10 10 800 400 $n framepic;
    setplotobject EScalar $e S2 $m COLOR $d 0 $f 0.0 $t 1.0;
    setview;
    zoom 0.4;
    plot;
    closewindow;
   ";
   step = 0;
   steps = 100;
   @frame;
   repeat {
    print "STEP ", step;
    @mysolve;
    step=step+1;
    @frame;
    if (step==steps) break;
   }
   .ve

   This runs 'mysolve' 100 times and
   creates metafiles 'film.0000', 'film.0001', 'film.0002', ... 'film.0100'.

   KEYWORDS:
   movie, film
   D*/
/****************************************************************************/

static INT CreateMetafileNameCommand (INT argc, char **argv)
{
  INT res,i,nopt;
  int frame;
  char name[LONGSTRSIZE],varname[LONGSTRSIZE];
  char fullname[LONGSTRSIZE];
  char *ext;

  nopt = 0;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'n' :
      if (sscanf(argv[i],expandfmt(CONCAT3("n %",NAMELENSTR,"[ -~]")),varname)!=1)
      {
        PrintErrorMessage('E',"cmfn","can't read varname");
        return(PARAMERRORCODE);
      }
      nopt = 1;
      break;
    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      return (PARAMERRORCODE);
    }

  res = sscanf(argv[0],expandfmt(CONCAT3(" cmfn %",LONGSTRLENSTR,"[0-9:.a-zA-Z_] %255[0-9:.a-zA-Z_]")),name,buffer);
  if (res!=2) return(CMDERRORCODE);

  if (GetStringValueInt(buffer,&frame)) return(CMDERRORCODE);
  ext = GetStringVar("EXT");

  if (ext==NULL)
    sprintf(fullname,"%s.%04d",name,frame);
  else
    sprintf(fullname,"%s.%04d.%s",name,frame,ext);

  if (nopt)
  {
    if (SetStringVar(varname,fullname)) return(CMDERRORCODE);
  }
  else
  {
    if (SetStringVar(name,fullname)) return(CMDERRORCODE);
  }

  return (OKCODE);
}

/****************************************************************************/
/*D
   readclock - printing execution time

   DESCRIPTION:
   This command is for measuring the time used.
   It prints the execution time since the last 'resetclock' to
   string variable ':CLOCK'.

   'readclock'

   KEYWORDS:
   time, stopwatch, clock

   SEE ALSO:
   resetclock;
   D*/
/****************************************************************************/

static INT ReadClockCommand (INT argc, char **argv)
{
  DOUBLE Time;

  NO_OPTION_CHECK(argc,argv);

  Time = CURRENT_TIME_LONG - Time0;

  if (SetStringValue(":CLOCK",Time)!=0) {
    PrintErrorMessage('E',"readclock",
                      "could not get string variable :CLOCK");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}

/****************************************************************************/
/*D
   resetclock - starting the time mesuring

   DESCRIPTION:
   This command starts the time mesuring.
   It sets the global variable 'Time0' to zero.

   'resetclock'

   KEYWORDS:
   time, stopwatch, clock

   SEE ALSO:
   readclock;
   D*/
/****************************************************************************/

static INT ResetClockCommand (INT argc, char **argv)
{
  NO_OPTION_CHECK(argc,argv);

  Time0 = CURRENT_TIME_LONG;

  return (OKCODE);
}

/****************************************************************************/
/*D
   InitClock(void) - starting the time mesuring

   SYNOPSIS:
   static INT InitClock(void);

   PARAMETERS:
   .  void -

   DESCRIPTION:
   This function starts the time mesuring.
   It sets the global variable 'Time0' to zero.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
   D*/
/****************************************************************************/

static INT InitClock(void)
{
  Time0 = CURRENT_TIME_LONG;

  return(0);
}

/****************************************************************************/
/*D
   date - prints the date

   DESCRIPTION:
   This command prints the date to the shell resp.
   writes it in the string variable ':date'.

   'date [$s] [$S]'

   .  no~option - print the date to the shell
   .  $s                 -  put in the string variable ':date'.
   .  $S         -  use short format of the form yy.mm.dd

   KEYWORDS:
   time, calendar

   SEE ALSO:
   'resetclock', 'readclock'
   D*/
/****************************************************************************/

static INT DateCommand (INT argc, char **argv)
{
  time_t Time;
  char *fmt;
  INT i,svopt;

  /* check options */
  svopt = FALSE;
  fmt = "%a %b %d %H:%M:%S %Y";
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 's' :
      svopt = TRUE;
      break;

    case 'S' :
      fmt = "%y.%m.%d";;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("date",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  time(&Time);
  strftime(buffer,BUFFERSIZE,fmt,localtime(&Time));

  if (svopt)
    SetStringVar(":date",buffer);
  else
    UserWriteF("%s\n",buffer);

  return (OKCODE);
}

/****************************************************************************/
/*D
   ls - lists the content of an environment directory.

   DESCRIPTION:
   This command lists the content of an environment directory.

   'ls [<path>]'

   .  no~option - lists the content of the current directory.
   .  <path>    - contains the relative or absolute path in UNIX-style

   KEYWORDS:
   environment, directory, list
   D*/
/****************************************************************************/

static INT ListEnvCommand (INT argc, char **argv)
{
  ENVDIR *currentDir;
  ENVITEM *theItem;
  char *s;
  INT i;

  NO_OPTION_CHECK(argc,argv);

  /* load current directory */
  currentDir = ChangeEnvDir(userPath);
  if (currentDir==NULL)
  {
    /* directory is invalid -> change to root directory */
    strcpy(userPath,DIRSEP);
    currentDir = ChangeEnvDir(userPath);
    if (currentDir == NULL)
      return (CMDERRORCODE);
  }

  /* strip ' '*ls' '* */
  s = strchr(argv[0],'l');
  strcpy(buffer,s);
  i = 2;
  while ((buffer[i]!='\0') && (strchr(WHITESPACE,buffer[i])!=NULL)) i++;
  s = buffer+i;

  /* pathname is now in s: change dir to path */
  if (strlen(s)>0)
    currentDir = ChangeEnvDir(s);

  if (currentDir==NULL)
  {
    PrintErrorMessage('E',"ls","invalid path as argument");
    return (CMDERRORCODE);
  }

  theItem = currentDir->down;
  while (theItem!=NULL)
  {
    UserWrite(theItem->v.name);
    if (theItem->v.type%2==0)
      UserWrite("\n");
    else
      UserWrite("*\n");
    theItem = theItem->v.next;
  }

  return(OKCODE);
}

/****************************************************************************/
/*D
   cd - change the current environment directory.

   DESCRIPTION:
   This command changes the current environment directory.
   It uses the function 'ChangeEnvDir'.

   'cd [<path>]'

   .  no~option - cd to root (cd /)
   .  <path>	 - <path> contains the relative or absolute path in UNIX-style

   KEYWORDS:
   environment, directory, working
   D*/
/****************************************************************************/

static INT ChangeEnvCommand (INT argc, char **argv)
{
  ENVDIR *currentDir;
  char *s;
  INT i;

  NO_OPTION_CHECK(argc,argv);

  /* load current directory */
  currentDir = ChangeEnvDir(userPath);
  if (currentDir==NULL)
  {
    /* directory is invalid -> change to root directory */
    strcpy(userPath,DIRSEP);
    currentDir = ChangeEnvDir(userPath);
    if (currentDir == NULL)
      return (CMDERRORCODE);
  }

  /* strip ' '*cd' '* */
  s = strchr(argv[0],'c');
  strcpy(buffer,s);
  i = 2;
  while ((buffer[i]!='\0') && (strchr(WHITESPACE,buffer[i])!=NULL)) i++;
  s = buffer+i;

  /* pathname is now in buffer */
  if (strlen(buffer)==0)
  {
    /* empty path: change to root directory */
    strcpy(userPath,DIRSEP);
    currentDir = ChangeEnvDir(userPath);
    if (currentDir == NULL)
      return (CMDERRORCODE);
    else
      return (OKCODE);
  }
  currentDir = ChangeEnvDir(s);
  if (currentDir==NULL)
  {
    PrintErrorMessage('E',"cd","invalid path as argument");
    return (CMDERRORCODE);
  }
  GetPathName(userPath);
  UserWrite(userPath);
  UserWrite("\n");

  return (OKCODE);
}

/****************************************************************************/
/*D
   pwd - print the current environment directory

   DESCRIPTION:
   This command print the current environment directory to the shell.
   It uses the function 'CangeEnvDir'.

   'pwd'

   KEYWORDS:
   environment, directory, working
   D*/
/****************************************************************************/

static INT PrintEnvDirCommand (INT argc, char **argv)
{
  ENVDIR *currentDir;

  NO_OPTION_CHECK(argc,argv);

  /* load current directory */
  currentDir = ChangeEnvDir(userPath);
  if (currentDir==NULL)
  {
    /* directory is invalid */
    strcpy(userPath,DIRSEP);
    currentDir = ChangeEnvDir(userPath);
    if (currentDir == NULL)
      return (CMDERRORCODE);
  }

  GetPathName(userPath);
  UserWrite(userPath);
  UserWrite("\n");

  return(OKCODE);
}

/****************************************************************************/
/*D
   envinfo - print total size and used memory

   DESCRIPTION:
   This command prints total size and used memory of the emvironment to shell.

   'envinfo'

   KEYWORDS:
   environment, size, heap, memory
   D*/
/****************************************************************************/

static INT EnvInfoCommand (INT argc, char **argv)
{
  NO_OPTION_CHECK(argc,argv);

  EnvHeapInfo(buffer);

  UserWrite(buffer);

  return (OKCODE);
}

/****************************************************************************/
/*D
   set - set (or print) a string variable struct

   DESCRIPTION:
   This command sets (or prints) a string variable struct.
   If it is not existing it is also created.
   It sets or prints the contents of a struct or struct directory.

   'set {<struct> <value>} | {[<structdir> | <struct>] [$r]}'

   .  <struct>~<value>                          - assign <value> (just a string of arbitrary length) to <struct>
   .  [<structdir>|<struct>]~[$r]  - display contents of <struct> or <structdir>
   .n                                (default: current struct dir)
   .  $r                                                - specifies the directory, its contents is listed recursively

   KEYWORDS:
   variable, create, set, assign, value, struct, show, display, print

   SEE ALSO:
   'structpath'
   D*/
/****************************************************************************/

static INT SetCommand (INT argc, char **argv)
{
  char name[LONGSTRSIZE], *namePtr;
  INT flag,ropt,i,res,rv;
        #ifdef ModelP
  /* allocate buffer for definition of long string variables */
  char *buffer;

  /* alloc command buffer */
  if ((buffer=(char *)malloc(cmdintbufsize))==NULL)
  {
    PrintErrorMessage('F',"SetCommand","could not allocate buffer");
    return(__LINE__);
  }

  /* if there are problems with expandfmt print format string !
          printf("SetCommand(): formatstring=%s\n",
                  expandfmt(CONCAT3(" set %",LONGSTRLENSTR,"[0-9:.a-zA-Z_] %16384[]\t\n -~]")));
   */
  res = sscanf(argv[0],expandfmt(CONCAT3(" set %",LONGSTRLENSTR,"[0-9:.a-zA-Z_] %16384[]\t\n -~]")),name,buffer);
        #ifdef __CC__
  res = sscanf(argv[0],expandfmt(CONCAT3(" set %",LONGSTRLENSTR,"[0-9:.a-zA-Z_] %16384[^]\t\n -~]")),name,buffer);
        #endif
        #else
  res = sscanf(argv[0],expandfmt(CONCAT3(" set %",LONGSTRLENSTR,"[0-9:.a-zA-Z_] %255[ -~]")),name,buffer);
        #endif /* ModelP */

  /* check options */
  ropt = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'r' :
      if (res>1)
      {
        PrintHelp("set",HELPITEM," (the r option applies not with setting a value)");
        return (PARAMERRORCODE);
      }
      ropt = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("set",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  /* print or set struct */
  switch (res)
  {
  case 1 :
    namePtr=&(name[0]);
    do
    {
      rv=PrintStructContents(namePtr,buffer,BUFFERSIZE,ropt);
      if ((rv!=0)&&(rv!=4))
      {
        PrintErrorMessage('E',"set","structure not found or bad structure");
        return (CMDERRORCODE);
      }
      UserWrite(buffer);
      namePtr=NULL;
    }
    while (rv==4);
    break;

  case 2 :
    rv =  SetStringVar(name,buffer);
    if (rv!=0)
    {
      PrintErrorMessage('E',"set","could not allocate variable");
      return (CMDERRORCODE);
    }

    break;

  default :
    flag=1;
    do
    {
      rv=PrintCurrentStructContents(flag,buffer,BUFFERSIZE,ropt);
      if ((rv!=0)&&(rv!=4))
      {
        PrintErrorMessage('E',"set","structure not found or bad structure");
        return (CMDERRORCODE);
      }
      UserWrite(buffer);
      flag=0;
    }
    while (rv==4);
    break;
  }

        #ifdef ModelP
  free(buffer);
        #endif

  if (rv==0)
    return (OKCODE);
  else
    return (CMDERRORCODE);
}

/****************************************************************************/
/*D
   dv - delete an existing string variable

   DESCRIPTION:
   This command deletes an existing string variable from the environment.

   dv <variable name>

   .  <variable name>    - <variable name> consists of a complete path related to the
                        current struct dir or the structure root directory in the environment

   KEYWORDS:
   variable, remove, delete

   SEE ALSO:
   def, structpath, dv
   D*/
/****************************************************************************/

static INT DeleteVariableCommand (INT argc, char **argv)
{
  INT res;
  char name[LONGSTRSIZE];

  NO_OPTION_CHECK(argc,argv);

  res = sscanf(argv[0],expandfmt(CONCAT3(" dv %",LONGSTRLENSTR,"[0-9:.a-zA-Z_]")),name);

  if (res!=1)
  {
    PrintHelp("dv",HELPITEM," (could not read name of variable)");
    return(PARAMERRORCODE);
  }

  if (argc!=1)
  {
    PrintHelp("dv",HELPITEM,NULL);
    return(PARAMERRORCODE);
  }

  if (DeleteVariable(name)!=0)
  {
    PrintErrorMessage('E',"dv","could not delete variable");
    return (CMDERRORCODE);
  }
  else
    return(DONE);
}

/****************************************************************************/
/*D
   ms  - create a structure

   DESCRIPTION:
   This commands creates a new string variable struct.
   It calls the function 'MakeStruct'.

   'ms <structdir>'

   .  <structdir>     - the <structdir> consists of a complete path related to the
                     current struct dir or the string variable root in the environment

   KEYWORDS:
   variable, create, struct

   SEE ALSO:
   'structpath'
   D*/
/****************************************************************************/

static INT MakeStructCommand (INT argc, char **argv)
{
  INT res;
  char name[LONGSTRSIZE];

  NO_OPTION_CHECK(argc,argv);

  res = sscanf(argv[0],expandfmt(CONCAT3(" ms %",LONGSTRLENSTR,"[0-9:.a-zA-Z_]")),name);

  if (res!=1)
  {
    PrintHelp("ms",HELPITEM," (could not read name of struct)");
    return(PARAMERRORCODE);
  }

  if (MakeStruct(name)!=0)
    return (CMDERRORCODE);
  else
    return (OKCODE);
}

/****************************************************************************/
/*D
   cs  - change to a struct directory

   DESCRIPTION:
   This commands changes to a struct directory.
   It calls the function 'ChangeStructDir'.

   'cs <structdir>'

   .  <structdir>   - the <structdir> consists of a complete path related to the
                   current struct dir or the string variable root in the environment

   KEYWORDS:
   variable, struct, change
   D*/
/****************************************************************************/

static INT ChangeStructCommand (INT argc, char **argv)
{
  char *s;
  INT i;

  NO_OPTION_CHECK(argc,argv);

  /* strip ' '*cs' '* */
  s = strchr(argv[0],'c');
  strcpy(buffer,s);
  i = 2;
  while ((buffer[i]!='\0') && (strchr(WHITESPACE,buffer[i])!=NULL)) i++;
  s = buffer+i;

  /* s is now the pathname */
  if (ChangeStructDir(s)==NULL)
  {
    PrintErrorMessage('E',"cs","invalid path as argument");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}

/****************************************************************************/
/*D
   pws  - prints the current struct path

   DESCRIPTION:
   This commands calls the function 'GetStructPathName' and
   prints the result to the shell.

   'pws'

   KEYWORDS:
   variable, print, display, show, struct
   D*/
/****************************************************************************/

static INT PrintWorkStructCommand (INT argc, char **argv)
{
  char structPath[1024];

  NO_OPTION_CHECK(argc,argv);

  GetStructPathName(structPath, 1024);
  UserWrite(structPath);
  UserWrite("\n");

  return(OKCODE);
}

/****************************************************************************/
/*D
   ds  - deletes a struct

   DESCRIPTION:
   This commands calls the function 'DeleteStruct' to remove a struct.

   'ds <structdir>'

   .  <structdir>   - the <structdir> consists of a complete path related to the
   .n                 current struct dir or the string variable root in the environment

   KEYWORDS:
   variable, delete, remove, struct

   SEE ALSO:
   dv, structpath

   D*/
/****************************************************************************/

static INT DeleteStructCommand (INT argc, char **argv)
{
  INT res;
  char name[LONGSTRSIZE];

  NO_OPTION_CHECK(argc,argv);

  res = sscanf(argv[0],expandfmt(CONCAT3(" ds %",LONGSTRLENSTR,"[0-9:.a-zA-Z_]")),name);

  if (res!=1)
  {
    PrintHelp("ds",HELPITEM," (could not read name of struct)");
    return(PARAMERRORCODE);
  }

  if (argc!=1)
  {
    PrintHelp("ds",HELPITEM,NULL);
    return(PARAMERRORCODE);
  }

  if (DeleteStruct(name)!=0)
  {
    PrintErrorMessage('E',"ds","could not delete structure");
    return (CMDERRORCODE);
  }
  else
    return(DONE);
}

/****************************************************************************/
/*D
   protocol - print strings to the protocol file

   DESCRIPTION:
   This command prints strings to protocol file.
   It writes formatted output to the open protocol file.

   'protocol {$i[ ]<verbatim text> | $n[ ]<verbatim text> | $t[ ]<verbatim text> | $f}*'

   .   $%i   - append <verbatim text> to protocol file
   .   $%n   - write a line feed and append <verbatim text> to protocol file
   .   $%t   - write a tab and append <verbatim text> to protocol file
   .n          NOTE: the first space (if there) following the option character is skipped
   .   $%f   - flush the file buffer

   EXAMPLE:
   .vb
   x = exp(1);
   protoOn exp.proto;
   protocol $%i the value of exp(1) is $%t @x;
   protocol $%n you can use $s in protocol;
   protoOff
   .ve

   Then, the file 'exp.proto' will consist of the string
   .vb
   "the value of exp(1) is\t2.7182818\nyou can use $s in protocol"
   .ve

   KEYWORDS:
   protocol, file, output, format

   SEE ALSO:
   'protoOn', 'protoOff'
   D*/
/****************************************************************************/

#define PROTOCOL_SEP '%'

static INT ProtocolCommand (INT argc, char **argv)
{
  INT i,from;

        #ifdef ModelP
  if (me != master) return(OKCODE);
        #endif

  if (protocolFile==NULL)
  {
    PrintErrorMessage('E',"protocol","no protocol file open!");
    return (CMDERRORCODE);
  }

  for (i=1; i<argc; i++)
  {
    if (argv[i][0]!=PROTOCOL_SEP)
    {
      PrintErrorMessage('E',"protocol","protocol options have to begin with %");
      return (PARAMERRORCODE);
    }
    from = (argv[i][2]==' ') ? 3 : 2;
    switch (argv[i][1])
    {
    case 'i' :
      fprintf(protocolFile,"%s",(argv[i])+from);
      break;

    case 't' :
      fprintf(protocolFile,"\t%s",(argv[i])+from);
      break;

    case 'n' :
      fprintf(protocolFile,"\n%s",(argv[i])+from);
      break;

    case 'f' :
      fflush(protocolFile);
      continue;

    default :
      sprintf(buffer," (unknown option '%s')",argv[i]);
      PrintHelp("protocol",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }
    /* write options not followed by a \ */
    while ((i+1<argc) && (argv[i+1][0]!=PROTOCOL_SEP))
      fprintf(protocolFile," $%s",(argv[++i]));
  }

  return (OKCODE);
}

/****************************************************************************/
/*D
   OpenProto - Open protocol file where specially formatted output is saved

   SYNOPSIS:
   static INT OpenProto (char *name, INT mode);

   PARAMETERS:
   .  name - name of the protocol file
   .  mode - APPEND_PROTO, RENAME_PROTO, TRYRENAME_PROTO, or NORENAME_PROTO

   DESCRIPTION:
   This function opens protocol file where specially formatted output is saved.
   It opens a protocol file for specially formatted output to file.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
   D*/
/****************************************************************************/

static INT OpenProto (char *name, INT mode)
{
  char realname[MAXPATHLENGTH],fullname[MAXPATHLENGTH],*pos;
  INT pathlen;
  char c;

  pathlen = 0;
  if (GetDefaultValue(DEFAULTSFILENAME,"protocoldir",fullname)==0)
  {
    pathlen = strlen(fullname);
    strcat(fullname,name);
  }
  else
    strcpy(fullname,name);

  if (protocolFile!=NULL)
  {
    fclose(protocolFile);
    protocolFile = NULL;
    PrintErrorMessage('W',"OpenProto","open protocol file closed!!\n");
  }

  if (mode==APPEND_PROTO)
  {
    protocolFile = fileopen(fullname,"a");
    if (protocolFile==NULL)
      return (1);
    else
      return (0);
  }

  strcpy(realname,fullname);

  if ((mode==RENAME_PROTO)||(mode==TRYRENAME_PROTO))
  {
    /* while file with realname exists */
    c = 'a';
    while ((protocolFile=fileopen(realname,"r"))!=NULL)
    {
      fclose(protocolFile);
      protocolFile = NULL;

      if (c<=MAXRENAMECHAR)
      {
        /* try new name */
        strcpy(realname,fullname);
        if (strchr(name,'.')!=NULL)
        {
          if ((pos=strrchr(realname,'.'))!=NULL)
          {
            /* place a char before .ext in name.ext */
            *pos++ = c++;
            *pos = '\0';
            pos = strrchr(fullname,'.');
            strcat(realname,pos);
          }
        }
        else
        {
          /* place a char after name */
          pos = realname + strlen(realname);
          *pos++ = c++;
          *pos = '\0';
        }
      }
      else if (mode==RENAME_PROTO)
      {
        PrintErrorMessageF('E',"OpenProto","could't find a new name for '%s'",fullname);
        return (1);
      }
      else
        break;
    }
  }

  protocolFile = fileopen(realname,"w");
  if (protocolFile==NULL)
    return (1);

  SetStringVar(":protofilename",realname+pathlen);

  if (strcmp(realname+pathlen,name)!=0)
  {
    PrintErrorMessageF('W',"OpenProto","opened protcol file '%s' (instead of '%s')",realname+pathlen,name);
  }

  return(0);
}

/****************************************************************************/
/*D
   protoOn - open protocol file where specially formatted output is saved

   DESCRIPTION:
   This command opens protocol file where specially formatted output is saved.

   'protoOn <filename> [$r[!] | $a]'

   .   <filename>  - name of the protocol file
   .    $r!        - if a file named <filename> exist already, rename it to <filename>.saved
   .n                break if the renaming fails
   .   $r          - like above but proceed even if renaming fails
   .   $a          - append to existing file named <filename>

   KEYWORDS:
   protocol, file, open, output, format

   SEE ALSO:
   'protoOff', 'protocol'
   D*/
/****************************************************************************/

static INT ProtoOnCommand (INT argc, char **argv)
{
  static char protoFileName[NAMESIZE];
  INT res,i,RenameMode;

        #ifdef ModelP
  if (me != master) return(OKCODE);
        #endif

  /* get document name */
  protoFileName[0] = '\0';
  res = sscanf(argv[0],expandfmt(CONCAT3(" protoOn %",NAMELENSTR,"[ -~]")),protoFileName);
  if (res!=1)
  {
    PrintHelp("protoOn",HELPITEM," (filename not found)");
    return (PARAMERRORCODE);
  }

  /* check options */
  RenameMode = NORENAME_PROTO;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      if (RenameMode!=NORENAME_PROTO)
      {
        PrintErrorMessage('E',"protoOn","specify either $r or $a");
        return (PARAMERRORCODE);
      }
      RenameMode = APPEND_PROTO;
      break;

    case 'r' :
      if (RenameMode!=NORENAME_PROTO)
      {
        PrintErrorMessage('E',"protoOn","specify either $r or $a");
        return (PARAMERRORCODE);
      }
      if (argv[i][1]=='!')
        RenameMode = RENAME_PROTO;
      else
        RenameMode = TRYRENAME_PROTO;
      break;

    default :
      sprintf(buffer," (unknown option '%s')",argv[i]);
      PrintHelp("protoOn",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (OpenProto(protoFileName,RenameMode)>0)
  {
    PrintErrorMessage('E',"protoOn","could not open protocol file");
    return(CMDERRORCODE);
  }

  return(OKCODE);
}

/****************************************************************************/
/*D
   protoOff - close protocol file

   DESCRIPTION:
   This command closes the protocol file.

   'protoOff'

   KEYWORDS:
   protocol, file, close, output, format

   SEE ALSO:
   'protoOn', 'protocol'
   D*/
/****************************************************************************/

static INT ProtoOffCommand (INT argc, char **argv)
{
        #ifdef ModelP
  if (me != master) return(OKCODE);
        #endif

  NO_OPTION_CHECK(argc,argv);

  if (protocolFile==NULL)
  {
    PrintErrorMessage('E',"protoOff","no protocol file open");
    return(PARAMERRORCODE);
  }

  fclose(protocolFile);

  protocolFile = NULL;

  return(OKCODE);
}

/****************************************************************************/
/*D
   GetProtocolFile - return pointer to current protocol file

   SYNOPSIS:
   FILE *GetProtocolFile (void)

   PARAMETERS:
   .  void - none

   DESCRIPTION:
   This function returns a pointer to the current protocol file (NULL if not open).

   RETURN VALUE:
   FILE *
   .n    file ptr if ok
   .n    NULL if no protocol file open
   D*/
/****************************************************************************/

FILE *GetProtocolFile (void)
{
  return (protocolFile);
}

/****************************************************************************/
/*D
   logon - open log file where all shell output is saved

   DESCRIPTION:
   This command opens a log file where all shell output is saved.

   'logon <logfilename> [$p] [$e] [$a] [$f] [$r [0|1]]'

   .   <filename>  - name of logfile
   .   $p			- write log to open protocol file
   .   $e			- extend filename with numbers of processors (parallel only)
   .   $a			- extend filename with processor id (parallel only)
   .   $f			- force (close open logfile iff)
   .   $r~[0|1]	- 0: replace (default), 1: rename existing log file (using modification date)

   KEYWORDS:
   protocol, file, open, output

   SEE ALSO:
   'logoff'
   D*/
/****************************************************************************/

static INT LogOnCommand (INT argc, char **argv)
{
  char logfile[NAMESIZE];
  INT i,rv,popt,res,pext,meext,rename;
  int ropt;

  /* check options */
  popt = pext = meext = rename = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'p' :
      if (protocolFile==NULL)
      {
        PrintErrorMessage('E',"logon","no protocol file open");
        return (PARAMERRORCODE);
      }
      popt = TRUE;
      break;

    case 'e' :
                                #ifdef ModelP
      pext = TRUE;
                                #endif
      break;

    case 'a' :
                                #ifdef ModelP
      meext = TRUE;
                                #endif
      break;

    case 'f' :
      CloseLogFile();
      break;

    case 'r' :
      res = sscanf(argv[i]," r %d",&ropt);
      if (res==0 || (res==1 && ropt==1)) rename = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("logon",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (popt)
  {
    SetLogFile(protocolFile);
    WriteLogFile("\nbeginlog\n");
    return (OKCODE);
  }

  /* get logfile name */
  if (sscanf(argv[0],expandfmt(CONCAT3(" logon %",NAMELENSTR,"[ -~]")),logfile)!=1)
  {
    PrintErrorMessage('E',"logon","could not read name of logfile");
    return(PARAMERRORCODE);
  }
        #ifdef ModelP
  if (pext == TRUE)
  {
    sprintf(logfile,"%s.p%04d",logfile,procs);
  }
  if (meext == TRUE)
  {
    sprintf(logfile,"%s.%04d",logfile,me);
  }
  else if (me != master)
    return (OKCODE);
        #endif

  rv = OpenLogFile(logfile,rename);
  switch (rv)
  {
  case 0 :
    return (OKCODE);

  case 1 :
    PrintErrorMessage('E',"logon","logfile already open");
    break;

  case 2 :
    PrintErrorMessage('E',"logon","could not open logfile");
    break;

  default :
    PrintErrorMessage('E',"logon","(unknown)");
  }

  return(CMDERRORCODE);
}

/****************************************************************************/
/*D
   logoff - close logfile

   DESCRIPTION:
   This command closes the logfile.

   'logoff $p'

   .  $p - logging is detached from open protocol file

   KEYWORDS:
   protocol, file, close, output

   SEE ALSO:
   'logon'
   D*/
/****************************************************************************/

static INT LogOffCommand (INT argc, char **argv)
{
  INT i,popt;

  /* check options */
  popt = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'p' :
      if (protocolFile==NULL)
      {
        PrintErrorMessage('E',"logoff","no protocol file open");
        return (PARAMERRORCODE);
      }
      popt = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("logon",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (popt)
  {
    WriteLogFile("\nendlog\n");
    SetLogFile(NULL);
    return (OKCODE);
  }

  if (CloseLogFile()!=0)
    PrintErrorMessage('W',"logoff","no logfile open");

  return(OKCODE);
}

#ifdef __TWODIM__

/****************************************************************************/
/*
   cnom - write a cnom output file

   DESCRIPTION:
   This function writes data in a format suitable for the program cnom 2.0
   written by Susanne Kroemker of the IWR, Heidelberg.

   'cnom ...'

   KEYWORDS:
   file, open, output, data
 */
/****************************************************************************/

static INT CnomCommand (INT argc, char **argv)
{
  char docName[32],plotprocName[NAMESIZE],tagName[NAMESIZE];
  int i,flag;

  if (currMG==NULL)
  {
    PrintErrorMessage('E',"cnom","no multigrid active");
    return(CMDERRORCODE);
  }

  /* get document name */
  docName[0] = (char) 0;
  sscanf(argv[0]," cnom %31[ -~]",docName);
  if (strlen(docName)==0)
  {
    PrintErrorMessage('E',"cnom","no document name");
    return(PARAMERRORCODE);
  }
  if (argc!=2)
  {
    PrintErrorMessage('E',"cnom","specify only one argument with cnom");
    PrintHelp("cnom",HELPITEM,buffer);
    return(PARAMERRORCODE);
  }

  flag=0;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'p' :
      if (sscanf(argv[i],expandfmt(CONCAT3("p %",NAMELENSTR,"[ -~]")),plotprocName)!=1)
      {
        PrintErrorMessage('E',"cnom","can't read plotprocName");
        return(PARAMERRORCODE);
      }
      flag=flag|1;
      break;
    case 't' :
      if (sscanf(argv[i],expandfmt(CONCAT3("t %",NAMELENSTR,"[ -~]")),tagName)!=1)
      {
        PrintErrorMessage('E',"cnom","can't read tagName");
        return(PARAMERRORCODE);
      }
      flag=flag|2;
      break;
    default :
      flag=flag|4;
      break;
    }

  if (flag!=3)
  {
    PrintHelp("cnom",HELPITEM,buffer);
    return (PARAMERRORCODE);
  }


  return(SaveCnomGridAndValues(currMG,docName,plotprocName,tagName));
}
#endif

/****************************************************************************/
/*D
   configure - configure a BVP

   DESCRIPTION:
   This command configures the BPV, calling BVP_Configure.
   The arguments depend on the domain module.

   'configure <BVP name> ...'

   EXAMPLE:
   'configure test $d Quadrilateral $P 2 1.1 1.3'

   In the 2D standard domain module, the BVP test will be coupled with
   a quadrilateral with corners (0,0), (1,0), (1.1,1.3) and (0,1).

   KEYWORDS:
   boundary value problem, change
   D*/
/****************************************************************************/

static INT ConfigureCommand (INT argc, char **argv)
{
  BVP *theBVP;
  BVP_DESC theBVPDesc;
  char BVPName[NAMESIZE];

  /* get BVP name */
  if ((sscanf(argv[0],expandfmt(CONCAT3(" configure %",NAMELENSTR,"[ -~]")),BVPName)!=1) || (strlen(BVPName)==0))
  {
    PrintHelp("configure",HELPITEM," (cannot read BndValProblem specification)");
    return(PARAMERRORCODE);
  }

  theBVP = BVP_GetByName(BVPName);
  if (theBVP == NULL)
  {
    PrintHelp("configure",HELPITEM," (cannot read BndValProblem specification)");
    return(PARAMERRORCODE);
  }

  if (BVP_SetBVPDesc(theBVP,&theBVPDesc))
    return (CMDERRORCODE);

  if (BVPD_CONFIG(&theBVPDesc)!=NULL)
    if ((*BVPD_CONFIG(&theBVPDesc))(argc,argv))
    {
      PrintErrorMessage('E',"configure"," (could not configure BVP)");
      return(CMDERRORCODE);
    }

  return(OKCODE);
}

/****************************************************************************/
/*D
   close - close current multigrid

   DESCRIPTION:
   This command closes the current (or all) open multigrid(s),
   frees their heaps and closes all the pictures belonging to them,
   calling 'DisposeMultiGrid' and 'DisposePicture'.

   'close [$a]'

   .   $a  - close all multigrids

   KEYWORDS:
   multigrid, close
   D*/
/****************************************************************************/

static INT CloseCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  UGWINDOW *theWin;
  PICTURE *thePic,*NextPic,*currPic;
  INT i,closeonlyfirst;

  if (ResetPrintingFormat())
    REP_ERR_RETURN(CMDERRORCODE);

  closeonlyfirst = TRUE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      closeonlyfirst = FALSE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("close",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  i = 0;
  do
  {
    /* get multigrid */
    theMG = currMG;
    if (theMG==NULL)
    {
      if (i==0)
      {
        PrintErrorMessage('W',"close","no open multigrid");
        return (OKCODE);
      }
      closeonlyfirst = FALSE;
      break;
    }

    currPic = GetCurrentPicture();

    /* close pictures first */
    for (theWin=GetFirstUgWindow(); theWin!=NULL; theWin=GetNextUgWindow(theWin))
      for (thePic=GetFirstPicture(theWin); thePic!=NULL; thePic=NextPic)
      {
        NextPic = GetNextPicture(thePic);

        if (PIC_MG(thePic)==theMG)
        {
          if (thePic==currPic)
            SetCurrentPicture(NULL);
          if (DisposePicture(thePic)!=0)
          {
            PrintErrorMessage('E',"closewindow","could not close a picture of that window");
            return (CMDERRORCODE);
          }
        }
      }

    if (DisposeMultiGrid(theMG)!=0)
    {
      PrintErrorMessage('E',"close","closing the mg failed");
      return (CMDERRORCODE);
    }
    i++;

    currMG = GetFirstMultigrid();
  }
  while (!closeonlyfirst);

  return(OKCODE);
}

/****************************************************************************/
/*D
   new - allocate a new multigrid

   DESCRIPTION:
   This command allocates a new multigrid, using the function 'CreateMultiGrid'.
   It allocates heap and a new multigrid structure.
   The specification of the boundary value problem must be supplied by
   the user with the functions 'CreateProblem' and 'CreateDomain'.
   It also creates the corner vertices and nodes of the domain.

   'new [<mgname>] $b <boundary value problem> $f <format> $h <heapsize> [$n] [$e]'

   .  <mgname>                          - the name of the multigrid (default is 'untitled-<nb>')
   .  $b~<boundary~value~problem>	- a boundary value problem
   .  $f~<format>                       - one of the enroled formats matching with <boundary value problem>
   .  $n							- do not use optimezed insert element function
   .  $e							- empty grid (no vertices, no elements)
   .  $h~<heapsize>                     - the heapsize to be allocated in byte (or use suffix
                                              "K" for kilobyte, "M" for megabyte, "G" for gigabyte)

   EXAMPLES:
   'new $p TestProblem $f nc $h 30000000;'

   'new $b TestProblem $f nc $h 30000K;'

   'new $b TestProblem $f nc $h 30M;'

   KEYWORDS:
   multigrid, new, create
   D*/
/****************************************************************************/

static INT NewCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char Multigrid[NAMESIZE],BVPName[NAMESIZE],Format[NAMESIZE];
  MEM heapSize;
  INT i,bopt,fopt,hopt,IEopt,emptyGrid;

  /* get multigrid name */
  if ((sscanf(argv[0],expandfmt(CONCAT3(" new %",NAMELENSTR,"[ -~]")),Multigrid)!=1) || (strlen(Multigrid)==0))
    sprintf(Multigrid,"untitled-%d",(int)untitledCounter++);

  theMG = GetMultigrid(Multigrid);
  if ((theMG != NULL) && (theMG == currMG)) CloseCommand(0,NULL);

  /* get problem, domain and format */
  heapSize = 0;
  bopt = fopt = hopt = FALSE;
  IEopt = TRUE;
  emptyGrid = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'b' :
      if (sscanf(argv[i],expandfmt(CONCAT3("b %",NAMELENSTR,"[ -~]")),BVPName)!=1)
      {
        PrintHelp("new",HELPITEM," (cannot read BndValProblem specification)");
        return(PARAMERRORCODE);
      }
      bopt = TRUE;
      break;

    case 'f' :
      if (sscanf(argv[i],expandfmt(CONCAT3("f %",NAMELENSTR,"[ -~]")),Format)!=1)
      {
        PrintHelp("new",HELPITEM," (cannot read format specification)");
        return(PARAMERRORCODE);
      }
      fopt = TRUE;
      break;

    case 'n' :
      IEopt = FALSE;
      break;

    case 'e' :
      emptyGrid = TRUE;
      break;

    case 'h' :
      if (ReadMemSizeFromString(argv[i]+1,&heapSize)!=0)                           /* skip leading 'h' in argv */
      {
        PrintHelp("new",HELPITEM," (cannot read heapsize specification)");
        return(PARAMERRORCODE);
      }
      hopt = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("new",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (!(bopt && fopt && hopt))
  {
    PrintHelp("new",HELPITEM," (the d, p, f and h arguments are mandatory)");
    return(PARAMERRORCODE);
  }

  /* allocate the multigrid structure */
  theMG = CreateMultiGrid(Multigrid,BVPName,Format,heapSize,IEopt,!emptyGrid);
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"new","could not create multigrid");
    return(CMDERRORCODE);
  }

  currMG = theMG;

  return(OKCODE);
}

/****************************************************************************/
/*D
   open - load a new multigrid from a data file

   DESCRIPTION:
   This command loads a new multigrid, using the function 'LoadMultiGrid'.
   Usually, this file should be generated by the 'save' command.
   It allocates the heap and a new multigrid structure.
   The specification of the problem and the domain must be supplied by
   the user with the functions 'CreateProblem' and 'CreateDomain'.

   'open <filename> [$t <type>] [$m <mg name>] [$b <problem>] [$f <format>] [$h <heapsize>]'

   .  <filename>                        - the name of the multigrid file (the fule name will be composed
                                                                        to: <filename>.ug.mg.<type>
   .  $t~<type>					- file was saved with type: asc (default) or bin
   .  <mg~name>					- grid will be created with this name
   .  $p~<boundary~value~problem>	- a boundary value problem
                                                                        (overrides saved one)
   .  $f~<format>                       - one of the enroled formats matching with <boundary value problem>
                                                                        (overrides saved one)
   .  $h~<heapsize>                     - the heapsize to be allocated
                                                                        (overrides saved one)

   KEYWORDS:
   multigrid, new, open, file

   SEE ALSO:
   'new', 'save'
   D*/
/****************************************************************************/

static INT OpenCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char Multigrid[NAMESIZE],File[NAMESIZE],BVPName[NAMESIZE],Format[NAMESIZE],type[NAMESIZE];
  char *theBVP,*theFormat,*theMGName;
  MEM heapSize;
  INT i,force,IEopt,autosave,try_load,fqn,mgpathes_set_old;

  /* get multigrid name */
  if ((sscanf(argv[0],expandfmt(CONCAT3(" open %",NAMELENSTR,"[ -~]")),File)!=1) || (strlen(File)==0))
  {
    PrintErrorMessage('E',"open","specify the name of the file to open");
    return (PARAMERRORCODE);
  }

  /* get problem and format */
  strcpy(type,"asc");
  theBVP = theFormat = theMGName = NULL;
  heapSize = force = autosave = fqn = 0;
  try_load = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      autosave = 1;
      break;

    case 'b' :
      if (sscanf(argv[i],expandfmt(CONCAT3("b %",NAMELENSTR,"[ -~]")),BVPName)!=1)
      {
        PrintHelp("open",HELPITEM," (cannot read BndValProblem specification)");
        return(PARAMERRORCODE);
      }
      theBVP = BVPName;
      break;

    case 'f' :
      if (sscanf(argv[i],expandfmt(CONCAT3("f %",NAMELENSTR,"[ -~]")),Format)!=1)
      {
        PrintHelp("open",HELPITEM," (cannot read format specification)");
        return(PARAMERRORCODE);
      }
      theFormat = Format;
      break;

    case 'F' :
      force = 1;
      break;

    case 'n' :
      IEopt = FALSE;
      break;

    case 'm' :
      if (sscanf(argv[i],expandfmt(CONCAT3("m %",NAMELENSTR,"[ -~]")),Multigrid)!=1)
      {
        PrintHelp("open",HELPITEM," (cannot read multigrid specification)");
        return(PARAMERRORCODE);
      }
      theMGName = Multigrid;
      break;

    case 't' :
      if (strncmp(argv[i],"try",3)==0)
      {
        try_load = TRUE;
        break;
      }
      if (sscanf(argv[i],expandfmt(CONCAT3("t %",NAMELENSTR,"[ -~]")),type)!=1)
      {
        PrintHelp("open",HELPITEM," (cannot read type specification)");
        return(PARAMERRORCODE);
      }
      break;

    case 'h' :
      if (ReadMemSizeFromString(argv[i]+1,&heapSize)!=0)                           /* skip leading 'h' in argv */
      {
        PrintHelp("open",HELPITEM," (cannot read heapsize specification)");
        return(PARAMERRORCODE);
      }
      break;

    case 'z' :
      fqn = 1;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("open",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (fqn) {
    mgpathes_set_old = mgpathes_set;
    mgpathes_set = 0;
  }

  /* allocate the multigrid structure */
  theMG = LoadMultiGrid(theMGName,File,type,theBVP,theFormat,
                        heapSize,force,IEopt,autosave);

  if (fqn) mgpathes_set = mgpathes_set_old;

  if (theMG==NULL)
  {
    PrintErrorMessage('E',"open","could not open multigrid");
    if (try_load)
      return(CMDERRORCODE);
    else
      RETURN(CMDERRORCODE);
  }
  currMG = theMG;

  return(OKCODE);
}

/****************************************************************************/
/*D
   save - save a multigrid structure in a file

   DESCRIPTION:
   This command writes the current multigrid structure in a file.

   'save [<name>] [$t <type>] [$r [0|1]] [$c <comment>]'

   .  <name>                  - name to save with (default is the mgname)
   .n								if name is ending in .scr a script file is saved which
                                                                will generate the surface of the grid as level 0 on execution
   .  $t~<type>			   - type can be asc (default> or bin. asc and bin can be opened with
                                                                the open command
   .  $c~<comment>            - optionally specify a comment string
   .  $r~[0|1]                - 0: replace (default), 1: rename existing mg file (using modification date)

   KEYWORDS:
   multigrid, save, write, data, file, output

   SEE ALSO:
   'open'
   D*/
/****************************************************************************/

static INT SaveCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char Name[NAMESIZE],type[NAMESIZE],Comment[LONGSTRSIZE];
  INT i,autosave,rename,res;
  int ropt;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"save","no open multigrid");
    return (CMDERRORCODE);
  }

  /* scan name */
  if (sscanf(argv[0],expandfmt(CONCAT3(" save %",NAMELENSTR,"[ -~]")),Name)!=1)
    strcpy(Name,ENVITEM_NAME(theMG));

  /* check options */
  autosave=rename=0;
  strcpy(Comment,NO_COMMENT);
  strcpy(type,"asc");
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'c' :
      if (sscanf(argv[i],expandfmt(CONCAT3(" c %",LONGSTRLENSTR,"[ -~]")),Comment)!=1)
      {
        PrintErrorMessage('E',"save","couldn't read the comment string");
        return (PARAMERRORCODE);
      }
      break;

    case 't' :
      if (sscanf(argv[i],expandfmt(CONCAT3("t %",NAMELENSTR,"[ -~]")),type)!=1)
      {
        PrintHelp("open",HELPITEM," (cannot read type specification)");
        return(PARAMERRORCODE);
      }
      break;

    case 'a' :
      autosave=1;
      break;

    case 'r' :
      res = sscanf(argv[i]," r %d",&ropt);
      if (res==0 || (res==1 && ropt==1)) rename = 1;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("save",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (SaveMultiGrid(theMG,Name,type,Comment,autosave,rename)) return (CMDERRORCODE);

  return(OKCODE);
}

/****************************************************************************/
/*D
   savedomain - save domain structure in a file

   DESCRIPTION:
   This command saves the domain structure of the current multigrid in a file.
   All arguments are passed to the current domain module interface function.

   'savedomain ...'

   SEE ALSO:
   'open'

   KEYWORDS:
   multigrid, domain, save, write, data, file, output
   D*/
/****************************************************************************/

static INT SaveDomainCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char Name[NAMESIZE];

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"savedomain","no open multigrid");
    return (CMDERRORCODE);
  }

  /* scan name */
  if (sscanf(argv[0],expandfmt(CONCAT3(" savedomain %",NAMELENSTR,"[ -~]")),Name)!=1)
    strcpy(Name,BVPD_NAME(MG_BVPD(theMG)));

  if (BVP_Save(MG_BVP(theMG),Name,ENVITEM_NAME(theMG),MGHEAP(theMG),argc,argv)) return (CMDERRORCODE);

  return(OKCODE);
}

/****************************************************************************/
/*D
   average, freeaverage - average result of a plot proc to nodal vector

   DESCRIPTION:
   The average command takes a plot procedure (scalar or vector), allocates
   a vec data desc with nodal components and computes a conforming
   approximation of the function via a box-volume weighted average.
   The vec data desc will be given the same name as the plot procedure. The
   freeaverage command is called with same arguments and deallocates the vec data
   descs. The vec data descs are not locked!

   'average {$ns <scalar plot proc> [$s <sym>] | $nv <vector plot proc> [$s <sym>]}*'
   'freeaverage {$ns <scalar plot proc> [$s <sym>] | $nv <vector plot proc> [$s <sym>]}*'

   EXAMPLE:
   .vb
   average $nv uwTrans $s sol $nv unTrans $s sol;
   savedata air3d $t xdr $n @step $T @TIME $a sol $b uwTrans $c unTrans;
   freeaverage $nv uwTrans $s sol $nv unTrans $s sol;
   .ve


   KEYWORDS:
   file, output

   D*/
/****************************************************************************/

#ifdef ModelP
static INT comm_comp, comm_box_comp;

static int Gather_Scalar (DDD_OBJ obj, void *data)
{
  NODE *pv = (NODE *)obj;
  DOUBLE *d = (DOUBLE *)data;

  d[0] = VVALUE(NVECTOR(pv),comm_comp);
  d[1] = VVALUE(NVECTOR(pv),comm_box_comp);

  return (NUM_OK);
}

static int Scatter_Scalar (DDD_OBJ obj, void *data)
{
  NODE *pv = (NODE *)obj;
  DOUBLE *d = (DOUBLE *)data;

  VVALUE(NVECTOR(pv),comm_comp) += d[0];
  VVALUE(NVECTOR(pv),comm_box_comp) += d[1];

  return (NUM_OK);
}

INT l_sum_scalar (GRID *g, INT comp, INT box_comp)
{
  comm_comp = comp; comm_box_comp = box_comp;
  DDD_IFAExchange(BorderNodeSymmIF, GRID_ATTR(g), 2*sizeof(DOUBLE),
                  Gather_Scalar, Scatter_Scalar);
  return (NUM_OK);
}

static int Gather_Vector (DDD_OBJ obj, void *data)
{
  NODE *pv = (NODE *)obj;
  DOUBLE *d = (DOUBLE *)data;
  INT i;

  for (i=0; i<DIM; i++)
    d[i] = VVALUE(NVECTOR(pv),comm_comp+i);
  d[DIM] = VVALUE(NVECTOR(pv),comm_box_comp);

  return (NUM_OK);
}

static int Scatter_Vector (DDD_OBJ obj, void *data)
{
  NODE *pv = (NODE *)obj;
  DOUBLE *d = (DOUBLE *)data;
  INT i;

  for (i=0; i<DIM; i++)
    VVALUE(NVECTOR(pv),comm_comp+i) += d[i];
  VVALUE(NVECTOR(pv),comm_box_comp) += d[DIM];

  return (NUM_OK);
}

INT l_sum_vector (GRID *g, INT comp, INT box_comp)
{
  comm_comp = comp; comm_box_comp = box_comp;
  DDD_IFAExchange(BorderNodeSymmIF, GRID_ATTR(g), (DIM+1)*sizeof(DOUBLE),
                  Gather_Vector, Scatter_Vector);
  return (NUM_OK);
}
#endif


INT AverageScalar (MULTIGRID *mg, EVALUES *eval, char *eval_name, VECDATA_DESC *vecdesc)
{
  INT box_compo,compo,n,k,i,error,j;
  GRID *g;
  NODE *theNode;
  VECTOR *theVector;
  SHORT NCmpInType[NVECTYPES];
  VECDATA_DESC *box_vd=NULL;
  PreprocessingProcPtr preProc;
  ElementEvalProcPtr evalProc;
  ELEMENT *el;
  DOUBLE value;
  DOUBLE *CornersCoord[MAX_CORNERS_OF_ELEM];
  DOUBLE LocalCoord[DIM];
  DOUBLE local[DIM];
  FVElementGeometry Geo;
  SubControlVolume *scv;

  /* get component */
  compo = VD_ncmp_cmpptr_of_otype(vecdesc,NODEVEC,&n)[0];
  assert(n>0);

  /* reset average to zero */
  for (k=0; k<=TOPLEVEL(mg); k++)
  {
    g = GRID_ON_LEVEL(mg,k);
    for (theNode=FIRSTNODE(g); theNode!= NULL; theNode=SUCCN(theNode))
    {
      theVector = NVECTOR(theNode);
      VVALUE(theVector,compo) = 0.0;
    }
  }

  /* allocate vecdata desc for box volume */
  for (k=0; k<NVECTYPES; k++) NCmpInType[k]=0;
  NCmpInType[NODEVEC]=1;       /* one component in node */
  if (AllocVDfromNCmp(mg,0,TOPLEVEL(mg),NCmpInType,NULL,&box_vd)) return(1);

  /* get component of box volume */
  box_compo = VD_ncmp_cmpptr_of_otype(box_vd,NODEVEC,&n)[0];

  /* reset box volume to zero */
  for (k=0; k<=TOPLEVEL(mg); k++)
  {
    g = GRID_ON_LEVEL(mg,k);
    for (theNode=FIRSTNODE(g); theNode!= NULL; theNode=SUCCN(theNode))
    {
      theVector = NVECTOR(theNode);
      VVALUE(theVector,box_compo) = 0.0;
    }
  }

  /* prepare plot function */
  preProc = eval->PreprocessProc;
  if (preProc!=NULL) preProc(eval_name,mg);
  evalProc = eval->EvalProc;

  /* run through all levels, elements, corners, ... */
  for (k=0; k<=TOPLEVEL(mg); k++)
    for (el=FIRSTELEMENT(GRID_ON_LEVEL(mg,k)); el!=NULL; el=SUCCE(el))
    {
      /* get control volume geometry */
      error = EvaluateFVGeometry(el,&Geo);

      /* process all corners of element */
      for (i=0; i<CORNERS_OF_ELEM(el); i++)
      {
        /* sub control volume */
        scv = Geo.scv+i;

        /* get value in corner i */
        for (j=0; j<CORNERS_OF_ELEM(el); j++)
          CornersCoord[j] = CVECT(MYVERTEX(CORNER(el,j)));                       /* x,y,z of corners */
        LocalCornerCoordinates(DIM,TAG(el),i,local);
        for (j=0; j<DIM; j++) LocalCoord[j] = local[j];
        value = evalProc(el,(const DOUBLE **)CornersCoord,LocalCoord);

        /* accumulate value weighted with box volume */
        theVector = NVECTOR(CORNER(el,i));
        VVALUE(theVector,compo) += scv->volume*value;

        /* ... and accumulate box volume */
        VVALUE(theVector,box_compo) += scv->volume;
      }
    }

  /* if it is parallel, then sum values at interfaces */
        #ifdef ModelP
  for (k=0; k<=TOPLEVEL(mg); k++)
    l_sum_scalar(GRID_ON_LEVEL(mg,k),compo,box_compo);
        #endif

  /* finally, divide by box volume */
  for (k=0; k<=TOPLEVEL(mg); k++)
  {
    g = GRID_ON_LEVEL(mg,k);
    for (theNode=FIRSTNODE(g); theNode!= NULL; theNode=SUCCN(theNode))
    {
      theVector = NVECTOR(theNode);
      VVALUE(theVector,compo) =
        VVALUE(theVector,compo)/VVALUE(theVector,box_compo);
    }
  }

  /* and free box volume */
  FreeVD(mg,0,TOPLEVEL(mg),box_vd);

  return(0);
}


INT AverageVector (MULTIGRID *mg, EVECTOR *eval, char *eval_name, VECDATA_DESC *vecdesc)
{
  INT box_compo,compo,n,k,i,error,j;
  GRID *g;
  NODE *theNode;
  VECTOR *theVector;
  SHORT NCmpInType[NVECTYPES];
  VECDATA_DESC *box_vd=NULL;
  PreprocessingProcPtr preProc;
  ElementVectorProcPtr evalProc;
  ELEMENT *el;
  DOUBLE value[DIM];
  DOUBLE *CornersCoord[MAX_CORNERS_OF_ELEM];
  DOUBLE LocalCoord[DIM];
  DOUBLE local[DIM];
  FVElementGeometry Geo;
  SubControlVolume *scv;

  /* get component */
  compo = VD_ncmp_cmpptr_of_otype(vecdesc,NODEVEC,&n)[0];
  assert(n==DIM);
  for (j=1; j<DIM; j++)
    if (VD_ncmp_cmpptr_of_otype(vecdesc,NODEVEC,&n)[j]!=compo+j)
    {
      UserWrite("can only handle consecutive components!\n");
      return(1);
    }

  /* reset average to zero */
  for (k=0; k<=TOPLEVEL(mg); k++)
  {
    g = GRID_ON_LEVEL(mg,k);
    for (theNode=FIRSTNODE(g); theNode!= NULL; theNode=SUCCN(theNode))
    {
      theVector = NVECTOR(theNode);
      for (j=0; j<DIM; j++) VVALUE(theVector,compo+j) = 0.0;
    }
  }

  /* allocate vecdata desc for box volume */
  for (k=0; k<NVECTYPES; k++) NCmpInType[k]=0;
  NCmpInType[NODEVEC]=1;       /* one component in node */
  if (AllocVDfromNCmp(mg,0,TOPLEVEL(mg),NCmpInType,NULL,&box_vd)) return(1);

  /* get component of box volume */
  box_compo = VD_ncmp_cmpptr_of_otype(box_vd,NODEVEC,&n)[0];

  /* reset box volume to zero */
  for (k=0; k<=TOPLEVEL(mg); k++)
  {
    g = GRID_ON_LEVEL(mg,k);
    for (theNode=FIRSTNODE(g); theNode!= NULL; theNode=SUCCN(theNode))
    {
      theVector = NVECTOR(theNode);
      VVALUE(theVector,box_compo) = 0.0;
    }
  }

  /* prepare plot function */
  preProc = eval->PreprocessProc;
  if (preProc!=NULL) preProc(eval_name,mg);
  evalProc = eval->EvalProc;

  /* run through all levels, elements, corners, ... */
  for (k=0; k<=TOPLEVEL(mg); k++)
    for (el=FIRSTELEMENT(GRID_ON_LEVEL(mg,k)); el!=NULL; el=SUCCE(el))
    {
      /* get control volume geometry */
      error = EvaluateFVGeometry(el,&Geo);

      /* process all corners of element */
      for (i=0; i<CORNERS_OF_ELEM(el); i++)
      {
        /* sub control volume */
        scv = Geo.scv+i;

        /* get value in corner i */
        for (j=0; j<CORNERS_OF_ELEM(el); j++)
          CornersCoord[j] = CVECT(MYVERTEX(CORNER(el,j)));                       /* x,y,z of corners */
        LocalCornerCoordinates(DIM,TAG(el),i,local);
        for (j=0; j<DIM; j++) LocalCoord[j] = local[j];
        evalProc(el,(const DOUBLE **)CornersCoord,LocalCoord,value);

        /* accumulate value weighted with box volume */
        theVector = NVECTOR(CORNER(el,i));
        for (j=0; j<DIM; j++) VVALUE(theVector,compo+j) += scv->volume*value[j];

        /* ... and accumulate box volume */
        VVALUE(theVector,box_compo) += scv->volume;
      }
    }

  /* if it is parallel, then sum values at interfaces */
        #ifdef ModelP
  for (k=0; k<=TOPLEVEL(mg); k++)
    l_sum_vector(GRID_ON_LEVEL(mg,k),compo,box_compo);
        #endif

  /* finally, divide by box volume */
  for (k=0; k<=TOPLEVEL(mg); k++)
  {
    g = GRID_ON_LEVEL(mg,k);
    for (theNode=FIRSTNODE(g); theNode!= NULL; theNode=SUCCN(theNode))
    {
      theVector = NVECTOR(theNode);
      for (j=0; j<DIM; j++)
        VVALUE(theVector,compo+j) =
          VVALUE(theVector,compo+j)/VVALUE(theVector,box_compo);
    }
  }

  /* and free box volume */
  FreeVD(mg,0,TOPLEVEL(mg),box_vd);

  return(0);
}

#define MAXVARIABLES    10

static INT AverageCommand (INT argc, char **argv)
{
  MULTIGRID *mg;
  INT ns;
  INT nv;
  EVALUES *es[MAXVARIABLES];
  char es_name[MAXVARIABLES][NAMESIZE];
  EVECTOR *ev[MAXVARIABLES];
  char ev_name[MAXVARIABLES][NAMESIZE];
  char s[NAMESIZE];
  VECDATA_DESC *vd,*vd2;
  SHORT NCmpInType[NVECTYPES];
  INT i,k;

  /* get current multigrid */
  mg = GetCurrentMultigrid();
  if (mg==NULL)
  {
    PrintErrorMessage('W',"average","no multigrid open\n");
    return (OKCODE);
  }

  /* scan input for $ns <scalar eval proc> [$s <symbol>] or $nv <scalar eval proc> [$s <symbol>]*/
  ns=nv=0;       /* no plot functions yet */
  for(i=1; i<argc; i++)
  {
    if (strncmp(argv[i],"ns",2)==0) {
      if (ns>=MAXVARIABLES)
      {
        PrintErrorMessage('E',"average:","too many scalar variables specified\n");
        break;
      }
      sscanf(argv[i],"ns %s", s);
      es[ns] = GetElementValueEvalProc(s);
      if (es[ns]==NULL)
      {
        PrintErrorMessageF('E',"average:","could not find scalar eval proc %s\n",s);
        break;
      }
      if (sscanf(argv[i+1],"s %s", s) == 1)
      {
        strcpy(es_name[ns],s);
        i++;
      }
      else
        strcpy(es_name[ns],es[ns]->v.name);
      ns++;
      continue;
    }

    if (strncmp(argv[i],"nv",2)==0) {
      if (nv>=MAXVARIABLES)
      {
        PrintErrorMessage('E',"average:","too many vector variables specified\n");
        break;
      }
      sscanf(argv[i],"nv %s", s);
      ev[nv] = GetElementVectorEvalProc(s);
      if (ev[nv]==NULL)
      {
        PrintErrorMessageF('E',"average:","could not find vector eval proc %s\n",s);
        break;
      }
      if (sscanf(argv[i+1],"s %s", s) == 1)
      {
        strcpy(ev_name[nv],s);
        i++;
      }
      else
        strcpy(ev_name[nv],ev[nv]->v.name);
      nv++;
      continue;
    }

  }

  /* scalar functions with one component in node .. */
  for (k=0; k<NVECTYPES; k++) NCmpInType[k]=0;
  NCmpInType[NODEVEC]=1;
  for (i=0; i<ns; i++)
  {
    /* allocate free VECDATA_DESC */
    vd=NULL;
    if (AllocVDfromNCmp(mg,0,TOPLEVEL(mg),NCmpInType,NULL,&vd)) return(1);

    /* if name exist then it must be that of vd */
    vd2=GetVecDataDescByName(mg,es[i]->v.name);
    if (vd2!=vd && vd2!=NULL)
    {
      UserWrite(es[i]->v.name); UserWrite(": name exists already, skipping\n");
      FreeVD(mg,0,TOPLEVEL(mg),vd);
      return(1);
      continue;
    }

    /* rename vd to name of plot proc */
    strcpy(vd->v.name,es[i]->v.name);
    UserWrite(es[i]->v.name); UserWrite(": created\n");

    /* ... and average it */
    if (AverageScalar(mg,es[i],es_name[i],vd)) return(1);
  }

  /* vector functions with DIM components in node .. */
  for (k=0; k<NVECTYPES; k++) NCmpInType[k]=0;
  NCmpInType[NODEVEC]=DIM;
  for (i=0; i<nv; i++)
  {
    /* allocate free VECDATA_DESC */
    vd=NULL;
    if (AllocVDfromNCmp(mg,0,TOPLEVEL(mg),NCmpInType,NULL,&vd)) return(1);

    /* if name exist then it must be that of vd */
    vd2=GetVecDataDescByName(mg,ev[i]->v.name);
    if (vd2!=vd && vd2!=NULL)
    {
      UserWrite(ev[i]->v.name); UserWrite(": name exists already, skipping\n");
      FreeVD(mg,0,TOPLEVEL(mg),vd);
      return(1);
      continue;
    }

    /* rename vd to name of plot proc */
    strcpy(vd->v.name,ev[i]->v.name);
    UserWrite(ev[i]->v.name); UserWrite(": created\n");

    /* ... and average it */
    if (AverageVector(mg,ev[i],ev_name[i],vd)) return(1);
  }

  return(0);
}

static INT FreeAverageCommand (INT argc, char **argv)
{
  MULTIGRID *mg;
  INT ns;
  INT nv;
  EVALUES *es[MAXVARIABLES];
  char es_name[MAXVARIABLES][NAMESIZE];
  EVECTOR *ev[MAXVARIABLES];
  char ev_name[MAXVARIABLES][NAMESIZE];
  char s[NAMESIZE];
  VECDATA_DESC *vd;
  INT i;

  /* get current multigrid */
  mg = GetCurrentMultigrid();
  if (mg==NULL)
  {
    PrintErrorMessage('W',"average","no multigrid open\n");
    return (OKCODE);
  }

  /* scan input for $ns <scalar eval proc> [$s <symbol>] or $nv <scalar eval proc> [$s <symbol>]*/
  ns=nv=0;       /* no plot functions yet */
  for(i=1; i<argc; i++)
  {
    if (strncmp(argv[i],"ns",2)==0) {
      if (ns>=MAXVARIABLES)
      {
        PrintErrorMessage('E',"freeaverage:","too many scalar variables specified\n");
        break;
      }
      sscanf(argv[i],"ns %s", s);
      es[ns] = GetElementValueEvalProc(s);
      if (es[ns]==NULL)
      {
        PrintErrorMessageF('E',"freeaverage:","could not find scalar eval proc %s\n",s);
        break;
      }
      if (sscanf(argv[i+1],"s %s", s) == 1)
      {
        strcpy(es_name[ns],s);
        i++;
      }
      else
        strcpy(es_name[ns],es[ns]->v.name);

      /* get VECDATA_DESC */
      vd = GetVecDataDescByName(mg,es[ns]->v.name);
      if (vd==NULL)
      {
        UserWrite(es[ns]->v.name); UserWrite(": VECDATA_DESC not found\n");
        continue;
      }

      /* free it */
      FreeVD(mg,0,TOPLEVEL(mg),vd);
      UserWrite(vd->v.name); UserWrite(": freed\n");

      ns++;
      continue;
    }

    if (strncmp(argv[i],"nv",2)==0) {
      if (nv>=MAXVARIABLES)
      {
        PrintErrorMessage('E',"freeaverage:","too many vector variables specified\n");
        break;
      }
      sscanf(argv[i],"nv %s", s);
      ev[nv] = GetElementVectorEvalProc(s);
      if (ev[nv]==NULL)
      {
        PrintErrorMessageF('E',"freeaverage:","could not find vector eval proc %s\n",s);
        break;
      }
      if (sscanf(argv[i+1],"s %s", s) == 1)
      {
        strcpy(ev_name[nv],s);
        i++;
      }
      else
        strcpy(ev_name[nv],ev[nv]->v.name);

      /* get VECDATA_DESC */
      vd = GetVecDataDescByName(mg,ev[nv]->v.name);
      if (vd==NULL)
      {
        UserWrite(ev[nv]->v.name); UserWrite(": VECDATA_DESC not found\n");
        continue;
      }

      /* free it */
      FreeVD(mg,0,TOPLEVEL(mg),vd);
      UserWrite(vd->v.name); UserWrite(": freed\n");

      nv++;
      continue;
    }

  }

  return(0);
}

/****************************************************************************/
/*D
   savedata - save multigrid data in a file

   DESCRIPTION:
   This function saves multigrid data from the current multigrid in a file.
   The multigrid has to be saved before.

   'savedata <filename> [$t <type>] [$n <number>] [$T <time>] [$r [0|1]] [$a <vd name> [$b <vd name>[$c <vd name>[$d <vd name>[$e <vd name>]]]]]'

   .  <filename>		- the filename will be composed to <filename>.ug.data.<type>
   .  $t~<type>		- type can be asc (default) or bin
   .  $n~<number>		- picture number for movie
   .  $T~<time>		- assign this time level
   .  $a~<vd name>...	- read data from this vec data descriptors
   .  $r~[0|1]			- 0: replace (default), 1: rename existing data file (using modification date)

   KEYWORDS:
   multigrid, save, write, data, file, output

   SEE ALSO:
   'save'
   D*/
/****************************************************************************/

static INT ReadSaveDataInput (MULTIGRID *theMG, INT argc, char **argv, char *VDSym, char EvalChar, VECDATA_DESC **theVD, EVALUES **theEVal, EVECTOR **theEVec)
{
  INT i;

  /* init */
  *theVD = NULL; *theEVal = NULL; *theEVec = NULL;

  /* read VecDesc */
  for (i=1; i<argc; i++)
    if (argv[i][0]==VDSym[0])
    {
      if (sscanf(argv[i]+1," %s",buffer)!=1) break;
      if (strlen(buffer)>=NAMESIZE) break;
      *theVD = GetVecDataDescByName(theMG,buffer);
      if (*theVD!=NULL) return (1);
    }

  /* get plot procedure */
  for (i=1; i<argc; i++)
    if (argv[i][0]==EvalChar)
    {
      if (sscanf(argv[i]+1," %s",buffer)!=1) break;
      if (strlen(buffer)>=NAMESIZE) break;
      *theEVal = GetElementValueEvalProc(buffer);
      if (*theEVal!=NULL) return (2);
      *theEVec = GetElementVectorEvalProc(buffer);
      if (*theEVec!=NULL) return (3);
    }

  return (0);
}

#define NM_MAX          100

static INT SaveDataCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char FileName[NAMESIZE],type[NAMESIZE],mname[NAMESIZE];
  VECDATA_DESC *theVDList[NM_MAX];
  char NameList[5][NAMESIZE];
  char **Names;
  char *NamePtr[5];
  EVALUES *theEValues[5];
  EVECTOR *theEVector[5];
  INT i,j,n,ret,number,rename,nm;
  int iValue;
  double Value[3];
  DOUBLE t[3];

  theMG = currMG;
  if (theMG==NULL) {PrintErrorMessage('E',"savedata","no open multigrid"); return (CMDERRORCODE);}

  /* scan filename */
  if (sscanf(argv[0],expandfmt(CONCAT3(" savedata %",NAMELENSTR,"[ -~]")),FileName)!=1) { PrintErrorMessage('E',"save","cannot read filename"); return (CMDERRORCODE);}

  strcpy(type,"asc");
  number = -1;
  t[0]=t[1]=t[2]=-1.0; nm=0;
  rename=0;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'm' :
      ret=sscanf(argv[i]+1," %s %d",mname,&iValue);
      if (ret!=2)
      {
        PrintHelp("savedata",HELPITEM," (multiple vector specification)");
        return(PARAMERRORCODE);
      }
      nm=iValue;
      if (nm<1 || nm>NM_MAX)
      {
        PrintHelp("savedata",HELPITEM," (multiple vector number out of range [0,xxx])");
        return(PARAMERRORCODE);
      }
      break;
    case 't' :
      if (sscanf(argv[i],expandfmt(CONCAT3("t %",NAMELENSTR,"[ -~]")),type)!=1)
      {
        PrintHelp("savedata",HELPITEM," (cannot read type specification)");
        return(PARAMERRORCODE);
      }
      break;

    case 'n' :
      if (sscanf(argv[i],"n %d",&iValue)!=1)
      {
        PrintHelp("savedata",HELPITEM," (cannot read number specification)");
        return(PARAMERRORCODE);
      }
      number = iValue;
      if (number<0 || number > 9999)
      {
        PrintHelp("savedata",HELPITEM," (number out of range [0,9999])");
        return(PARAMERRORCODE);
      }
      break;

    case 'T' :
      ret = sscanf(argv[i],"T %lf %lf %lf",Value,Value+1,Value+2);
      if (ret<1 || ret>3)
      {
        PrintHelp("savedata",HELPITEM," (cannot read TIME specification)");
        return(PARAMERRORCODE);
      }
      for (j=0; j<ret; j++)
        t[j] = Value[j];
      if (t[0]<0.0)
      {
        PrintHelp("savedata",HELPITEM," (TIME out of range ]-inf, 0.0[)");
        return(PARAMERRORCODE);
      }
      break;
    case 'r' :
      ret = sscanf(argv[i]," r %d",&iValue);
      if (ret==0 || (ret==1 && iValue==1)) rename = 1;
      break;
    }
  if (((t[0]<0.0) && number>=0) || ((t[0]>=0.0) && number<0))
  {
    PrintHelp("savedata",HELPITEM," (specify both or none the options 'n' and 'T')");
    return(PARAMERRORCODE);
  }

  /* get input */
  n=0;
  if (nm<=0)
  {
    ret = ReadSaveDataInput (theMG,argc,argv,"a",'A',theVDList+0,theEValues+0,theEVector+0);        if (ret) n++;
    ret = ReadSaveDataInput (theMG,argc,argv,"b",'B',theVDList+1,theEValues+1,theEVector+1);        if (ret) n++;
    ret = ReadSaveDataInput (theMG,argc,argv,"c",'C',theVDList+2,theEValues+2,theEVector+2);        if (ret) n++;
    ret = ReadSaveDataInput (theMG,argc,argv,"d",'D',theVDList+3,theEValues+3,theEVector+3);        if (ret) n++;
    ret = ReadSaveDataInput (theMG,argc,argv,"e",'E',theVDList+4,theEValues+4,theEVector+4);        if (ret) n++;
  }
  else
  {
    for (i=0; i<nm; i++)
    {
      sprintf(buffer,"%s%d",mname,i);
      theVDList[i]=GetVecDataDescByName(theMG,buffer);
      if (theVDList[i]==NULL) return (PARAMERRORCODE);
    }
    n=nm;
  }

  /* read names */
  Names=NULL;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'N' :
      if (sscanf(argv[i],"N %s %s %s %s %s",NameList[0],NameList[1],NameList[2],NameList[3],NameList[4])!=n) return (PARAMERRORCODE);
      Names=NamePtr;
      for (j=0; j<5; j++) NamePtr[j]=NameList[j];
      break;
    }

  if (n<=0) return (PARAMERRORCODE);
  if (SaveData(theMG,FileName,rename,type,number,t[0],t[1],t[2],n,theVDList,theEValues,theEVector,Names)) return (PARAMERRORCODE);

  return(OKCODE);
}

/****************************************************************************/
/*D
   loaddata - load multigrid data from a file

   DESCRIPTION:
   This function loads multigrid data from a file.

   'loaddata <filename> [$t <type>] [$n <number>] [$f [$h <heapsize>]] [$a <vd name> [$b <vd name>[$c <vd name>[$d <vd name>[$e <vd name>]]]]]'

   .  <filename>		- the filename will be composed to <filename>.ug.data.<type>
   .  $t~<type>		- type can be asc (default) or bin
   .  $n~<number>		- picture number of movie
   .  $f				- open or reopen multigrid corresponding to data file
   .  $h~<heapsize>    - the heapsize to be allocated
                                                                        (overrides saved one)
   .  $a~<vd name>...	- save data to this vec data descriptors

   KEYWORDS:
   multigrid, load, read, file, data

   SEE ALSO:
   'save'
   D*/
/****************************************************************************/

static INT LoadDataCommand (INT argc, char **argv)
{
  char FileName[NAMESIZE],type[NAMESIZE],mname[NAMESIZE];
  VECDATA_DESC *theVDList[NM_MAX];
  INT i,m,n,number,force,ret,nm,fqn,datapathes_set_old;
  int iValue;
  MEM heapSize;

  /* scan filename */
  if (sscanf(argv[0],expandfmt(CONCAT3(" loaddata %",NAMELENSTR,"[ -~]")),FileName)!=1) { PrintErrorMessage('E',"save","cannot read filename"); return (CMDERRORCODE);}

  strcpy(type,"asc");
  heapSize=0;
  force=0;
  fqn=0;
  number = -1;
  nm=0;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'm' :
      ret=sscanf(argv[i]+1," %s %d",mname,&iValue);
      if (ret!=2)
      {
        PrintHelp("savedata",HELPITEM," (multiple vector specification)");
        return(PARAMERRORCODE);
      }
      nm=iValue;
      if (nm<1 || nm>NM_MAX)
      {
        PrintHelp("savedata",HELPITEM," (multiple vector number out of range [0,xxx])");
        return(PARAMERRORCODE);
      }
      break;

    case 't' :
      if (sscanf(argv[i],expandfmt(CONCAT3("t %",NAMELENSTR,"[ -~]")),type)!=1)
      {
        PrintHelp("loaddata",HELPITEM," (cannot read type specification)");
        return(PARAMERRORCODE);
      }
      break;

    case 'f' :
      force=1;
      break;

    case 'n' :
      if (sscanf(argv[i],"n %d",&iValue)!=1)
      {
        PrintHelp("loaddata",HELPITEM," (cannot read number specification)");
        return(PARAMERRORCODE);
      }
      number = iValue;
      if (number<0 || number > 999999)
      {
        PrintHelp("loaddata",HELPITEM," (number out of range [0,999999])");
        return(PARAMERRORCODE);
      }
      break;

    case 'h' :
      if (ReadMemSizeFromString(argv[i]+1,&heapSize)!=0)                           /* skip leading 'h' in argv */
      {
        PrintHelp("new",HELPITEM," (cannot read heapsize specification)");
        return(PARAMERRORCODE);
      }
      break;
    case 'z' :
      fqn = 1;
      break;
    }

  if (fqn) {
    datapathes_set_old = datapathes_set;
    datapathes_set = 0;
  }

  /* open or reopen multigrid */
  if (force)
  {
    currMG = OpenMGFromDataFile(currMG,number,type,FileName,heapSize);
    if (currMG==NULL)
    {
      PrintErrorMessage('E',"loaddata","cannot open multigrid");
      return (CMDERRORCODE);
    }
  }
  if (currMG==NULL) {PrintErrorMessage('E',"loaddata","no open multigrid"); return (CMDERRORCODE);}

  /* get vecdatadesc */
  n=0;
  if (nm==0)
  {
    theVDList[n++]=ReadArgvVecDesc(currMG,"a",argc,argv);
    theVDList[n++]=ReadArgvVecDesc(currMG,"b",argc,argv);
    theVDList[n++]=ReadArgvVecDesc(currMG,"c",argc,argv);
    theVDList[n++]=ReadArgvVecDesc(currMG,"d",argc,argv);
    theVDList[n++]=ReadArgvVecDesc(currMG,"e",argc,argv);
  }
  else
  {
    for (i=0; i<nm; i++)
    {
      sprintf(buffer,"%s%d",mname,i);
      theVDList[i]=GetVecDataDescByName(currMG,buffer);
      if (theVDList[i]==NULL)
      {
        theVDList[i]=CreateVecDescOfTemplate (currMG,buffer,NULL);
        if (theVDList[i]==NULL) return (CMDERRORCODE);
      }
    }
    n=nm;
  }
  m=0;
  for (i=0; i<n; i++)
    if (theVDList[i]!=NULL)
      m = i+1;

  if (m<=0) return (PARAMERRORCODE);
  if (LoadData(currMG,FileName,type,number,m,theVDList)) return (CMDERRORCODE);

  if (fqn) datapathes_set = datapathes_set_old;

  return(OKCODE);
}

/****************************************************************************/
/*D
   changemc - change magic cookie of multigrid

   DESCRIPTION:
   This function changes the magic cookie of multigrid.

   'changemc <mc>'

   .  <mc>				- new magic cookie

   KEYWORDS:
   multigrid, load, read, file, data

   SEE ALSO:
   'save'
   D*/
/****************************************************************************/

static INT ChangeMagicCookieCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  int iValue;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"changemc","no open multigrid");
    return (CMDERRORCODE);
  }

  if (sscanf(argv[0]," changemc %d",&iValue)!=1)
  {
    PrintErrorMessage('E',"changemc","cannot read magic-cookie");
    return (CMDERRORCODE);
  }
  MG_MAGIC_COOKIE(theMG) = iValue;
  return(OKCODE);
}

/****************************************************************************/
/*D
   level - select another current level

   DESCRIPTION:
   This command changes another current level of the current multigrid.

   level <level> | + | -

   .  <level> - go to level <level>
   .  +       - go to the next finer level
   .  -       - go to the next coarser level

   KEYWORDS:
   multigrid, current
   D*/
/****************************************************************************/

static INT LevelCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;

  /* following variables: keep type for sscanf */
  int l;

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"level","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check parameters */
  if (sscanf(argv[0]," level %d",&l)==1)
  {
    if ((l<BOTTOMLEVEL(theMG)) || (l>TOPLEVEL(theMG)))
    {
      PrintErrorMessage('E',"level","level out of range");
      return (PARAMERRORCODE);
    }
    else
      CURRENTLEVEL(theMG) = l;
  }
  else if (strchr(argv[0],'+')!=NULL)
  {
    if (CURRENTLEVEL(theMG)==TOPLEVEL(theMG))
    {
      PrintErrorMessage('W',"level","already on TOPLEVEL");
      return (OKCODE);
    }
    else
      CURRENTLEVEL(theMG)++;
  }
  else if (strchr(argv[0],'-')!=NULL)
  {
    if (CURRENTLEVEL(theMG)==BOTTOMLEVEL(theMG))
    {
      PrintErrorMessage('W',"level","already on BOTTOMLEVEL");
      return (OKCODE);
    }
    else
      CURRENTLEVEL(theMG)--;
  }
  else
  {
    PrintErrorMessage('E',"level","specify <level>, + or - with the level command");
    return (CMDERRORCODE);
  }

  UserWriteF("  current level is %d (bottom level %d, top level %d)\n",
             CURRENTLEVEL(theMG),BOTTOMLEVEL(theMG),TOPLEVEL(theMG));

  InvalidatePicturesOfMG(theMG);
  InvalidateUgWindowsOfMG(theMG);

  return(OKCODE);
}

/****************************************************************************/
/*D
   renumber - reassign the object IDs in the multigrid


   DESCRIPTION:
   This command reassigns the object IDs in the multigrid
   subsequently to fill the gaps, calling the function 'RenumberMultiGrid'.

   'renumber'

   KEYWORDS:
   multigrid, id
   D*/
/****************************************************************************/

static INT RenumberMGCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"renumber","no open multigrid");
    return (CMDERRORCODE);
  }

  if (RenumberMultiGrid(theMG,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0)!=GM_OK)
  {
    PrintErrorMessage('E',"renumber","renumbering of the mg failed");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}

/****************************************************************************/
/*D
   wplist - list information on all windows and pictures

   DESCRIPTION:
   This command lists information on all windows and pictures, calling
   the functions 'ListWindowPictureHeader', 'ListPicture' and
   'ListUGWindow'.

   'wplist'

   KEYWORDS:
   graphics, plot, window, list, display, show
   D*/
/****************************************************************************/

static INT WindowPictureListCommand (INT argc, char **argv)
{
  UGWINDOW *currUgW, *theUgW;
  PICTURE *currPic, *thePic;

  NO_OPTION_CHECK(argc,argv);

  currUgW = GetCurrentUgWindow();
  currPic = GetCurrentPicture();

  ListWindowPictureHeader();
  for (theUgW=GetFirstUgWindow(); theUgW!=NULL; theUgW=GetNextUgWindow(theUgW))
  {
    ListUgWindow(theUgW,(theUgW==currUgW));
    for (thePic=GetFirstPicture(theUgW); thePic!=NULL; thePic=GetNextPicture(thePic))
      ListPicture(thePic,(thePic==currPic));
  }

  return (OKCODE);
}

/****************************************************************************/
/*D
   mglist - list information on all multigrids

   DESCRIPTION:
   This command lists information on all multigrids, calling
   the functions 'ListMultiGridHeader' and 'ListMultiGrid'.

   'mglist [$s]'

   .  $s - short format for less information

   KEYWORDS:
   multigrid, list, display, show
   D*/
/****************************************************************************/

static INT MGListCommand (INT argc, char **argv)
{
  MULTIGRID *theMG,*theCurrMG;
  INT i,longformat;

  theCurrMG = GetCurrentMultigrid();
  if (theCurrMG==NULL)
  {
    PrintErrorMessage('W',"mglist","no multigrid open\n");
    return (OKCODE);
  }

  longformat = TRUE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 's' :
      longformat = FALSE;
      break;

    case 'l' :
      /* old syntax */
      longformat = TRUE;
      break;

    default :
      sprintf(buffer," (unknown option '%s')",argv[i]);
      PrintHelp("mglist",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  ListMultiGridHeader(longformat);

  for (theMG=GetFirstMultigrid(); theMG!=NULL; theMG=GetNextMultigrid(theMG))
    ListMultiGrid(theMG,(theMG==theCurrMG),longformat);

  return (OKCODE);
}

/****************************************************************************/
/*D
   glist - list information on the current multigrid

   DESCRIPTION:
   This command lists information on the current multigrid, calling
   the function 'ListGrids'.

   'glist'

   KEYWORDS:
   multigrid, list, display, show
   D*/
/****************************************************************************/

static INT GListCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: GListCommand(): me not in Context,"\
                     " no listing of grid\n",me))
    return(OKCODE);
  }
        #endif

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    UserWrite("no multigrid open\n");
    return (OKCODE);
  }

  ListGrids(theMG);

  return (OKCODE);
}

/****************************************************************************/
/*D
   nlist - list information on specified nodes

   DESCRIPTION:
   This command lists information on specified nodes, calling
   the functions 'ListNodeRange' and 'ListNodeSelection'.

   'nlist {$s | $i <fromID> [<toID>] | $g <global id> | $k <key>} [$d] [$b] [$n] [$v] [$a]'

   .  $s  - list info for the selected nodes
   .  $i  - list info for nodes with an ID in the range <fromID> through <toID>
         if <fromID> is omitted only the node with <fromID> is listed
   .  $g <glob. id>- list info for node with global id (only for ModelP)
   .  $k <key>     - list info for node with key

   .  $d  - up to version 2.3 ONLY: list also user data space
   .  $b  - print additional info for boundary nodes
   .  $n  - list also neighbours of each node
   .  $v  - print extended info (verbose mode)
   .  $a  - list all nodes

   KEYWORDS:
   multigrid, node, link, list, display, show
   D*/
/****************************************************************************/

static INT NListCommand (INT argc, char **argv)
{

  MULTIGRID *theMG;
  INT i,fromN,toN,res,mode,idopt,dataopt,boundaryopt,neighbouropt,verboseopt;
  char buff[32];

  /* following variables: keep type for sscanf */
  long f,t;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: NListCommand(): me not in Context," \
                     " no listing of nodes\n",me))
    return(OKCODE);
  }
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"nlist","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check options */
  idopt = LV_ID;
  dataopt = boundaryopt = neighbouropt = verboseopt = mode = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'i' :
      if (mode!=FALSE)
      {
        PrintErrorMessage('E',"nlist","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_ID;
      res = sscanf(argv[i]," i %ld %ld",&f,&t);
      fromN = f;
      toN   = t;
      if (res<1)
      {
        PrintErrorMessage('E',"nlist","specify at least one id with the i option");
        return (PARAMERRORCODE);
      }
      else if (res==1)
        toN = fromN;
      else if (fromN>toN)
      {
        PrintErrorMessage('E',"nlist","from ID > to ID");
        return (PARAMERRORCODE);
      }
      break;

#ifdef ModelP
    case 'g' :
      mode = DO_ID;
      idopt = LV_GID;
      res = sscanf(argv[i]," g %s",buff);
      fromN = toN = (DDD_GID) strtol(buff, NULL, 0);
      break;
#endif
    case 'k' :
      mode = DO_ID;
      idopt = LV_KEY;
      res = sscanf(argv[i]," k %s",buff);
      fromN = toN = (INT) strtol(buff, NULL, 0);
      break;

    case 's' :
      if (mode!=FALSE)
      {
        PrintErrorMessage('E',"nlist","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_SELECTION;
      break;

    case 'a' :
      if (mode!=FALSE)
      {
        PrintErrorMessage('E',"nlist","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_ALL;
      break;

    case 'd' :
      dataopt = TRUE;
      break;

    case 'b' :
      boundaryopt = TRUE;
      break;

    case 'n' :
      neighbouropt = TRUE;
      break;

    case 'v' :
      verboseopt = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("nlist",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  switch (mode)
  {
  case DO_ID :
    ListNodeRange(theMG,fromN,toN,idopt,dataopt,boundaryopt,neighbouropt,verboseopt);
    break;

  case DO_ALL :
    ListNodeRange(theMG,0,MAX_I,idopt,dataopt,boundaryopt,neighbouropt,verboseopt);
    break;

  case DO_SELECTION :
    ListNodeSelection(theMG,dataopt,boundaryopt,neighbouropt,verboseopt);
    break;

  default :
    PrintErrorMessage('E',"nlist","specify either the a, s or i option");
    return (PARAMERRORCODE);
  }

  return(OKCODE);
}

/****************************************************************************/
/*D
   elist - list information on specified elements

   DESCRIPTION:
   This command lists information on specified elements, calling
   the functions 'ListElementRange' and 'ListElementSelection'.

   'elist $s | {$i <fromID> [<toID>]} [$d] [$b] [$n] [$v] [$a]'

   .  $s  - list info for the selected elements
   .  $i  - list info for elements with an ID in the range <fromID> through <toID>
         if <fromID> is omitted only the element with <fromID> is listed
   .  $g <glob. id>- list info for element with global id (only for ModelP)
   .  $k <key>     - list info for element with key

   .  $d  - up to version 2.3 ONLY: list also user data space
   .  $b  - print additional info for boundary elements
   .  $n  - list also neighbours of each element
   .  $v  - print extended info (verbose mode)
   .  $l  - list only elements of current level
   .  $a  - list all elements

   KEYWORDS:
   multigrid, element, list, display, show
   D*/
/****************************************************************************/

static INT EListCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  INT i,fromE,toE,res,mode,idopt,dataopt,boundaryopt,neighbouropt,verboseopt,levelopt;
  char buff[32];

  /* following variables: keep type for sscanf */
  long f,t;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: EListCommand(): me not in Context," \
                     " no listing of elements\n",me))
    return(OKCODE);
  }
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"elist","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check options */
  idopt = LV_ID;
  dataopt = boundaryopt = neighbouropt = verboseopt = levelopt = mode = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'i' :
      if (mode!=FALSE)
      {
        PrintErrorMessage('E',"elist","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_ID;
      res = sscanf(argv[i]," i %ld %ld",&f,&t);
      fromE = f;
      toE   = t;
      if (res<1)
      {
        PrintErrorMessage('E',"elist","specify at least one id with the i option");
        return (PARAMERRORCODE);
      }
      else if (res==1)
        toE = fromE;
      else if (fromE>toE)
      {
        PrintErrorMessage('E',"elist","from ID > to ID");
        return (PARAMERRORCODE);
      }
      break;

#ifdef ModelP
    case 'g' :
      mode = DO_ID;
      idopt = LV_GID;
      res = sscanf(argv[i]," g %s",buff);
      fromE = toE = (DDD_GID) strtol(buff, NULL, 0);
      break;
#endif
    case 'k' :
      mode = DO_ID;
      idopt = LV_KEY;
      res = sscanf(argv[i]," k %s",buff);
      fromE = toE = (INT) strtol(buff, NULL, 0);
      break;

    case 's' :
      if (mode!=FALSE)
      {
        PrintErrorMessage('E',"elist","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_SELECTION;
      break;

    case 'a' :
      if (mode!=FALSE)
      {
        PrintErrorMessage('E',"elist","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_ALL;
      break;

    case 'l' :
      levelopt = TRUE;
      break;

    case 'd' :
      dataopt = TRUE;
      break;

    case 'b' :
      boundaryopt = TRUE;
      break;

    case 'n' :
      neighbouropt = TRUE;
      break;

    case 'v' :
      verboseopt = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("elist",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  switch (mode)
  {
  case DO_ID :
    ListElementRange(theMG,fromE,toE,idopt,dataopt,boundaryopt,neighbouropt,verboseopt,levelopt);
    break;

  case DO_ALL :
    ListElementRange(theMG,0,MAX_I,idopt,dataopt,boundaryopt,neighbouropt,verboseopt,levelopt);
    break;

  case DO_SELECTION :
    ListElementSelection(theMG,dataopt,boundaryopt,neighbouropt,verboseopt);
    break;

  default :
    PrintErrorMessage('E',"elist","specify either the a, s or i option");
    return (PARAMERRORCODE);
  }

  return(OKCODE);
}

/****************************************************************************/
/*D
   slist - list information on all selected nodes or elements

   DESCRIPTION:
   This command lists information on selected nodes or elements, calling
   the functions 'ListNodeSelection', 'ListElementSelection'.
   (Listing of selected vectors is not implemented.)

   'slist [$d] [$b] [$n] [$v]'

   .   $d  - up to version 2.3 ONLY: list also user data space
   .   $b  - print additional info for boundary nodes/elements
   .   $n  - list also neighbours of each node/element
   .   $v  - print extended info (verbose mode)

   KEYWORDS:
   multigrid, selection, list, display, show

   SEE ALSO:
   'select'
   D*/
/****************************************************************************/

static INT SelectionListCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  INT i,dataopt,boundaryopt,neighbouropt,verboseopt;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: SelectionListCommand(): me not in Context,"\
                     " no listing of selection\n",me))
    return(OKCODE);
  }
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"slist","no open multigrid");
    return (CMDERRORCODE);
  }

  if (SELECTIONSIZE(theMG)==0)
  {
    PrintErrorMessage('W',"slist","nothing selected");
    return (OKCODE);
  }

  /* check options */
  dataopt = boundaryopt = neighbouropt = verboseopt = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'd' :
      dataopt = TRUE;
      break;

    case 'b' :
      boundaryopt = TRUE;
      break;

    case 'n' :
      neighbouropt = TRUE;
      break;

    case 'v' :
      verboseopt = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("slist",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  switch (SELECTIONMODE(theMG))
  {
  case elementSelection :
    ListElementSelection(theMG,dataopt,boundaryopt,neighbouropt,verboseopt);
    break;

  case nodeSelection :
    ListNodeSelection(theMG,dataopt,boundaryopt,neighbouropt,verboseopt);
    break;

  case vectorSelection :
    UserWrite("sorry, this service is not available for vector selections\n");
    break;

  default :
    PrintErrorMessage('W',"slist","selectionmode ???");
    return (PARAMERRORCODE);
  }

  return(OKCODE);
}

/****************************************************************************/
/*D
   rlist - list rule records of element type for refinement

   DESCRIPTION:
   This command lists the rule record of a refinement rule for an element type,
   if an integer is given or all records for this element type, if all-option is set.

   'rlist [tri|qua|tet|hex] {[rulenumber] | [$a]}'

   .  $a  - list all rules for element type

   KEYWORDS:
   multigrid, element, rule, type, list, display, show
   D*/
/****************************************************************************/

static INT RuleListCommand (INT argc, char **argv)
{
  INT i,allopt,rn,rv,tag;
  char etype[32];

  rn = -1;
  allopt = FALSE;

  /* check options */
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      allopt = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("rlist",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  /* scan parameters */
  if (allopt == FALSE)
    rv = sscanf(argv[0],"rlist %31[triquatethexa] %d",etype,&rn);
  else
    rv = sscanf(argv[0],"rlist %31[triaquadtetrahexa]",etype);

  tag = -1;
        #ifdef __TWODIM__
  if (strcmp("tri",etype)==0) tag = TRIANGLE;
  if (strcmp("qua",etype)==0) tag = QUADRILATERAL;
        #endif
        #ifdef __THREEDIM__
  if (strcmp("tet",etype)==0) tag = TETRAHEDRON;
  if (strcmp("hex",etype)==0) tag = HEXAHEDRON;
        #endif

  if (tag==-1)
  {
    PrintErrorMessage('E',"rlist","wrong element type");
    return (CMDERRORCODE);
  }

  if (rn==-1 && allopt==FALSE || rn>=0 && allopt==TRUE)
  {
    PrintErrorMessage('E',"rlist","specify rulenumber OR $a option!");
    return (CMDERRORCODE);
  }

  if (allopt == TRUE)
    for (i=0; i<MaxRules[tag]; i++)
      ShowRefRule(tag,i);
  else
    ShowRefRule(tag,rn);

  return(OKCODE);
}

static INT PrintValueCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VECDATA_DESC *theVD;
  double val;
  char name[NAMESIZE];
  char value[VALUELEN];
  int found = FALSE;
  int idx;
  int n;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"printvalue","no open multigrid");
    return (CMDERRORCODE);
  }
  if (sscanf(argv[0],"printvalue %s %d",name,&n)!=2)
  {
    PrintErrorMessage('E',"printvalue","could not scan vec desc and selection number");
    return (PARAMERRORCODE);
  }
  theVD = GetVecDataDescByName(theMG,name);
  if (theVD==NULL)
  {
    PrintErrorMessageF('E',"printvalue","vec desc '%s' not found",name);
    return (PARAMERRORCODE);
  }

  if ((SELECTIONMODE(theMG)==vectorSelection) && (SELECTIONSIZE(theMG)>n))
  {
    VECTOR *vec = (VECTOR *)SELECTIONOBJECT(theMG,n);
    if (VD_ISDEF_IN_TYPE(theVD,VTYPE(vec)))
    {
      val = VVALUE(vec,VD_CMP_OF_TYPE(theVD,VTYPE(vec),0));
      idx = VINDEX(vec);
      found = TRUE;
    }
  }
  if (found)
    sprintf(buffer,"%.10e",val);
  else
    sprintf(buffer,"---");

  UserWriteF("value 0 of %s in vec %d = %s\n",name,idx,buffer);
  if (ReadArgvChar("s",value,argc,argv)==0)
    if (SetStringVar(value,buffer))
    {
      PrintErrorMessageF('E',"printvalue","coul not write onto string var '%s'",value);
      return (PARAMERRORCODE);
    }

  return OKCODE;
}

/****************************************************************************/
/*D
   vmlist - list information on specified vectors and matrices

   DESCRIPTION:
   This command lists information on specified vectors and matrices, calling
   the functions 'ListVectorRange' and 'ListVectorSelection'.

   'vmlist {$s | $i <fromID> [<toID>] | $g <global id> | $k <key>} [$t <dt-list>] [$m] [$d] [$a] [$l <f> <t>] [$pos 0|1] [$obj 0|1]'

   .  $s			- list info for the selected vectors
   .  $i			- list info for vectors with an ID in the range <fromID> through <toID>
                          if <fromID> is omitted only the vector with <fromID> is listed
   .  $g <glob. id>- list info for vector with global id (only for ModelP)
   .  $k <key>     - list info for vector with key
   .  $t~<dt-list>	- list only vectors of specified data types (list of chars, no seperator)

   .  $m			- list also the associated matrix entries
   .  $d			- list also the user data
   .  $skip~0|1	- show skip flags (default 0)
   .  $a			- list all vectors
   .  $l <f> <t>   - process levels f <= l <= t
   .  $pos~0|1		- show vector positions (default 1)
   .  $obj~0|1		- show vector object related info (default 1)

   KEYWORDS:
   multigrid, vector, matrix, userdata, list, display, show
   D*/
/****************************************************************************/

static INT VMListCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  FORMAT *theFormat;
  GRID *theGrid;
  INT i,j,fl,tl,fromV,toV,res,mode,idopt,dataopt,matrixopt,vclass,vnclass,datatypes,modifiers;
  VECDATA_DESC *theVD;
  MATDATA_DESC *theMD;
  char value[VALUELEN];
  char buff[32];
  /* following variables: keep type for sscanf */
  long f,t;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: VMListCommand(): me not in Context," \
                     " no listing of VM\n",me))
    return(OKCODE);
  }
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"vmlist","no open multigrid");
    return (CMDERRORCODE);
  }
  theGrid = GRID_ON_LEVEL(theMG,CURRENTLEVEL(theMG));
  theFormat = MGFORMAT(theMG);

  if (ReadArgvINT("vclass",&vclass,argc,argv))
    vclass = 3;
  if (ReadArgvINT("vnclass",&vnclass,argc,argv))
    vnclass = 3;

  if (ReadArgvChar("vmlist",value,argc,argv) == 0) {
    theVD = GetVecDataDescByName(theMG,value);
    if (theVD != NULL) {
      if (ReadArgvOption("S",argc,argv))
        PrintSVector(theMG,theVD);
            #ifdef __INTERPOLATION_MATRIX__
      else if (ReadArgvOption("I",argc,argv))
        PrintIMatrix(theGrid,theVD,vclass,vnclass);
            #endif
      else if (ReadArgvOption("s",argc,argv))
      {
        /* get selection list */
        if ((SELECTIONMODE(theMG)==vectorSelection) && (SELECTIONSIZE(theMG)>=1))
        {
          VECTOR **vlist = (VECTOR**)malloc((SELECTIONSIZE(theMG)+1)*sizeof(VECTOR*));
          if (vlist!=NULL)
          {
            int i;
            for (i=0; i<SELECTIONSIZE(theMG); i++)
              vlist[i] = (VECTOR *)SELECTIONOBJECT(theMG,i);
            vlist[SELECTIONSIZE(theMG)] = NULL;
            PrintVectorListX((const VECTOR **)vlist,theVD,vclass,vnclass,UserWriteF);
            free(vlist);
          }
        }
      }
      else
        PrintVector(theGrid,theVD,vclass,vnclass);
      return(OKCODE);
    }
    theMD = GetMatDataDescByName(theMG,value);
    if (theMD != NULL) {
      if (ReadArgvOption("T",argc,argv))
        PrintTMatrix(theGrid,theMD,vclass,vnclass);
      else if (ReadArgvOption("D",argc,argv))
        PrintDiagMatrix(theGrid,theMD,vclass,vnclass);
      else
        PrintMatrix(theGrid,theMD,vclass,vnclass);
      return(OKCODE);
    }
  }
  modifiers = LV_MOD_DEFAULT;
  if (ReadArgvINT("skip",&j,argc,argv)==0)
    if (j)
      SET_FLAG(modifiers,LV_SKIP);
    else
      CLEAR_FLAG(modifiers,LV_SKIP);
  if (ReadArgvINT("pos",&j,argc,argv)==0)
    if (j)
      SET_FLAG(modifiers,LV_POS);
    else
      CLEAR_FLAG(modifiers,LV_POS);
  if (ReadArgvINT("obj",&j,argc,argv)==0)
    if (j)
      SET_FLAG(modifiers,LV_VO_INFO);
    else
      CLEAR_FLAG(modifiers,LV_VO_INFO);

  /* check options */
  datatypes = 0;
  idopt = LV_ID;
  dataopt = matrixopt = mode = FALSE;
  fl = tl = CURRENTLEVEL(theMG);
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'i' :
      if (mode!=FALSE)
      {
        PrintErrorMessage('E',"vmlist","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_ID;
      res = sscanf(argv[i]," i %ld %ld",&f,&t);
      fromV = f;
      toV   = t;
      if (res<1)
      {
        PrintErrorMessage('E',"vmlist","specify at least one id with the i option");
        return (PARAMERRORCODE);
      }
      else if (res==1)
        toV = fromV;
      else if (fromV>toV)
      {
        PrintErrorMessage('E',"vmlist","from ID > to ID");
        return (PARAMERRORCODE);
      }
      break;

#ifdef ModelP
    case 'g' :
      mode = DO_ID;
      idopt = LV_GID;
      res = sscanf(argv[i]," g %s",buff);
      fromV = toV = (DDD_GID) strtol(buff, NULL, 0);
      break;
#endif
    case 'k' :
      mode = DO_ID;
      idopt = LV_KEY;
      res = sscanf(argv[i]," k %s",buff);
      fromV = toV = (INT) strtol(buff, NULL, 0);
      break;

    case 's' :
      if (strncmp(argv[i],"skip",4)==0)
        /* handled by ReadArgvINT */
        break;
      if (mode!=FALSE)
      {
        PrintErrorMessage('E',"vmlist","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_SELECTION;
      break;

    case 'a' :
      if (mode!=FALSE)
      {
        PrintErrorMessage('E',"vmlist","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_ALL;
      break;

    case 'l' :
      res = sscanf(argv[i]," l %ld %ld",&f,&t);
      fl = f;
      tl = t;
      if (res!=2)
      {
        PrintErrorMessage('E',"vmlist","specify from and to level with the l option");
        return (PARAMERRORCODE);
      }
      else if (fl>tl)
      {
        PrintErrorMessage('E',"vmlist","from level > to level");
        return (PARAMERRORCODE);
      }
      break;

    case 'd' :
      dataopt = TRUE;
      break;

    case 't' :
      for (j=0; j<NVECTYPES; j++)
        if (FMT_S_VEC_TP(theFormat,j)>0)
          if (strchr(argv[i]+1,FMT_VTYPE_NAME(theFormat,j))!=NULL)
            datatypes |= BITWISE_TYPE(j);
      break;

    case 'm' :
      matrixopt = TRUE;
      break;

    case 'z' :
      matrixopt = -TRUE;
      break;

    case 'p' :
    case 'o' :
      /* handled by ReadArgvINT */
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("vmlist",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }
  if (datatypes==0)
    /* default */
    for (j=0; j<NVECTYPES; j++)
      datatypes |= BITWISE_TYPE(j);

  switch (mode)
  {
  case DO_ID :
    ListVectorRange(theMG,fl,tl,fromV,toV,idopt,matrixopt,dataopt,datatypes,modifiers);
    break;

  case DO_ALL :
    ListVectorRange(theMG,fl,tl,0,MAX_I,idopt,matrixopt,dataopt,datatypes,modifiers);
    break;

  case DO_SELECTION :
    if (SELECTIONMODE(theMG)==elementSelection)
      ListVectorOfElementSelection(theMG,matrixopt,dataopt,modifiers);
    else
      ListVectorSelection(theMG,matrixopt,dataopt,modifiers);
    break;

  default :
    PrintErrorMessage('E',"vmlist","specify either the a, s or i option");
    return (PARAMERRORCODE);
  }

  return(OKCODE);
}

/****************************************************************************/
/*D
   convert - convert a matrix into a sparse format

   DESCRIPTION:
   This command converts a matrix into a sparse format,
   prints it out or svae it to a file.

   `convert <mat> [$p] [$f <name>]`
   .  <mat> - matrix symbol
   .  p - print on shell
   D*/
/****************************************************************************/

static int ReadMatrixDimensions (char *name, int *n, int *na)
{
  int i;

  FILE *stream = fileopen(name,"r");
  if (stream == NULL) return(1);
  fscanf(stream," %d\n",n);
  for (i=0; i<=*n; i++)
    fscanf(stream," %d ",na);
  fclose(stream);

  return(0);
}

static int ReadMatrix (char *name, int n, int *ia, int *ja, double *a)
{
  int i;

  FILE *stream = fileopen(name,"r");
  if (stream == NULL) return(1);
  fscanf(stream," %d\n",&i);
  if (i != n) return(1);
  for (i=0; i<=n; i++)
    fscanf(stream," %d ",ia+i);
  fscanf(stream,"\n");
  for (i=0; i<ia[n]; i++)
    fscanf(stream," %d ",ja+i);
  fscanf(stream,"\n");
  for (i=0; i<ia[n]; i++)
    fscanf(stream," %lf ",a+i);
  fscanf(stream,"\n");
  fclose(stream);

  return(0);
}

static int WriteMatrix (char *name, int n, int *ia, int *ja, double *a)
{
  int i;

  FILE *stream = fileopen(name,"w");
  if (stream == NULL) return(1);
  fprintf(stream," %d\n",n);
  for (i=0; i<=n; i++)
    fprintf(stream," %d ",ia[i]);
  fprintf(stream,"\n");
  for (i=0; i<ia[n]; i++)
    fprintf(stream," %d ",ja[i]);
  fprintf(stream,"\n");
  for (i=0; i<ia[n]; i++)
    fprintf(stream," %f ",a[i]);
  fprintf(stream,"\n");
  fclose(stream);

  return(0);
}

static int WriteMatrixfmt (char *name, int n, int *ia, int *ja, double *a,
                           int inc)
{
  int i;

  FILE *stream = fileopen(name,"w");
  if (stream == NULL) return(1);
  fprintf(stream,"%d %d",n,ia[n]+inc);
  for (i=0; i<=n; i++) {
    if ((i%10) == 0) fprintf(stream,"\n");
    fprintf(stream,"%6d",ia[i]+inc);
  }
  for (i=0; i<ia[n]; i++) {
    if ((i%3) == 0) fprintf(stream,"\n");
    fprintf(stream,"%6d %18.9f",ja[i]+inc,a[i]);
  }
  fprintf(stream,"\n");
  fclose(stream);

  return(0);
}

static INT ConvertCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *theGrid;
  HEAP *theHeap;
  MATDATA_DESC *A;
  int n,nn,*ia,*ja,inc;
  double *a,*r;
  char name[32];
  INT i,j,MarkKey,symmetric,ncomp;

  theMG = GetCurrentMultigrid();
  if (theMG==NULL) {
    PrintErrorMessage('E',"convert","no current multigrid");
    return(CMDERRORCODE);
  }
  theGrid = GRID_ON_LEVEL(theMG,CURRENTLEVEL(theMG));
  if ((A = ReadArgvMatDesc(theMG,"convert",argc,argv))==NULL) {
    PrintErrorMessage('E',"convert","could not read symbol");
    return (PARAMERRORCODE);
  }
  theHeap = MGHEAP(theMG);
  MarkTmpMem(theHeap,&MarkKey);
  symmetric = ReadArgvOption("symmetric",argc,argv);
  inc = ReadArgvOption("inc",argc,argv);
  if (ReadArgvINT("ncomp",&ncomp,argc,argv))
    ncomp = 1;
  if (ReadArgvChar("r",name,argc,argv) == 0) {
    if (ReadMatrixDimensions(name,&n,&nn)) {
      PrintErrorMessage('E',"convert",
                        "could not read matrix dimensions");
      ReleaseTmpMem(MGHEAP(theMG),MarkKey);
      return(CMDERRORCODE);
    }
    ia = (int *)GetTmpMem(theHeap,sizeof(int) * (n+1),MarkKey);
    a = (double *)GetTmpMem(theHeap,sizeof(double) * nn,MarkKey);
    ja = (int *)GetTmpMem(theHeap,sizeof(int) * nn,MarkKey);
    if ((ia == NULL) || (a == NULL) || (ja == NULL)) {
      PrintErrorMessage('E',"convert",
                        "could not allocate memory");
      ReleaseTmpMem(MGHEAP(theMG),MarkKey);
      return(CMDERRORCODE);
    }
    if (ReadMatrix(name,n,ia,ja,a)) {
      PrintErrorMessage('E',"convert","could write matrix");
      ReleaseTmpMem(MGHEAP(theMG),MarkKey);
      return(CMDERRORCODE);
    }
  }
  else if (ConvertMatrix(theGrid,MGHEAP(theMG),MarkKey,A,symmetric,
                         &n,&ia,&ja,&a)) {
    PrintErrorMessage('E',"convert","could not read matrix");
    ReleaseTmpMem(MGHEAP(theMG),MarkKey);
    return(CMDERRORCODE);
  }
  if (ReadArgvChar("f",name,argc,argv) == 0)
    if (ReadArgvOption("fmt",argc,argv)) {
      if (WriteMatrixfmt(name,n,ia,ja,a,inc)) {
        PrintErrorMessage('E',"convert","could write matrix");
        ReleaseTmpMem(MGHEAP(theMG),MarkKey);
        return(CMDERRORCODE);
      }
    }
    else if (WriteMatrix(name,n,ia,ja,a)) {
      PrintErrorMessage('E',"convert","could write matrix");
      ReleaseTmpMem(MGHEAP(theMG),MarkKey);
      return(CMDERRORCODE);
    }
  if (ReadArgvOption("p",argc,argv)) {
    r = (DOUBLE *)GetTmpMem(MGHEAP(theMG),sizeof(DOUBLE) * n,MarkKey);
    for (i=0; i<n; i++) {
      for (j=0; j<n; j++)
        r[j] = 0.0;
      for (j=ia[i]; j<ia[i+1]; j++)
        r[ja[j]] = a[j];
      for (j=0; j<n; j++)
        UserWriteF("%8.4f",r[j]);
      UserWrite("\n");
    }
  }
  ReleaseTmpMem(MGHEAP(theMG),MarkKey);
  return (OKCODE);
}

/****************************************************************************/
/*D
   in - insert an inner node and vertex

   DESCRIPTION:
   This command inserts an inner node and the corresponding vertex
   into a multigrid with only level 0, calling the function
   'InsertInnerNode'.

   'in <x> <y> [<z>]'

   .  <x>~<y>~[<z>] - specify as much coordinates as the space has dimensions

   KEYWORDS:
   multigrid, insert, create, node, edit
   D*/
/****************************************************************************/

static INT InsertInnerNodeCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  DOUBLE xc[DIM];
  INT i;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"in","no open multigrid");
    return (CMDERRORCODE);
  }

  if (sscanf(argv[0],"in %lf %lf %lf",xc,xc+1,xc+2)!=DIM)
  {
    PrintErrorMessageF('E',"in","specify %d coordinates for an inner node",(int)DIM);
    return (PARAMERRORCODE);
  }

  /* NB: toplevel=0 is checked by InsertInnerNode() */
  if (InsertInnerNode(GRID_ON_LEVEL(theMG,0),xc)==NULL)
  {
    PrintErrorMessage('E',"in","inserting an inner node failed");
    return (CMDERRORCODE);
  }

  InvalidatePicturesOfMG(theMG);
  InvalidateUgWindowsOfMG(theMG);

  return (OKCODE);
}

static INT NGInsertInnerNodeCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  DOUBLE xc[DIM];
  INT i;
  static int n;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"in","no open multigrid");
    return (CMDERRORCODE);
  }

  UserWriteF("# IPoint %d\n",n);
  n++;
  UserWriteF("# %s\n",argv[0]);
  if (sscanf(argv[0],"ngin %lf %lf %lf",xc,xc+1,xc+2)!=DIM)
  {
    PrintErrorMessageF('E',"in","specify %d coordinates for an inner node",(int)DIM);
    return (PARAMERRORCODE);
  }

  UserWriteF("I %lf %lf %lf;\n",xc[0],xc[1],xc[2]);

  return (OKCODE);
}

/****************************************************************************/
/*D
   bn - insert a boundary node and vertex

   DESCRIPTION:
   This command inserts an boundary node and the corresponding vertex
   into a multigrid with only level 0, calling the function
   'InsertBoubdaryNode'. The options are passed to the domain module function BVP_InsertBndP.

   'bn...'

   for the domain module std .... is
   '<Id> <s> [<t>]'
   .  <Id>              - insert a boundary node on the patch with <Id>
   .  <s>~[<t>]    - specify as much patch coordinates as the boundary has dimensions

   KEYWORDS:
   multigrid, insert, create, node, edit
   D*/
/****************************************************************************/

static INT InsertBoundaryNodeCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  BNDP *bndp;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"bn","no open multigrid");
    return (CMDERRORCODE);
  }

  bndp = BVP_InsertBndP (MGHEAP(theMG),MG_BVP(theMG),argc,argv);
  if (bndp == NULL)
  {
    PrintErrorMessage('E',"bn","inserting a boundary point failed");
    return (CMDERRORCODE);
  }

  if (InsertBoundaryNode(GRID_ON_LEVEL(theMG,0),bndp)==NULL)
  {
    PrintErrorMessage('E',"bn","inserting a boundary node failed");
    return (CMDERRORCODE);
  }

  InvalidatePicturesOfMG(theMG);
  InvalidateUgWindowsOfMG(theMG);

  return (OKCODE);
}


static INT NGInsertBoundaryNodeCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  BNDP *bndp;
  static int i;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"ngbn","no open multigrid");
    return (CMDERRORCODE);
  }

  UserWriteF("# BPoint %d \n",i);
  /* this works only for LGM domain and does no real insertion !!! */
  bndp = BVP_InsertBndP (MGHEAP(theMG),MG_BVP(theMG),argc,argv);
  if (bndp == NULL)
  {
    i++;
    return (OKCODE);
  }
  return (CMDERRORCODE);
}

/****************************************************************************/
/*D
   gn - insertion of a boundary/inner node and vertex from global coordinates

   DESCRIPTION:
   This command inserts a boundary node if a position can be found matching the
   global point with prescribed or default accuracy. Otherwise an inner node
   is inserted at the global position.
   CAUTION: some domain module maybe does not support insertion of boundary
   nodes from global coordinates!

   'gn <x> <y> [<z>] [$r <res>]'

   .  <x>~<y>~[<z>]		- global coordinates
   .  $r~<res>				- resolution

   KEYWORDS:
   multigrid, insert, create, node, edit
   D*/
/****************************************************************************/

static INT InsertGlobalNodeCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  BNDP *bndp;
  int i;
  int ropt = FALSE;
  int err = OKCODE;
  DOUBLE resolution;
  INT n = 2;
  INT my_argc = 0;
  char **my_argv;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"gn","no open multigrid");
    return (CMDERRORCODE);
  }

  /* assemble command line for bn */
  if (ReadArgvDOUBLE("r",&resolution,argc,argv)==0)
  {
    ropt = TRUE;
    n++;
  }
  my_argv = (char**)malloc(n*sizeof(char*));
  if (my_argv==NULL)
    return CMDERRORCODE;
  my_argv[my_argc] = StrDup(argv[0]);
  if (my_argv[my_argc]==NULL)
  {
    err = CMDERRORCODE;
    goto Exit_gn;
  }
  my_argv[my_argc][0] = 'b';                    /* gn --> bn */
  my_argc++;

  my_argv[my_argc] = StrDup("g");
  if (my_argv[my_argc]==NULL)
  {
    err = CMDERRORCODE;
    goto Exit_gn;
  }
  my_argc++;

  if (ropt)
  {
    char r_opt[64];
    sprintf(r_opt,"$r %g",resolution);
    my_argv[my_argc] = StrDup(r_opt);
    if (my_argv[my_argc]==NULL)
    {
      err = CMDERRORCODE;
      goto Exit_gn;
    }
    my_argc++;
  }
  ASSERT(n==my_argc);

  /* try inserting a boundary node */
  bndp = BVP_InsertBndP (MGHEAP(theMG),MG_BVP(theMG),my_argc,my_argv);
  if (bndp == NULL)
  {
    double x[DIM_MAX];
    DOUBLE xc[DIM];

    /* failed: try inserting an inner node */
    if (sscanf(argv[0],"gn %lf %lf %lf",x,x+1,x+2)!=DIM)
    {
      PrintErrorMessageF('E',"gn","specify %d global coordinates",(int)DIM);
      err = PARAMERRORCODE;
      goto Exit_gn;
    }
    for (i=0; i<DIM; i++)
      xc[i] = x[i];

    /* NB: toplevel=0 is checked by InsertInnerNode() */
    if (InsertInnerNode(GRID_ON_LEVEL(theMG,0),xc)==NULL)
    {
      PrintErrorMessage('E',"gn","inserting an inner node failed");
      err = CMDERRORCODE;
      goto Exit_gn;
    }
    UserWrite("  ### gn: inserted a in\n");
  }
  else if (InsertBoundaryNode(GRID_ON_LEVEL(theMG,0),bndp)==NULL)
  {
    PrintErrorMessage('E',"gn","inserting a boundary node failed");
    err =  CMDERRORCODE;
    goto Exit_gn;
  }
  else
    UserWrite("  ### gn: inserted a bn\n");

  InvalidatePicturesOfMG(theMG);
  InvalidateUgWindowsOfMG(theMG);

Exit_gn:
  for (i=0; i<my_argc; i++)
    if (my_argv[i]!=NULL)
      free(my_argv[i]);
  free(my_argv);

  return err;
}

/****************************************************************************/
/*D
   deln - delete a node and vertex

   DESCRIPTION:
   This command deletes a node and the corresponding vertex
   of the current multigrid, calling the function 'DeleteNode'.

   'deln <Id> | $s'

   .  <Id>  - ID of the node to be deleted
   .  $s    - delete ALL nodes from the selection

   KEYWORDS:
   multigrid, delete, remove, node, edit
   D*/
/****************************************************************************/

static INT DeleteNodeCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  INT sopt,i;

  /* following variables: keep type for sscanf */
  int id;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"deln","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check options */
  sopt = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 's' :
      sopt = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("deln",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (sopt)
  {
    if (SELECTIONMODE(theMG)==nodeSelection)
      for (i=0; i<SELECTIONSIZE(theMG); i++)
        if (DeleteNode(GRID_ON_LEVEL(theMG,0),(NODE *)SELECTIONOBJECT(theMG,i))!=GM_OK)
        {
          PrintErrorMessage('E',"deln","deleting the node failed");
          return (CMDERRORCODE);
        }
    ClearSelection(theMG);
    InvalidatePicturesOfMG(theMG);
    InvalidateUgWindowsOfMG(theMG);

    return (OKCODE);
  }

  if (sscanf(argv[0],"deln %d",&id)!=1)
  {
    PrintErrorMessage('E',"deln","specify the ID of the node to be deleted");
    return (PARAMERRORCODE);
  }

  /* NB: toplevel=0 is checked by DeleteNode() */
  if (DeleteNodeWithID(GRID_ON_LEVEL(theMG,0),id)!=GM_OK)
  {
    PrintErrorMessage('E',"deln","deleting the node failed");
    return (CMDERRORCODE);
  }

  InvalidatePicturesOfMG(theMG);
  InvalidateUgWindowsOfMG(theMG);

  return (OKCODE);
}

/****************************************************************************/
/*D
   move - move a node and vertex

   DESCRIPTION:
   This command moves a node and the corresponding vertex
   of the current multigrid to a new position,
   calling the functions 'MoveInnerNode' and 'MoveBoundaryNode'.

   'move {<Id> | $s} {$i <x> <y> [<z>] | $b <SegId> <s> [<t>]}'

   .  <Id>                - Id of the node to be moved
   .  $i~<x>~<y>~[<z>]    - specify as much coordinates as the space has dimensions
   .  $b~<Id>~<s>~[<t>]   - in the current implementation (domain module dependent)
                                                 boundary nodes can not be moved

   KEYWORDS:
   multigrid, move, node, edit
   D*/
/****************************************************************************/

static INT MoveNodeCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VERTEX *myVertex;
  NODE *theNode;
  DOUBLE xc[DIM];
  INT type,i,j,level,relative;

  /* following variables: keep type for sscanf */
  double x[DIM_MAX];
  int id,segid;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"move","no open multigrid");
    return (CMDERRORCODE);
  }

  theNode = NULL;

  /* check parameters */
  if (sscanf(argv[0],"move %d",&id)==1)
  {
    /* search node */
    for (level=0; level<=TOPLEVEL(theMG); level++)
      if ((theNode=FindNodeFromId(GRID_ON_LEVEL(theMG,level),id))!=NULL)
        break;
    if (theNode==NULL)
    {
      PrintErrorMessageF('E',"move","node with ID %ld not found",(long)id);
      return (CMDERRORCODE);
    }
  }

  /* check options */
  relative = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 's' :
      if (SELECTIONMODE(theMG)==nodeSelection)
      {
        PrintErrorMessage('E',"move","there is no node in the selection");
        return (PARAMERRORCODE);
      }
      if (SELECTIONSIZE(theMG)!=1)
      {
        PrintErrorMessage('E',"move","there is more than one node in the selection");
        return (PARAMERRORCODE);
      }
      theNode = (NODE *)SELECTIONOBJECT(theMG,0);
      break;

    case 'i' :
      if (OBJT(MYVERTEX(theNode))!=IVOBJ)
      {
        PrintErrorMessageF('E',"move","node with ID %ld is no inner node",(long)id);
        return (CMDERRORCODE);
      }
      type = IVOBJ;
      if (sscanf(argv[i],"i %lf %lf %lf",x,x+1,x+2)!=DIM)
      {
        PrintErrorMessageF('E',"move","specify %d new coordinates for an inner node",(int)DIM);
        return (PARAMERRORCODE);
      }
      for (j=0; j<DIM; j++)
        xc[j] = x[j];
      break;

    case 'b' :
      if (OBJT(MYVERTEX(theNode))!=BVOBJ)
      {
        PrintErrorMessageF('E',"move","node with ID %ld is no boundary node",(long)id);
        return (CMDERRORCODE);
      }
      type = BVOBJ;
      if (sscanf(argv[i],"b %d %lf %lf",&segid,x,x+1)!=1+DIM_OF_BND)
      {
        PrintErrorMessageF('E',"move","specify the segment if and %d new coordinates for a boundary node",(int)DIM_OF_BND);
        return (PARAMERRORCODE);
      }
      for (j=0; j<DIM_OF_BND; j++)
        xc[j] = x[j];
      break;

    case 'r' :
      relative = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("move",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (theNode==NULL)
  {
    PrintErrorMessage('E',"move","you have to either specify\n"
                      "the ID of the node to move or the s option");
    return (PARAMERRORCODE);
  }

  myVertex = MYVERTEX(theNode);
  if (type==IVOBJ)
  {
    if (relative)
    {
      /* move relative to old position */
      for (j=0; j<DIM; j++)
        xc[j] += CVECT(myVertex)[j];
    }
    if (MoveNode(theMG,theNode,xc,TRUE)!=GM_OK)
    {
      PrintErrorMessage('E',"move","failed moving the node");
      return (CMDERRORCODE);
    }
  }
  else if (type==BVOBJ)
  {
    PrintErrorMessage('E',"move","moving boundary nodes not implemented yet");
    return (CMDERRORCODE);
  }
  else
  {
    PrintHelp("move",HELPITEM," (either i or b option is mandatory)");
    return (PARAMERRORCODE);
  }

  InvalidatePicturesOfMG(theMG);

  return (OKCODE);
}

/****************************************************************************/
/*D
   ie - insert an element

   DESCRIPTION:
   This command inserts an element into a multigrid with only level 0,
   calling the function 'InsertElement'.


   'ie {<Id>}+ | $s'

   .  {<Id>}+  - specify at least three (2d) or four (3d) corner nodes, the corresponding (unique) element will be created
   .  $s                - taking selected nodes

   KEYWORDS:
   multigrid, insert, create, element, edit
   D*/
/****************************************************************************/

static INT InsertElementCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  NODE *theNode,*theNodes[MAX_CORNERS_OF_ELEM];
  char *token,*vstr;
  INT i,nNodes,Id[MAX_CORNERS_OF_ELEM];

  /* following variables: keep type for sscanf */
  int id;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"ie","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check options */
  nNodes = 0;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 's' :
      if (SELECTIONMODE(theMG)==nodeSelection)
        for (nNodes=0; nNodes<SELECTIONSIZE(theMG); nNodes++)
        {
          theNode = (NODE *)SELECTIONOBJECT(theMG,nNodes);
          if (nNodes>=MAX_CORNERS_OF_ELEM)
          {
            PrintErrorMessage('E',"ie","too many nodes are in the selection");
            return (CMDERRORCODE);
          }
          theNodes[nNodes] = theNode;
        }
      else
      {
        PrintErrorMessage('E',"ie","objects other than nodes are in the selection");
        return (PARAMERRORCODE);
      }
      if (nNodes==0)
      {
        PrintErrorMessage('E',"ie","no nodes are in the selection");
        return (PARAMERRORCODE);
      }
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("ie",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  /* got the nodes via s option? */
  if (nNodes>0)
  {
    if (InsertElement(GRID_ON_LEVEL(theMG,0),nNodes,theNodes,NULL,NULL,NULL)==NULL)
    {
      PrintErrorMessage('E',"ie","inserting the element failed");
      return (CMDERRORCODE);
    }
    else
    {
      InvalidatePicturesOfMG(theMG);
      InvalidateUgWindowsOfMG(theMG);
      return (OKCODE);
    }
  }

  /* set vstr after 'ie' */
  if ((vstr=strchr(argv[0],'e'))!=NULL)
    ++vstr;
  else
    return (CMDERRORCODE);

  /* we need the string split into tokens */
  nNodes = 0;
  token = strtok(vstr,WHITESPACE);
  while (token!=NULL)
  {
    if (nNodes>=MAX_CORNERS_OF_ELEM)
    {
      PrintErrorMessageF('E',"ie","specify at most %d id's",(int)MAX_CORNERS_OF_ELEM);
      return (PARAMERRORCODE);                                  /* too many items */
    }
    if (sscanf(token," %d",&id)!=1)
    {
      PrintErrorMessageF('E',"ie","could not read the id of corner no %d",(int)i);
      return (PARAMERRORCODE);                                  /* too many items */
    }
    Id[nNodes++] = id;
    token = strtok(NULL,WHITESPACE);
  }

  /* NB: toplevel=0 is checked by InsertElementFromIDs() */
  if (InsertElementFromIDs(GRID_ON_LEVEL(theMG,0),nNodes,Id,NULL)==NULL)
  {
    PrintErrorMessage('E',"ie","inserting the element failed");
    return (CMDERRORCODE);
  }

  InvalidatePicturesOfMG(theMG);
  InvalidateUgWindowsOfMG(theMG);

  return (OKCODE);
}

static INT NGInsertElementCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char *token,*vstr;
  INT i,bf,nNodes,Id[MAX_CORNERS_OF_ELEM];
  static int n;

  /* following variables: keep type for sscanf */
  int id;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"ngie","no open multigrid");
    return (CMDERRORCODE);
  }

  /* set vstr after 'ngie' */
  if ((vstr=strchr(argv[0],'e'))!=NULL)
    ++vstr;
  else
    return (CMDERRORCODE);

  UserWriteF("# %s\n",argv[0]);
  UserWriteF("# element %d\n",n);
  n++;
  UserWriteF("E ");

  /* we need the string split into tokens */
  nNodes = bf = 0;
  token = strtok(vstr,WHITESPACE);
  i = 0;
  while (token!=NULL)
  {
    /* separator for boundary faces */
    if (strcmp(token,"F") == 0)
    {
      UserWriteF("\n");
      bf = 1;
      goto NEXTTOKEN;
    }

    /* read next boundary face */
    if (bf > 0)
    {

      if (sscanf(token," %d",&id)!=1)
      {
        PrintErrorMessageF('E',"ngie","could not read the id of boundary face no %d",(int)bf);
        return (PARAMERRORCODE);                                        /* too many items */
      }
      UserWriteF("	F");
      switch (nNodes)
      {
      case 4 :
      case 5 :
      case 6 :
        UserWriteF("ngie: elementtype = %d not implemented!\n",nNodes);
        break;
      case 8 :
      {
        INT n;
        /* assert(id<SIDES_OF_ELEM_TAG(nNodes));*/

        for (n=0; n<CORNERS_OF_SIDE_TAG(7,id); n++)
        {
          UserWriteF(" %d",Id[CORNER_OF_SIDE_TAG(7,id,n)]);
        }
        UserWriteF("\n");
      }
      break;
      default :
        assert(0);
      }

      bf++;
      goto NEXTTOKEN;
    }

    /* read next node */
    if (nNodes>=MAX_CORNERS_OF_ELEM)
    {
      PrintErrorMessageF('E',"ngie","specify at most %d id's",(int)MAX_CORNERS_OF_ELEM);
      return (PARAMERRORCODE);                                  /* too many items */
    }
    if (sscanf(token," %d",&id)!=1)
    {
      PrintErrorMessageF('E',"ngie","could not read the id of corner no %d",(int)nNodes);
      return (PARAMERRORCODE);                                  /* too many items */
    }

    /* first id is subdomain */
    if (i > 0)
    {
      Id[nNodes] = id;
      nNodes++;
    }
    UserWriteF(" %d",id);

NEXTTOKEN:
    token = strtok(NULL,WHITESPACE);
    i++;
  }

  UserWriteF(";\n");

  return (OKCODE);
}

/****************************************************************************/
/*D
   dele - delete an element

   DESCRIPTION:
   This command deletes the specified  element of a multigrid
   with only level 0, calling the function 'DeleteElement'.

   'dele <Id> | $s'

   .  <Id> - ID of the element to be deleted
   .  $s   - delete all elements from the selection

   KEYWORDS:
   multigrid, delete, remove, element, edit
   D*/
/****************************************************************************/

static INT DeleteElementCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  INT i,sopt;

  /* following variables: keep type for sscanf */
  int id;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"dele","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check options */
  sopt = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 's' :
      sopt = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("dele",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (sopt)
  {
    if (SELECTIONMODE(theMG)==elementSelection)
      for (i=0; i<SELECTIONSIZE(theMG); i++)
        if (DeleteElement(theMG,(ELEMENT *)SELECTIONOBJECT(theMG,i))!=GM_OK)
        {
          PrintErrorMessage('E',"dele","deleting the element failed");
          return (CMDERRORCODE);
        }
    ClearSelection(theMG);
    InvalidatePicturesOfMG(theMG);
    InvalidateUgWindowsOfMG(theMG);

    return (OKCODE);
  }

  if (sscanf(argv[0],"dele %d",&id)!=1)
  {
    PrintErrorMessage('E',"dele","specify the ID of the element to be deleted");
    return (PARAMERRORCODE);
  }

  /* NB: toplevel=0 is checked by DeleteElement() */
  if (DeleteElementWithID(theMG,id)!=GM_OK)
  {
    PrintErrorMessage('E',"dele","deleting the element failed");
    return (CMDERRORCODE);
  }

  InvalidatePicturesOfMG(theMG);
  InvalidateUgWindowsOfMG(theMG);

  return (OKCODE);
}

/****************************************************************************/
/*D
   adapt - adapt the current multigrid

   DESCRIPTION:
   This command adapt the multigrid according to the refinement marks
   set in the elements, calling the function 'AdaptMultiGrid'.
   If coarsen marks are set refinements are deleted.

   'adapt [$g] [$a] [$h] [$x] [$d <vector plot proc>]'

   .  no~option				- only local refinement
   .  $g						- copy nonrefined regions to new level
   .  $a						- refine all elements
   .  $d~<vector~plot~proc>	- 3D only: use vector eval proc for determination of
                                                                regular refinement direction of tetrahedra
   .  $h						- refine not closed (not implemented yet)
   .  $x						- use hexahedra (not implemented yet)

   KEYWORDS:
   multigrid, adapt, mark
   D*/
/****************************************************************************/

static INT AdaptCommand (INT argc, char **argv)
{
  MULTIGRID       *theMG;
  INT i,mode,mark,rv;
  INT seq;
  INT mgtest;
  EVECTOR         *theElemEvalDirection;

        #ifdef ModelP
  if (!CONTEXT(me))
  {
    PRINTDEBUG(ui,0,("%2d: AdaptCommand(): me not in Context,"
                     " grid not refined\n",me))
    return (OKCODE);
  }
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"adapt","no open multigrid");
    return (CMDERRORCODE);
  }

  /* init defaults */
  seq = GM_REFINE_PARALLEL;
  mgtest = GM_REFINE_NOHEAPTEST;

  /* check options */
  theElemEvalDirection = NULL;
  mode = GM_REFINE_TRULY_LOCAL;
  mark = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      mark = MARK_ALL;
      break;

            #ifdef __THREEDIM__
    case 'd' :
      if (sscanf(argv[i],"a %s",buffer)==1)
        theElemEvalDirection = GetElementVectorEvalProc(buffer);
      if (theElemEvalDirection==NULL)
        UserWrite("direction eval fct not found: taking shortest interior edge\n");
      break;
                        #endif

    case 'g' :
      mode = mode | GM_COPY_ALL;
      break;

    case 'h' :
      mode = mode | GM_REFINE_NOT_CLOSED;
      break;

    case 's' :
      seq = GM_REFINE_SEQUENTIAL;
      break;

    case 't' :
      mgtest = GM_REFINE_HEAPTEST;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("refine",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

        #ifdef ModelP
  /* currently only this is supported in parallel */
  if (0)
    if (procs > 1)
    {
      mark = MARK_ALL;
      mode = mode | GM_REFINE_NOT_CLOSED;
    }
        #endif

  if (mark == MARK_ALL)
  {
    INT l,nmarked;
    ELEMENT *theElement;

    nmarked = 0;

    for (l=TOPLEVEL(theMG); l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l)); theElement!=NULL; theElement=SUCCE(theElement))
      {
        if (EstimateHere(theElement))
          if ((rv = MarkForRefinement(theElement,RED,NULL))!=0)
          {
            l = TOPLEVEL(theMG);
            break;
          }
          else
            nmarked++;
      }
    UserWriteF("%d: %d elements marked for regular refinement\n",me,nmarked);
  }

  /* get velocity */
        #ifdef __THREEDIM__
  SetAlignmentPtr (theMG, theElemEvalDirection);
        #endif

  rv = AdaptMultiGrid(theMG,mode,seq,mgtest);

  InvalidatePicturesOfMG(theMG);
  InvalidateUgWindowsOfMG(theMG);

  switch (rv)
  {
  case GM_OK :
    UserWriteF(" %s refined\n",ENVITEM_NAME(theMG));
    SetStringVar(":errno","0");
    return (OKCODE);

  case GM_COARSE_NOT_FIXED :
    PrintErrorMessage('E',"refine","do 'fixcoarsegrid' first and then refine!");
    SetStringVar(":errno","1");
    return (CMDERRORCODE);

  case GM_ERROR :
    PrintErrorMessage('E',"refine","could not refine, data structure still ok");
    SetStringVar(":errno","1");
    return (CMDERRORCODE);

  case GM_FATAL :
    PrintErrorMessage('F',"refine","could not refine, data structure inconsistent\n");
    SetStringVar(":errno","1");
    return (CMDERRORCODE);

  default :
    PrintErrorMessage('E',"refine","unknown error in refine");
    SetStringVar(":errno","1");
    return (CMDERRORCODE);
  }
}

/****************************************************************************/
/*D
   fixcoarsegrid - marks the end of corse grid generation

   DESCRIPTION:
   If the coarse grid is build interactively by 'ie', this command
   terminates this process and calls 'FixCoarseGrid'.

   SYNTAX:
   'fixcoarsegrid'

   KEYWORDS:
   multigrid, edit, finish
   D*/
/****************************************************************************/

static INT FixCoarseGridCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;

  theMG = currMG;

  PRINTDEBUG(ui,2,("%d: FixCoarseGrid currMG %x fixed %d\n",
                   me,theMG,MG_COARSE_FIXED(theMG)));

  if (theMG==NULL) {
    PrintErrorMessage('E',"fixcoarsegrid","no open multigrid");
    return (CMDERRORCODE);
  }
  if (FixCoarseGrid(theMG)) return (CMDERRORCODE);

  PRINTDEBUG(ui,2,("%d: FixCoarseGrid currMG %x fixed %d\n",
                   me,theMG,MG_COARSE_FIXED(theMG)));

  return(OKCODE);
}

/****************************************************************************/
/*D
   collapse - construct coarse grid from surface

   DESCRIPTION:
   The coarse grid is build from all surface elements.

   SYNTAX:
   'collapse'
   D*/
/****************************************************************************/

static INT CollapseCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;

  theMG = currMG;

  if (theMG==NULL) {
    PrintErrorMessage('E',"collapse","no open multigrid");
    return (CMDERRORCODE);
  }
  if (Collapse(theMG)) return (CMDERRORCODE);

  return(OKCODE);
}

/****************************************************************************/
/*D
   mark - mark elements with refinement type

   DESCRIPTION:
   This command marks elements with refinement type,
   calling the function 'MarkForRefinement'.

   SYNTAX:
   'mark [$h | {[<rule> [<side>]] [$a | $i <Id> | $s]} | $c] [$pos <x y [z]>] [$x <x>] [$y <y>] [$z <z>]'

   .  <rule>     - specify a refinement rule ("red" is default)
   .  <side>     - has to be specified if the corresponding rule can be applied in several orientations
   .  $a         - refine all (leave) elements
   .  $c		  - set all marks to no refinement
   .  $i <Id>    - refine the element with <Id>
   .  $s         - refine all elements from the current selection
   .  $h         - show available rules
   .  $x         - marks elements with corner[0] < x
   .  $y         - marks elements with corner[1] < y
   .  $z         - marks elements with corner[2] < z

   KEYWORDS:
   multigrid, refine, adapt, rule, type, mark

   SEE ALSO:
   'refine'
   D*/
/****************************************************************************/

static INT MarkCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  ELEMENT *theElement;
  char rulename[32];
  INT i,j,l,mode,rv,Rule,sid;
  DOUBLE_VECTOR global;
  DOUBLE x,X,y,Y;
  long nmarked;
#       ifdef __THREEDIM__
  DOUBLE z,Z;
#       endif

  /* following variables: keep type for sscanf */
  int id,idfrom,idto;
  INT Side;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%d: MarkCommand() me not in Context," \
                     " nothing marked\n",me))
    return (OKCODE);
  }
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"mark","no open multigrid");
    return (CMDERRORCODE);
  }

  /* first check help option */
  for (i=1; i<argc; i++)
    if (argv[i][0]=='h')
    {
      UserWrite("the following rules are available:\n");
      for (j=0; j<NO_OF_RULES && myMR[j].RuleName!=NULL; j++)
      {
        UserWrite(myMR[j].RuleName);
        UserWrite("\n");
      }
      return (OKCODE);
    }

  /* scan parameters */
  /*rv = sscanf(argv[0],"mark %31[redbluecopycoarsnoi_123qtpritet2hex] %d",rulename,&Side);*/
  rv = sscanf(argv[0],"mark %31[a-z_0-9] %d",rulename,&Side);
  if (rv<1)
  {
    /* set the default rule */
    strcpy(rulename,myMR[0].RuleName);
    Rule = myMR[0].RuleId;
    Side = NO_SIDE_SPECIFIED;
  }
  else
  {
    Rule = NO_RULE_SPECIFIED;
    for (i=0; i<NO_OF_RULES; i++)
      if (strcmp(rulename,myMR[i].RuleName)==0)
      {
        Rule = myMR[i].RuleId;
        break;
      }

    if (Rule==NO_RULE_SPECIFIED)
    {
      PrintErrorMessageF('E',"mark","unknown rule '%s'",rulename);
      return (PARAMERRORCODE);
    }

    if (rv!=2)
      Side = NO_SIDE_SPECIFIED;
  }

  if (ReadArgvOption("c",argc, argv))
  {
    for (i=0; i<=TOPLEVEL(theMG); i++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,i));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
          MarkForRefinement(theElement,NO_REFINEMENT,NULL);

    UserWrite("all refinement marks removed\n");

    return(OKCODE);
  }

  if (ReadArgvDOUBLE("x",&x,argc, argv)==0)
  {
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
          for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
            if (XC(MYVERTEX(CORNER(theElement,j))) < x)
              MarkForRefinement(theElement,Rule,NULL);

    UserWriteF("all elements in x < %f marked for refinement\n",
               (float) x);

    return(OKCODE);
  }

  if (ReadArgvDOUBLE("X",&x,argc, argv)==0)
  {
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
          for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
            if (XC(MYVERTEX(CORNER(theElement,j))) > x)
              MarkForRefinement(theElement,Rule,NULL);

    UserWriteF("all elements in x > %f marked for refinement\n",
               (float) x);

    return(OKCODE);
  }

  if (ReadArgvDOUBLE("y",&y,argc, argv)==0)
  {
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
          for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
            if (YC(MYVERTEX(CORNER(theElement,j))) < y)
              MarkForRefinement(theElement,Rule,NULL);

    UserWriteF("all elements in y < %f marked for refinement\n",
               (float) y);

    return(OKCODE);
  }

  if (ReadArgvDOUBLE("Y",&y,argc, argv)==0)
  {
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
          for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
            if (YC(MYVERTEX(CORNER(theElement,j))) > y)
              MarkForRefinement(theElement,Rule,NULL);

    UserWriteF("all elements in y > %f marked for refinement\n",
               (float) y);

    return(OKCODE);
  }

  if (ReadArgvDOUBLE("stripes",&x,argc, argv)==0)
  {
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
        {
          INT flag=1;
          for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
          {
            DOUBLE xc=YC(MYVERTEX(CORNER(theElement, j)));
            xc=fmod(xc,(4*x));
            if ((xc<0.9*x)||(xc>2.1*x)) flag=0;
          }
          if(flag)
            MarkForRefinement(theElement,Rule,NULL);
        }
    UserWriteF("stripes %f\n", (float) x);

    return(OKCODE);
  }

  if (ReadArgvINT("S",&sid,argc, argv)==0)
  {
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
          for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
            if (SUBDOMAIN(theElement) == sid)
              MarkForRefinement(theElement,Rule,NULL);

    UserWriteF("all elements in subdomain %d marked for refinement\n",sid);

    return(OKCODE);
  }

#ifdef __THREEDIM__
  if ((ReadArgvDOUBLE("x0",&x,argc, argv)==0) &&
      (ReadArgvDOUBLE("x1",&X,argc, argv)==0) &&
      (ReadArgvDOUBLE("y0",&y,argc, argv)==0) &&
      (ReadArgvDOUBLE("y1",&Y,argc, argv)==0) &&
      (ReadArgvDOUBLE("z0",&z,argc, argv)==0) &&
      (ReadArgvDOUBLE("z1",&Z,argc, argv)==0))
  {
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
          for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
            if ((XC(MYVERTEX(CORNER(theElement,j))) < X) &&
                (XC(MYVERTEX(CORNER(theElement,j))) > x) &&
                (YC(MYVERTEX(CORNER(theElement,j))) < Y) &&
                (YC(MYVERTEX(CORNER(theElement,j))) > y) &&
                (ZC(MYVERTEX(CORNER(theElement,j))) < Z) &&
                (ZC(MYVERTEX(CORNER(theElement,j))) > z))
              MarkForRefinement(theElement,Rule,NULL);

    UserWriteF("all elements in box marked for refinement\n");

    return(OKCODE);
  }

  if (ReadArgvDOUBLE("z",&z,argc, argv)==0)
  {
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
          for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
            if (ZC(MYVERTEX(CORNER(theElement,j))) < z)
              MarkForRefinement(theElement,Rule,NULL);

    UserWriteF("all elements in z < %f marked for refinement\n",
               (float) z);

    return(OKCODE);
  }

  if (ReadArgvDOUBLE("Z",&z,argc, argv)==0)
  {
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
          for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
            if (ZC(MYVERTEX(CORNER(theElement,j))) > z)
              MarkForRefinement(theElement,Rule,NULL);

    UserWriteF("all elements in z > %f marked for refinement\n",
               (float) z);

    return(OKCODE);
  }

#endif

  if (ReadArgvPosition("pos",argc,argv,global)==0)
  {
    DOUBLE r;

    if (ReadArgvDOUBLE("r",&r,argc, argv)==0) {
      for (l=0; l<=TOPLEVEL(theMG); l++)
        for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
             theElement!=NULL; theElement=SUCCE(theElement))
          if (EstimateHere(theElement))
            for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
            {
              DOUBLE dist;

              V_DIM_EUKLIDNORM_OF_DIFF(global,
                                       CVECT(MYVERTEX(CORNER(theElement,j))),dist);
              if (dist <= r) {
                MarkForRefinement(theElement,Rule,NULL);
                break;
              }
            }
      UserWriteF("all elements in |x - p|  < %f marked for refinement\n",
                 (float) r);

      return(OKCODE);
    }
    theElement = FindElementOnSurface(theMG,global);
    if (theElement != NULL) {
      MarkForRefinement(theElement,Rule,NULL);
        #ifdef ModelP
      j = (INT) UG_GlobalSumDOUBLE(1.0);
      i = DDD_InfoGlobalId(PARHDRE(theElement));
    }
    else {
      j = (INT) UG_GlobalSumDOUBLE(0.0);
      i = -1;
    }
    if (j == 0)
      return(PARAMERRORCODE);

    for (l=0; l<j; l++) {
      rv = UG_GlobalMaxINT(i);
      UserWriteF("element GID %08x marked for refinement\n",rv);
      if (rv == i) i = -1;
    }
        #else
      UserWriteF("element %d marked for refinement\n",ID(theElement));
    }
    else
      return(PARAMERRORCODE);
            #endif

    return (OKCODE);
  }

  /* check options */
  mode = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      if (mode!=FALSE)
      {
        PrintErrorMessage('E',"mark","specify only one option of a, b, i, s");
        return (PARAMERRORCODE);
      }
      mode = MARK_ALL;
      break;

    case 'i' :
      if (mode!=FALSE)
      {
        PrintErrorMessage('E',"mark","specify only one option of a, b, i, s");
        return (PARAMERRORCODE);
      }
      mode = MARK_ID;
      j = sscanf(argv[i],"i %d %d", &idfrom, &idto);
      if (j<1 || j>2)
      {
        PrintErrorMessage('E',"mark","cannot scan id(s)");
        return (PARAMERRORCODE);
      }
      if (j == 1) idto = idfrom;
      break;

    case 's' :
      if (mode!=FALSE)
      {
        PrintErrorMessage('E',"mark","specify only one option of a, b, i, s");
        return (PARAMERRORCODE);
      }
      mode = MARK_SELECTION;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("mark",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (mode==FALSE)
  {
    PrintErrorMessage('E',"mark","specify exactly one option of a, b, i, s");
    return (PARAMERRORCODE);
  }

  /* print rule and side */
  if (Side==NO_SIDE_SPECIFIED)
    UserWriteF("   using rule %s (no side given)\n",rulename);
  else
    UserWriteF("   using rule %s, side %d\n",rulename,(int)Side);

  nmarked = 0;
  switch (mode)
  {
  case MARK_ALL :
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement)) {
        if (EstimateHere(theElement))
          if ((rv = MarkForRefinement(theElement,
                                      Rule,(void *)Side))!=0)
          {
            l = TOPLEVEL(theMG);
            break;
          }
          else
            nmarked++;
      }
    break;

  case MARK_ID :
    theElement = NULL;
    for (id=idfrom; id<=idto; id++) {
      for (l=0; l<=TOPLEVEL(theMG); l++)
        if ((theElement=FindElementFromId(GRID_ON_LEVEL(theMG,l),id))!=NULL)
          break;

      if (theElement==NULL)
      {
        PrintErrorMessageF('W',"mark","element with ID %ld could not be found, nothing marked",id);
      }

      if (EstimateHere(theElement))
        if ((rv = MarkForRefinement(theElement,
                                    Rule,(void *)Side))!=0)
          break;
        else
          nmarked++;
    }
    break;

  case MARK_SELECTION :
    if (SELECTIONMODE(theMG)==elementSelection)
      for (i=0; i<SELECTIONSIZE(theMG); i++)
      {
        theElement = (ELEMENT *)SELECTIONOBJECT(theMG,i);
        if (EstimateHere(theElement))
        {
          if ((rv = MarkForRefinement(theElement,
                                      Rule,(void *)Side))!=0)
            break;
          else
            nmarked++;
        }
      }
    break;
  }

        #ifdef ModelP
  nmarked = UG_GlobalSumINT(nmarked);
        #endif
  UserWriteF(" %ld elements marked for refinement\n",nmarked);

  if (rv && theElement)
  {
    PrintErrorMessageF('W',"mark","rule could not be applied for element with ID %ld, nothing marked",ID(theElement));
    return (CMDERRORCODE);
  }
  else
    return(OKCODE);
}

/****************************************************************************/
/*D
   smooth - invoke hierarchical multigrid smoother

   DESCRIPTION:
   This command invokes hierarchical multigrid smoother,
   calling the function 'SmoothMultiGrid'.

   SYNTAX:
   'smooth <nIt> [$b] [$nc]'

   .    <nIt>   - number of iterations
   .    $b      - also smooth boundary nodes
   .    $nc     - improvement for nonconvex domains

   KEYWORDS:
   multigrid
   D*/
/****************************************************************************/

static INT SmoothMGCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  INT i,smbdry;

  /* following variables: keep type for sscanf */
  int nit;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"smooth","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check pararmeters */
  if (sscanf(argv[0],"smooth %d",&nit)!=1)
  {
    PrintHelp("smooth",HELPITEM," (specify number of iterations)");
    return (PARAMERRORCODE);
  }

  /* check options */
  smbdry = GM_KEEP_BOUNDARY_NODES;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'b' :
      smbdry = 1;
      break;

    case 'n' :
      smbdry = 2;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("move",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (SmoothMultiGrid(theMG,nit,smbdry)!=GM_OK)
  {
    PrintErrorMessage('E',"smooth","failed smoothing the multigrid");
    return (CMDERRORCODE);
  }

  InvalidatePicturesOfMG(theMG);

  return (OKCODE);
}

/****************************************************************************/
/*D
   smoothgrid - resize quadrilaterals and triangles on surface levels according to
              the element sizes on level l-1

   DESCRIPTION:
   'smoothgrid [$limit <value>] [$reset] [$f <value>]  [$b] [($ortho0 || $ortho1) <val0> <val1> ... <valN>]'

   . $limit~<value>       - give maximum displacement of the vertices in local coordinates
                         of the father element (0 < value < 0.5, default: 0.3)
   . $reset               - reset elements to default size
   . $f <value>           - apply smoothgrid for all elements between level <value> and current level (default: current level)
   . $ortho0 <b0> <b1> ...- generate 'orthogonal' elements on boundaries <b0> ... <bN> (N<=20)
   . $ortho1 <b0> <b1> ...- generate 'orthogonal' elements on boundaries <b0> ... <bN> (N<=20) for all sons
                         of the boundary elements on level 0
   . $b                   - move only boundary mid nodes
   . $spline              - additionaly smooth elements using spline functions
   . $spline0             - smooth elements using spline functions only

   KEYWORDS:
   multigrid, anisotropy
   D*/
/****************************************************************************/

static INT SmoothGridCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  DOUBLE LimitLocDis;
  INT i,GridReset,lowLevel;
  INT bnd_num,bnd[22],fl,option,dummy;
  float fValue;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"smoothgrid","no current multigrid");
    return(CMDERRORCODE);
  }
  if (CURRENTLEVEL(theMG)==0)
  {
    PrintErrorMessage('E',"smoothgrid","cannot smooth grid on level 0");
    return(CMDERRORCODE);
  }
#ifdef __THREEDIM__
  PrintErrorMessage('E',"smoothgrid","3D not implemented yet");
  return(CMDERRORCODE);
#endif

  /* options */
  lowLevel = CURRENTLEVEL(theMG);
  option = 0;
  GridReset = FALSE;
  LimitLocDis = 0.3;
  bnd_num = 0;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'r' :
      if (strcmp(argv[i],"reset")==0)
        GridReset=TRUE;
      else
      {
        PrintErrorMessageF('E',"smoothgrid","(invalid option '%s')",argv[i]);
        return (PARAMERRORCODE);
      }
      break;
    case 'l' :
      if (sscanf(argv[i],"limit %f",&fValue)!=1)
      {
        PrintErrorMessageF('E',"smoothgrid","(invalid option '%s')",argv[i]);
        return (PARAMERRORCODE);
      }
      LimitLocDis = (DOUBLE) fValue;
      if (LimitLocDis>=0.5 || LimitLocDis <=0)
      {
        PrintErrorMessage('E',"smoothgrid","specify a local limit between 0 and 0.5 (default 0.3)");
        return (PARAMERRORCODE);
      }
      break;

    case 'f' :
      if (sscanf(argv[i],"f %d",&fl)!=1)
      {
        PrintErrorMessageF('E',"smoothgrid","(invalid option '%s')",argv[i]);
        return (PARAMERRORCODE);
      }
      lowLevel = fl;
      break;
    case 'o' :
      if (strstr(argv[i],"ortho0")!=NULL)
      {
        if ((bnd_num=sscanf(argv[i],"ortho0 %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                            &bnd[0],&bnd[1],&bnd[2],&bnd[3],&bnd[4],&bnd[5],&bnd[6],&bnd[7],&bnd[8],&bnd[9],
                            &bnd[10],&bnd[11],&bnd[12],&bnd[13],&bnd[14],&bnd[15],&bnd[16],&bnd[17],&bnd[18],
                            &bnd[19],&bnd[20],&bnd[21],&dummy))<1)
        {
          PrintErrorMessage('E',"smoothgrid","specify at least one boundary-id with 'ortho0' option");
          return (PARAMERRORCODE);
        }
        if (option!=0)
        {
          PrintErrorMessage('E',"smoothgrid","specify either $b, $ortho0 or $ortho1 option");
          return (PARAMERRORCODE);
        }
        option = 1;
      }
      else if (strstr(argv[i],"ortho1")!=NULL)
      {
        if ((bnd_num=sscanf(argv[i],"ortho1 %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                            &bnd[0],&bnd[1],&bnd[2],&bnd[3],&bnd[4],&bnd[5],&bnd[6],&bnd[7],&bnd[8],&bnd[9],
                            &bnd[10],&bnd[11],&bnd[12],&bnd[13],&bnd[14],&bnd[15],&bnd[16],&bnd[17],&bnd[18],
                            &bnd[19],&bnd[20],&bnd[21],&dummy))<1)
        {
          PrintErrorMessage('E',"smoothgrid","specify at least one boundary-id with 'ortho1' option");
          return (PARAMERRORCODE);
        }
        if (option!=0)
        {
          PrintErrorMessage('E',"smoothgrid","specify either $b, $ortho0 or $ortho1 option");
          return (PARAMERRORCODE);
        }
        option = 2;
      }
      else
      {
        PrintErrorMessageF('E',"smoothgrid","(invalid option '%s')",argv[i]);
        return (PARAMERRORCODE);
      }
      if (bnd_num>21)
      {
        PrintErrorMessage('E',"smoothgrid","cannot process more than 9 boundaries with 'ortho' option");
        return (PARAMERRORCODE);
      }
      break;

    case 'b' :
      if (option!=0)
      {
        PrintErrorMessage('E',"smoothgrid","specify either $b, $ortho0 or $ortho1 option");
        return (PARAMERRORCODE);
      }
      option = 3;
      break;

    case 's' :
      break;
    default :
      PrintErrorMessageF('E',"smoothgrid","(invalid option '%s')",argv[i]);
      return (PARAMERRORCODE);
    }

  if (ReadArgvOption("spline",argc,argv))
  {
    if (option==0)
      option = 5;
    else if (option==1)
      option = 6;
    else if (option==2)
      option = 7;
  }
  if (ReadArgvOption("spline0",argc,argv))
  {
    option = 4;
  }
  UserWriteF("option = %d\n",option);
  if (GridReset==TRUE)
  {
    if (SmoothGridReset(theMG,lowLevel,CURRENTLEVEL(theMG))!=0) return(CMDERRORCODE);
  }
  else
  {
    lowLevel = MIN(FULLREFINELEVEL(theMG),lowLevel);
    lowLevel = MAX(lowLevel,1);
    if (SmoothGrid(theMG,lowLevel,CURRENTLEVEL(theMG),LimitLocDis,bnd_num,bnd,option)!=0) return(CMDERRORCODE);
  }

  InvalidatePicturesOfMG(theMG);
  InvalidateUgWindowsOfMG(theMG);

  return (OKCODE);
}

/****************************************************************************/
/*D
   ordernodes - order the nodes lexicographically according to the specified directions


   DESCRIPTION:
   This command orders the nodes according to the user provided dependencies.
   It orders the nodes of the current multigrid, calling the function
   'OrderNodesInGrid' on all levels.

   If specified the links are ordered in the corresponding order.

   'ordernodes ur|ul|dr|dl|ru|rd|lu|ld [$l <level>] [$L]'

   .   $l~<level> - only on level <level>
   .n                              u=up, d=down, r=right, l=left
   .   $L		   - also order links

    EXAMPLE:
        'ordernodes rd $l2'

   Order nodes of grid level 2 lexicographically in horizontal lines from
   left to right and the lines vertical from top down.

   KEYWORDS:
   multigrid, order
   D*/
/****************************************************************************/

static INT OrderNodesCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *theGrid;
  INT i,res,level,fromLevel,toLevel;
  INT sign[DIM],order[DIM],xused,yused,zused,error,AlsoOrderLinks;
  char ord[3];

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"ordernodes","no open multigrid");
    return (CMDERRORCODE);
  }
  fromLevel = 0;
  toLevel   = TOPLEVEL(theMG);

  /* read ordering directions */
        #ifdef __TWODIM__
  res = sscanf(argv[0],expandfmt("ordernodes %2[rlud]"),ord);
        #else
  res = sscanf(argv[0],expandfmt("ordernodes %3[rlbfud]"),ord);
        #endif
  if (res!=1)
  {
    PrintHelp("ordernodes",HELPITEM," (could not read order type)");
    return(PARAMERRORCODE);
  }
  if (strlen(ord)!=DIM)
  {
    PrintHelp("ordernodes",HELPITEM," (specify DIM chars out of 'rlud' or 'rlbfud' resp.)");
    return(PARAMERRORCODE);
  }
  error = xused = yused = zused = FALSE;
  for (i=0; i<DIM; i++)
    switch (ord[i])
    {
    case 'r' :
      if (xused) error = TRUE;
      xused = TRUE;
      order[i] = _X_; sign[i] =  1; break;
    case 'l' :
      if (xused) error = TRUE;
      xused = TRUE;
      order[i] = _X_; sign[i] = -1; break;

                        #ifdef __TWODIM__
    case 'u' :
      if (yused) error = TRUE;
      yused = TRUE;
      order[i] = _Y_; sign[i] =  1; break;
    case 'd' :
      if (yused) error = TRUE;
      yused = TRUE;
      order[i] = _Y_; sign[i] = -1; break;
                        #else
    case 'b' :
      if (yused) error = TRUE;
      yused = TRUE;
      order[i] = _Y_; sign[i] =  1; break;
    case 'f' :
      if (yused) error = TRUE;
      yused = TRUE;
      order[i] = _Y_; sign[i] = -1; break;

    case 'u' :
      if (zused) error = TRUE;
      zused = TRUE;
      order[i] = _Z_; sign[i] =  1; break;
    case 'd' :
      if (zused) error = TRUE;
      zused = TRUE;
      order[i] = _Z_; sign[i] = -1; break;
                        #endif
    }
  if (error)
  {
    PrintHelp("ordernodes",HELPITEM," (bad combination of 'rludr' or 'rlbfud' resp.)");
    return(PARAMERRORCODE);
  }

  /* check options */
  AlsoOrderLinks = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'l' :
      res = sscanf(argv[i],"l %d",&level);
      if (res!=1)
      {
        PrintErrorMessage('E',"ordernodes","could not read level");
        return(PARAMERRORCODE);
      }
      if ((level>=fromLevel)&&(level<=toLevel))
        fromLevel = toLevel = level;
      else
      {
        PrintErrorMessage('E',"ordernodes","level out of range");
        return(PARAMERRORCODE);
      }
      break;

    case 'L' :
      AlsoOrderLinks = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("ordernodes",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  /* first we renumber the multigrid (to have node-IDs coinciding with lists) */
  if (RenumberMultiGrid(theMG,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0)!=GM_OK)
  {
    PrintErrorMessage('E',"ordernodes","renumbering of the mg failed");
    return (CMDERRORCODE);
  }

  /* now we reorder the nodes on the specified levels */
  for (level=fromLevel; level<=toLevel; level++)
  {
    theGrid = GRID_ON_LEVEL(theMG,level);

    UserWriteF(" [%d:",level);

    if (OrderNodesInGrid(theGrid,order,sign,AlsoOrderLinks)!=GM_OK)
    {
      PrintErrorMessage('E',"ordernodes","OrderNodesInGrid failed");
      return (CMDERRORCODE);
    }
    UserWrite("o]");
  }

  UserWrite("\n");

  return (OKCODE);
}


/****************************************************************************/
/*D
   lexorderv - order the vectors lexicographically


   DESCRIPTION:
   This command orders the vectors lexicographically according to the user
   specified directions.
   It orders the vectors of the current multigrid, calling the function
   'LexOrderVectorsInGrid'.

   'lexorderv ur|ul|dr|dl|ru|rd|lu|ld [$l <level>] [$m] [$w s|n] [$s <|>]'

   .   $l~<level> - only on level <level>
   .n                              u=up, d=down, r=right, l=left
   .   $m		   - also order matrices
   .   $w~s|n	   - order skip or nonskip vectors resp.
   .   $s <|>	   - put skip vectors at begin or end of the list resp.

   KEYWORDS:
   multigrid, order
   D*/
/****************************************************************************/

static INT LexOrderVectorsCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *theGrid;
  INT i,res,level,fromLevel,toLevel;
  INT sign[DIM],order[DIM],which,xused,yused,zused,rused,pused,error,AlsoOrderMatrices,SpecialTreatSkipVecs,mode;
  char ord[3];

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"lexorderv","no open multigrid");
    return (CMDERRORCODE);
  }
  fromLevel = 0;
  toLevel   = TOPLEVEL(theMG);

  /* read ordering directions */
        #ifdef __TWODIM__
  res = sscanf(argv[0],expandfmt("lexorderv %2[rludIOPN]"),ord);
        #else
  res = sscanf(argv[0],expandfmt("lexorderv %3[rlbfud]"),ord);
        #endif
  if (res!=1)
  {
    PrintHelp("lexorderv",HELPITEM," (could not read order type)");
    return(PARAMERRORCODE);
  }
  if (strlen(ord)!=DIM)
  {
    PrintHelp("lexorderv",HELPITEM," (specify DIM chars out of 'rlud', 'IOPN' or 'rlbfud' resp.)");
    return(PARAMERRORCODE);
  }
  error = xused = yused = zused = rused = pused = FALSE;
  for (i=0; i<DIM; i++)
    switch (ord[i])
    {
    case 'r' :
      if (xused) error = TRUE;
      xused = TRUE;
      order[i] = _X_; sign[i] =  1; break;
    case 'l' :
      if (xused) error = TRUE;
      xused = TRUE;
      order[i] = _X_; sign[i] = -1; break;

                        #ifdef __TWODIM__
    case 'u' :
      if (yused) error = TRUE;
      yused = TRUE;
      order[i] = _Y_; sign[i] =  1; break;
    case 'd' :
      if (yused) error = TRUE;
      yused = TRUE;
      order[i] = _Y_; sign[i] = -1; break;
                        #else
    case 'b' :
      if (yused) error = TRUE;
      yused = TRUE;
      order[i] = _Y_; sign[i] =  1; break;
    case 'f' :
      if (yused) error = TRUE;
      yused = TRUE;
      order[i] = _Y_; sign[i] = -1; break;

    case 'u' :
      if (zused) error = TRUE;
      zused = TRUE;
      order[i] = _Z_; sign[i] =  1; break;
    case 'd' :
      if (zused) error = TRUE;
      zused = TRUE;
      order[i] = _Z_; sign[i] = -1; break;
                        #endif

                        #ifdef __TWODIM__

    /* polar coordiante directions */
    case 'I' :                          /* capital i */
      if (rused) error = TRUE;
      rused = TRUE;
      order[i] = 0; sign[i] =  1; break;

    case 'O' :
      if (rused) error = TRUE;
      rused = TRUE;
      order[i] = 0; sign[i] = -1; break;

    case 'P' :
      if (pused) error = TRUE;
      pused = TRUE;
      order[i] = 1; sign[i] =  1; break;

    case 'N' :
      if (pused) error = TRUE;
      pused = TRUE;
      order[i] = 1; sign[i] = -1; break;
                        #endif
    }
  if (error)
  {
    PrintHelp("lexorderv",HELPITEM," (bad combination of 'rludr' or 'rlbfud' resp.)");
    return(PARAMERRORCODE);
  }
  mode = OV_CARTES;
  if (rused || pused)
    if (!(rused && pused))
    {
      PrintHelp("lexorderv",HELPITEM," (bad combination of cartesian/polar direction)");
      return(PARAMERRORCODE);
    }
    else
      mode = OV_POLAR;

  /* check options */
  AlsoOrderMatrices = SpecialTreatSkipVecs = FALSE;
  which = GM_TAKE_SKIP | GM_TAKE_NONSKIP;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'l' :
      res = sscanf(argv[i],"l %d",&level);
      if (res!=1)
      {
        PrintErrorMessage('E',"lexorderv","could not read level");
        return(PARAMERRORCODE);
      }
      if ((level>=fromLevel)&&(level<=toLevel))
        fromLevel = toLevel = level;
      else
      {
        PrintErrorMessage('E',"lexorderv","level out of range");
        return(PARAMERRORCODE);
      }
      break;

    case 'm' :
      AlsoOrderMatrices = TRUE;
      break;

    case 'w' :
      which = 0;
      if (strchr(argv[i],'s')!=NULL)
        which |= GM_TAKE_SKIP;
      if (strchr(argv[i],'n')!=NULL)
        which |= GM_TAKE_NONSKIP;
      break;

    case 's' :
      if              (strchr(argv[i],'<')!=NULL)
        SpecialTreatSkipVecs = GM_PUT_AT_BEGIN;
      else if (strchr(argv[i],'>')!=NULL)
        SpecialTreatSkipVecs = GM_PUT_AT_END;
      else if (strchr(argv[i],'0')!=NULL)
        SpecialTreatSkipVecs = FALSE;
      else
      {
        PrintErrorMessage('E',"lexorderv","use < or > with s-option");
        return(PARAMERRORCODE);
      }
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("lexorderv",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  /* now we reorder the vectors on the specified levels */
  for (level=fromLevel; level<=toLevel; level++)
  {
    theGrid = GRID_ON_LEVEL(theMG,level);

    UserWriteF(" [%d:",level);

    if (LexOrderVectorsInGrid(theGrid,mode,order,sign,which,SpecialTreatSkipVecs,AlsoOrderMatrices)!=GM_OK)
    {
      PrintErrorMessage('E',"lexorderv","LexOrderVectorsInGrid failed");
      return (CMDERRORCODE);
    }
    UserWrite("ov]");
  }

  UserWrite("\n");

  return (OKCODE);
}

/****************************************************************************/
/*D
   shellorderv - order the vectors shell by shell


   DESCRIPTION:
   This command orders the vectors of the current level of the current
   multigrid in shells starting from a seed.

   'shellorderv f | l | s'

   .  f - take first vector as seed
   .  l - take last vector as seed
   .  s - take selected vector as seed

   KEYWORDS:
   multigrid, order, shell
   D*/
/****************************************************************************/

static INT ShellOrderVectorsCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *theGrid;
  VECTOR *seed;
  char option;

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"shellorderv","no open multigrid");
    return (CMDERRORCODE);
  }
  theGrid = GRID_ON_LEVEL(theMG,CURRENTLEVEL(theMG));

  if (sscanf(argv[0],"shellorderv %c",&option)!=1)
  {
    PrintErrorMessage('E',"shellorderv","specify f, l or s");
    return (CMDERRORCODE);
  }

  switch (option)
  {
  case 'f' :
    seed = FIRSTVECTOR(theGrid);
    break;
  case 'l' :
    seed = LASTVECTOR(theGrid);
    break;
  case 's' :
    if (SELECTIONMODE(theMG)!=vectorSelection)
    {
      PrintErrorMessage('E',"shellorderv","no vector selection");
      return (CMDERRORCODE);
    }
    if (SELECTIONSIZE(theMG)!=1)
    {
      PrintErrorMessage('E',"shellorderv","select ONE vector");
      return (CMDERRORCODE);
    }
    seed = (VECTOR *)SELECTIONOBJECT(theMG,0);
    break;
  default :
    PrintErrorMessage('E',"shellorderv","specify f, l or s");
    return (CMDERRORCODE);
  }

  if (ShellOrderVectors(theGrid,seed)!=0)
  {
    PrintErrorMessage('E',"shellorderv","ShellOrderVectors failed");
    return (CMDERRORCODE);
  }
  else
  {
    l_setindex(theGrid);
    return (OKCODE);
  }
}

/****************************************************************************/
/*D
   orderv - order the vectors according to the user provided dependencies


   DESCRIPTION:
   This command orders the vectors according to the user provided dependencies.
   It orders the vectors of the current multigrid, calling the function
   'OrderVectors'.

   'orderv $m FFCCLL | FCFCLL $d <dep-proc> $o <dep-proc options> $c <find-cut-proc> [$a]'

   .  $m FFCCLL | FCFCLL     - possible modes are FFCCLL or FCFCLL
   .  $d <dep-proc>          - the ordering algorithm uses this dependency procedure...
   .  $o <dep-proc options>  - ...and passes these options to it
   .  $a                     - order all levels of the current multigrid

   SEE ALSO:
   'lineorderv'

   KEYWORDS:
   multigrid, order, downstream
   D*/
/****************************************************************************/

static INT OrderVectorsCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char modestr[7];              /* <-- with change of size also change format string in sscanf! */
  char *dep,*dep_opt,*cut;
  INT mode,i,levels,PutSkipFirst,SkipPat;
  int iValue;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"orderv","no open multigrid");
    return (CMDERRORCODE);
  }

  levels           = GM_CURRENT_LEVEL;
  mode             = FALSE;
  dep              = dep_opt = cut = NULL;
  PutSkipFirst = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'm' :
      if (sscanf(argv[i],"m %6[FCL]",modestr)!=1)
      {
        PrintHelp("orderv",HELPITEM," (could not read the mode)");
        return (PARAMERRORCODE);
      }
      if (strcmp(modestr,"FCFCLL")==0)
        mode = GM_FCFCLL;
      else if (strcmp(modestr,"FFLLCC")==0)
        mode = GM_FFLLCC;
      else if (strcmp(modestr,"FFLCLC")==0)
        mode = GM_FFLCLC;
      else if (strcmp(modestr,"CCFFLL")==0)
        mode = GM_CCFFLL;
      else
      {
        PrintHelp("orderv",HELPITEM," (you have to specify FFCCLL or FCFCLL as mode)");
        return (PARAMERRORCODE);
      }
      break;

    case 'd' :
      /* skip leading blanks */
      dep = argv[i]+1;
      while ((*dep!='\0') && (strchr(WHITESPACE,*dep)!=NULL))
        dep++;
      break;

    case 'o' :
      /* skip leading blanks */
      dep_opt = argv[i]+1;
      while ((*dep_opt!='\0') && (strchr(WHITESPACE,*dep_opt)!=NULL))
        dep_opt++;
      break;

    case 'c' :
      /* skip leading blanks */
      cut = argv[i]+1;
      while ((*cut!='\0') && (strchr(WHITESPACE,*cut)!=NULL))
        cut++;
      break;

    case 's' :
      PutSkipFirst = TRUE;
      if (sscanf(argv[i],"s %x",&iValue)!=1)
      {
        PrintErrorMessage('E',"orderv","could not read skip pattern");
        return(PARAMERRORCODE);
      }
      SkipPat = iValue;
      break;

    case 'a' :
      levels = GM_ALL_LEVELS;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("orderv",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (mode==FALSE)
  {
    PrintErrorMessage('E',"orderv","the m option is mandatory");
    return (PARAMERRORCODE);
  }

  if (dep==NULL)
  {
    UserWrite("WARNING: no depency specified\n");
    if (dep_opt!=NULL)
    {
      UserWrite("WARNING: ignore specified options for dependency\n");
      dep_opt=NULL;
    }
  }

  if (dep!=NULL && dep_opt==NULL)
  {
    PrintErrorMessage('E',"orderv","the o option is mandatory if dopt specified");
    return (PARAMERRORCODE);
  }

  if (OrderVectors(theMG,levels,mode,PutSkipFirst,SkipPat,dep,dep_opt,cut)!=GM_OK)
  {
    PrintErrorMessage('E',"orderv","order vectors failed");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}

/****************************************************************************/
/*D
   revvecorder - revert the vector order


   DESCRIPTION:
   This command reverts the order of the vector list.

   'revvecorder [$a]'

   .  $a  - reorder all levels of the current multigrid

   KEYWORDS:
   multigrid, order, reverse
   D*/
/****************************************************************************/

static INT RevertVecOrderCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  INT i,from,to,l;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"revvecorder","no open multigrid");
    return (CMDERRORCODE);
  }

  from = to = CURRENTLEVEL(theMG);
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      from = 0;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("revvecorder",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  for (l=from; l<=to; l++)
  {
    RevertVecOrder(GRID_ON_LEVEL(theMG,l));
    UserWriteF(" [%d:rev]",l);
  }
  UserWrite("\n");

  return (OKCODE);
}

/****************************************************************************/
/*D
   lineorderv - order the vectors in lines according to the user provided dependencies


   DESCRIPTION:
   This command orders the vectors in lines according to the user provided dependencies.
   It orders the vectors of the current multigrid, calling the function
   'LineOrderVectors'.

   'lineorderv $d <dep-proc> $o <dep-proc options> $c <find-cut-proc> [$a] [$v <level>]'

   .  $d <dep-proc>          - the ordering algorithm uses this dependency procedure...
   .  $o <dep-proc options>  - ...and passes these options to it
   .  $c					  - user supplied find cut procedure
   .  $a                     - order all levels of the current multigrid
   .  $v~<level>			  - verbose level

   SEE ALSO:
   'orderv'

   KEYWORDS:
   multigrid, order, downstream, lines
   D*/
/****************************************************************************/

static INT LineOrderVectorsCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char *dep,*dep_opt,*cut;
  INT i,levels,verboselevel;
  int iValue;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"lineorderv","no open multigrid");
    return (CMDERRORCODE);
  }

  levels           = GM_CURRENT_LEVEL;
  dep              = dep_opt = cut = NULL;
  verboselevel = 0;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'd' :
      /* skip leading blanks */
      dep = argv[i]+1;
      while ((*dep!='\0') && (strchr(WHITESPACE,*dep)!=NULL))
        dep++;
      break;

    case 'o' :
      /* skip leading blanks */
      dep_opt = argv[i]+1;
      while ((*dep_opt!='\0') && (strchr(WHITESPACE,*dep_opt)!=NULL))
        dep_opt++;
      break;

    case 'c' :
      /* skip leading blanks */
      cut = argv[i]+1;
      while ((*cut!='\0') && (strchr(WHITESPACE,*cut)!=NULL))
        cut++;
      break;

    case 'a' :
      levels = GM_ALL_LEVELS;
      break;

    case 'v' :
      if (sscanf(argv[i],"v %d",&iValue)!=1)
      {
        PrintErrorMessage('E',"lineorderv","specify integer with v option");
        return (CMDERRORCODE);
      }
      verboselevel = iValue;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("lineorderv",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (dep==NULL)
  {
    PrintErrorMessage('E',"lineorderv","the d option is mandatory");
    return (PARAMERRORCODE);
  }

  if (dep_opt==NULL)
  {
    PrintErrorMessage('E',"lineorderv","the o option is mandatory");
    return (PARAMERRORCODE);
  }

  if (LineOrderVectors(theMG,levels,dep,dep_opt,cut,verboselevel)!=GM_OK)
  {
    PrintErrorMessage('E',"lineorderv","order vectors failed");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}

/****************************************************************************/
/*D
   setindex - set vector index in ascending order

   DESCRIPTION:
   'setindex' sets the vector index in ascending order.

   KEYWORDS:
   multigrid, vector, index
   D*/
/****************************************************************************/

static INT SetIndexCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *theGrid;

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"setindex","no open multigrid");
    return (CMDERRORCODE);
  }
  theGrid = GRID_ON_LEVEL(theMG,CURRENTLEVEL(theMG));

  if (l_setindex(theGrid)!=0)
  {
    PrintErrorMessage('E',"setindex","l_setindex failed");
    return (CMDERRORCODE);
  }
  else
    return (OKCODE);
}

/****************************************************************************/
/*D
   find - find (and select) a node (element) from a given position

   DESCRIPTION:
   This function finds (and selects) a node (element) from a given position,
   where some tolerance can be specified.
   It finds a node (element) on the current level of the current multigrid,
   using the functions
   'FindNodeFromPosition', 'FindElementFromPosition'
   'AddNodeToSelection', 'RemoveNodeFromSelection',
   'AddElementToSelection' and 'RemoveElementFromSelection'

   find <x> <y> <z> {$n <tol> | $v <tol> | $e} [$s]

   .  <x> <y> <z> - specify as much coordinates as the space has dimensions
   .  $n <tol>    - find a node maching the position with tolerance <tol>
   .  $v <tol>    - find a vector maching the position with tolerance <tol>
   .  $e          - find an element maching the position
   .  $s          - add the selected node (element) to the selection buffer
                 (if not specified the node is just listed)

   KEYWORDS:
   multigrid, node, element, position, find, select
   D*/
/****************************************************************************/

static INT FindCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *theGrid;
  NODE *theNode;
  VECTOR *theVector;
  ELEMENT *theElement;
  DOUBLE xc[DIM],tolc[DIM];
  INT i,j,select,isNode,isElement,isVector;

  /* following variables: keep type for sscanf */
  double x[DIM_MAX],tol;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"find","no open multigrid");
    return (CMDERRORCODE);
  }
  theGrid = GRID_ON_LEVEL(theMG,CURRENTLEVEL(theMG));

  /* check pararmeters */
  if (sscanf(argv[0],"find %lf %lf %lf",x,x+1,x+2)!=DIM)
  {
    PrintHelp("find",HELPITEM," (could not get coordinates)");
    return (PARAMERRORCODE);
  }
  for (i=0; i<DIM; i++)
    xc[i] = x[i];

  /* check options */
  select = isNode = isElement = isVector = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'n' :
      if (sscanf(argv[i],"n %lf",&tol)!=1)
      {
        PrintHelp("find",HELPITEM," (could not read tolerance)");
        return (PARAMERRORCODE);
      }
      for (j=0; j<DIM; j++)
        tolc[j] = tol;
      theNode = FindNodeFromPosition(theGrid,xc,tolc);
      if (theNode==NULL)
      {
        PrintErrorMessage('W',"find","no node is matching");
        return (CMDERRORCODE);
      }
      isNode = TRUE;
      break;

    case 'v' :
      if (sscanf(argv[i],"v %lf",&tol)!=1)
      {
        PrintHelp("find",HELPITEM," (could not read tolerance)");
        return (PARAMERRORCODE);
      }
      for (j=0; j<DIM; j++)
        tolc[j] = tol;
      theVector = FindVectorFromPosition(theGrid,xc,tolc);
      if (theVector==NULL)
      {
        PrintErrorMessage('W',"find","no vector is matching");
        return (CMDERRORCODE);
      }
      isVector = TRUE;
      break;

    case 'e' :
      theElement = FindElementFromPosition(theGrid,xc);
      if (theElement==NULL)
      {
        PrintErrorMessage('W',"find","no element is matching");
        return (CMDERRORCODE);
      }
      isElement = TRUE;
      break;

    case 's' :
      select = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("find",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (select)
  {
    if (isNode)
      if (AddNodeToSelection(theMG,theNode)!=GM_OK)
      {
        PrintErrorMessage('E',"find","selecting the node failed");
        return (CMDERRORCODE);
      }

    if (isVector)
      if (AddVectorToSelection(theMG,theVector)!=GM_OK)
      {
        PrintErrorMessage('E',"find","selecting the vector failed");
        return (CMDERRORCODE);
      }

    if (isElement)
      if (AddElementToSelection(theMG,theElement)!=GM_OK)
      {
        PrintErrorMessage('E',"find","selecting the element failed");
        return (CMDERRORCODE);
      }
  }
  else
  {
    if (isNode)
      ListNode(theMG,theNode,FALSE,FALSE,FALSE,FALSE);

    if (isVector)
      ListVector(theMG,theVector,FALSE,FALSE,LV_MOD_DEFAULT);

    if (isElement)
      ListElement(theMG,theElement,FALSE,FALSE,FALSE,FALSE);
  }

  return (OKCODE);
}

/****************************************************************************/
/*D
   select - select a node or element from a given position

   DESCRIPTION:
   This function finds (and selects) a node (element) from a given position,
   where some tolerance can be specified.
   It adds/removes nodes/elements from the selection buffer of the current
   multigrid, using the functions
   'FindNodeFromId', 'FindElementFromId'
   'AddNodeToSelection', 'RemoveNodeFromSelection',
   'AddElementToSelection' and 'RemoveElementFromSelection'

   'select $i | $c | $n {+|-} <Id> | $e {+|-} <Id>'

   .  $i				- print type and number of list members
   .  $c               - clear the selection buffer
   .  $n~{+|-}~<Id>    - add (+) or remove (-) the node with <Id> to (from) the selection buffer
   .  $e~{+|-}~<Id>    - add (+) or remove (-) the element with <Id> to (from) the selection buffer

   KEYWORDS:
   multigrid, select, element, node
   D*/
/****************************************************************************/

static INT SelectCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  NODE *theNode;
  ELEMENT *theElement;
  VECTOR *theVector;
  INT i,level;
  char c;

  /* following variables: keep type for sscanf */
  int id;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: SelectCommand(): me not in Context," \
                     " no selection of elements\n",me))
    return(OKCODE);
  }
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"select","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check options */
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'c' :
      ClearSelection(theMG);
      break;

    case 'n' :
      if (sscanf(argv[i],"n %c %d",&c,&id)!=2)
      {
        PrintErrorMessage('E',"select","could not get +/- or ID");
        return (PARAMERRORCODE);
      }
      if (c=='+')
      {
        /* search node */
        theNode = NULL;
        for (level=0; level<=TOPLEVEL(theMG); level++)
          if ((theNode=FindNodeFromId(GRID_ON_LEVEL(theMG,level),id))!=NULL)
            break;
        if (theNode==NULL)
        {
          PrintErrorMessageF('E',"select","node with ID %ld not found",(long)id);
          return (CMDERRORCODE);
        }
        if (AddNodeToSelection(theMG,theNode)!=GM_OK)
        {
          PrintErrorMessage('E',"select","selecting the node failed");
          return (CMDERRORCODE);
        }
      }
      else if (c=='-')
      {
        if (SELECTIONMODE(theMG)==nodeSelection)
          for (i=0; i<SELECTIONSIZE(theMG); i++)
          {
            theNode = (NODE *)SELECTIONOBJECT(theMG,i);
            if (ID(theNode)==id)
              break;
          }
        if (theNode==NULL)
        {
          PrintErrorMessageF('E',"select","node with ID %ld is not in selection",(long)id);
          return (CMDERRORCODE);
        }
        if (RemoveNodeFromSelection(theMG,theNode)!=GM_OK)
        {
          PrintErrorMessage('E',"select","removing the node failed");
          return (CMDERRORCODE);
        }
      }
      else
      {
        PrintErrorMessage('E',"select","specify + or - with n option");
        return (PARAMERRORCODE);
      }
      break;

    case 'e' :
      if (sscanf(argv[i],"e %c %d",&c,&id)!=2)
      {
        PrintErrorMessage('E',"select","could not get +/- or ID");
        return (PARAMERRORCODE);
      }
      if (c=='+')
      {
        /* search element */
        theElement = NULL;
        for (level=0; level<=TOPLEVEL(theMG); level++)
          if ((theElement=FindElementFromId(GRID_ON_LEVEL(theMG,level),id))!=NULL)
            break;
        if (theElement==NULL)
        {
          PrintErrorMessageF('E',"select","element with ID %ld not found",(long)id);
          return (CMDERRORCODE);
        }
        if (AddElementToSelection(theMG,theElement)!=GM_OK)
        {
          PrintErrorMessage('E',"select","selecting the element failed");
          return (CMDERRORCODE);
        }
      }
      else if (c=='-')
      {
        if (SELECTIONMODE(theMG)==elementSelection)
          for (i=0; i<SELECTIONSIZE(theMG); i++)
          {
            theElement = (ELEMENT *)SELECTIONOBJECT(theMG,i);
            if (ID(theElement)==id)
              break;
          }
        if (theElement==NULL)
        {
          PrintErrorMessageF('E',"select","element with ID %ld is not in selection",(long)id);
          return (CMDERRORCODE);
        }
        if (RemoveElementFromSelection(theMG,theElement)!=GM_OK)
        {
          PrintErrorMessage('E',"select","removing the element failed");
          return (CMDERRORCODE);
        }
      }
      else
      {
        PrintErrorMessage('E',"select","specify + or - with n option");
        return (PARAMERRORCODE);
      }
      break;

    case 'v' :
      if (sscanf(argv[i],"v %c %d",&c,&id)!=2)
      {
        PrintErrorMessage('E',"select","could not get +/- or ID");
        return (PARAMERRORCODE);
      }
      if (c=='+')
      {
        /* search vector */
        theVector = NULL;
        for (level=0; level<=TOPLEVEL(theMG); level++)
          if ((theVector=FindVectorFromIndex(GRID_ON_LEVEL(theMG,level),id))!=NULL)
            break;
        if (theVector==NULL)
        {
          PrintErrorMessageF('E',"select","vector with ID %ld not found",(long)id);
          return (CMDERRORCODE);
        }
        if (AddVectorToSelection(theMG,theVector)!=GM_OK)
        {
          PrintErrorMessage('E',"select","selecting the vector failed");
          return (CMDERRORCODE);
        }
      }
      else if (c=='-')
      {
        if (SELECTIONMODE(theMG)==vectorSelection)
          for (i=0; i<SELECTIONSIZE(theMG); i++)
          {
            theVector = (VECTOR *)SELECTIONOBJECT(theMG,i);
            if (ID(theVector)==id)
              break;
          }
        if (theVector==NULL)
        {
          PrintErrorMessageF('E',"select","vector with ID %ld is not in selection",(long)id);
          return (CMDERRORCODE);
        }
        if (RemoveVectorFromSelection(theMG,theVector)!=GM_OK)
        {
          PrintErrorMessage('E',"select","removing the vector failed");
          return (CMDERRORCODE);
        }
      }
      else
      {
        PrintErrorMessage('E',"select","specify + or - with n option");
        return (PARAMERRORCODE);
      }
      break;

    case 'i' :
      if (SELECTIONSIZE(theMG)==0)
        UserWrite("nothing selected\n");
      else switch (SELECTIONMODE(theMG))
        {
        case elementSelection :
          UserWriteF("%d elements selected (use for example 'elist $s')\n",SELECTIONSIZE(theMG));
          break;
        case nodeSelection :
          UserWriteF("%d nodes selected (use for example 'nlist $s')\n",SELECTIONSIZE(theMG));
          break;
        case vectorSelection :
          UserWriteF("%d vectors selected (use for example 'vmlist $s')\n",SELECTIONSIZE(theMG));
          break;
        default :
          UserWrite("unknown selection type\n");
        }
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("select",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  return (OKCODE);
}

/****************************************************************************/
/*D
   extracon - display number of (and delete) extra connections

   DESCRIPTION:
   This command displays the number extra connections.(and deletes them if specified).
   Extra connection extend the usual sparsity pattern.

   'extracon [$d]'

   .  $c - also check the connections

   KEYWORDS:
   multigrid, matrices, connections, pattern, delete, remove
   D*/
/****************************************************************************/

static INT ExtraConnectionCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *theGrid;
  VECTOR *vec;
  MATRIX *mat;
  INT Delete,i,nextra,nc;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"extracon","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check options */
  Delete = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'd' :
      Delete = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("extracon",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }
  theGrid = GRID_ON_LEVEL(theMG,CURRENTLEVEL(theMG));

  /* count extra connections on current level */
  nextra = 0;
  for (vec=FIRSTVECTOR(theGrid); vec!=NULL; vec=SUCCVC(vec))
    for (mat=MNEXT(VSTART(vec)); mat!=NULL; mat=MNEXT(mat))
      if (CEXTRA(MMYCON(mat)))
        nextra++;
  nextra /= 2;                  /* have been counted twice */

  nc = NC(theGrid);
        #ifdef ModelP
  nextra = UG_GlobalSumINT(nextra);
  nc = UG_GlobalSumINT(nc);
        #endif

  UserWriteF("%d extra connections on level %d (total %d)\n",
             (int)nextra,(int)CURRENTLEVEL(theMG),(int)NC(theGrid));

  SetStringValue(":extraconratio",nextra/((DOUBLE)nc));

  if (Delete)
  {
    if (DisposeExtraConnections(theGrid)!=GM_OK)
    {
      PrintErrorMessage('E',"extracon","deleting extra connections failed");
      return (CMDERRORCODE);
    }
    UserWrite("...deleted\n");
  }

  return (OKCODE);
}


/****************************************************************************/
/*D
   check - check consistency of the data structure

   DESCRIPTION:
   This command checks consistency of the data structure, using
   the function 'CheckGrid'. Dependent on the options are called inside
   'CheckGrid' one or more of 'CheckGeometry' 'CheckAlgebra', 'CheckLists'
   and 'CheckInterfaces'. Default check is 'CheckGeometry'.

   'check {$a | $g | $c | $l | $i | $w}* [$n]'

   .  $a - all possible checks are done
   .  $g - check the geometric part of data structures (default)
   .  $c - also check the algebraic part of data structures
   .  $l - also check the lists of objects and counters of a grid
   .  $i - also check interfaces (only parallel version)
   .  $b - also check boundary value problems
   .  $n - also check the numerical part of data structures
   .  $w - print a list of all control words of object types with control entries to stdout

   KEYWORDS:
   multigrid, check, consistency, data structure, algebra, counters, interfaces
   D*/
/****************************************************************************/

static INT CheckCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *theGrid;
  INT checkgeom,checkalgebra,checklists,checkbvp,checknp;
        #ifdef ModelP
  INT checkif;
        #endif
  INT level,err,i;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"check","no open multigrid");
    return (CMDERRORCODE);
  }

  /* set default options */
  checkgeom = TRUE;
  checkalgebra = checklists = checkbvp = checknp = FALSE;
        #ifdef ModelP
  checkif = FALSE;
        #endif

  /* read options */
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      checkgeom = checkalgebra = checklists = checknp = TRUE;
                                #ifdef ModelP
      checkif = TRUE;
                                #endif
      break;

    case 'g' :
      checkgeom = TRUE;
      break;

    case 'c' :
      checkalgebra = TRUE;
      break;

    case 'l' :
      checklists = TRUE;
      break;

                        #ifdef ModelP
    case 'i' :
      checkif = TRUE;
      break;
                        #endif

    case 'b' :
      checkbvp = TRUE;
      break;

    case 'n' :
      checknp = TRUE;
      break;

    case 'w' :
      ListAllCWsOfAllObjectTypes(UserWriteF);
      break;

    default :
      if (!checknp) {
        sprintf(buffer,"(invalid option '%s')",argv[i]);
        PrintHelp("check",HELPITEM,buffer);
        return (PARAMERRORCODE);
      }
    }
  err = 0;

  /* check BVP if */
  if (checkbvp==TRUE)
    if (BVP_Check (MG_BVP(theMG)))
      err++;

  for (level=0; level<=TOPLEVEL(theMG); level++)
  {
    theGrid = GRID_ON_LEVEL(theMG,level);
    UserWriteF("[%d:",(int)level);

                #ifndef ModelP
    if (CheckGrid(theGrid,checkgeom,checkalgebra,checklists)!=GM_OK)
                #else
    if (CheckGrid(theGrid,checkgeom,checkalgebra,checklists,checkif)!=GM_OK)
                #endif
      err++;

    UserWrite("]\n");
  }
  UserWrite("\n");

  if (checknp)
    if (CheckNP(theMG,argc,argv))
      err++;

  if (err)
    return (CMDERRORCODE);
  else
    return (OKCODE);
}

/****************************************************************************/
/*D
   quality - calculate minimal and maximal angle of specified elements

   DESCRIPTION:
   This command calculates the minimal and maximal angle
   between sides of the specified elements
   and lists elements with angle < or > given angles.
   It calls the functions 'QualityElement'.

   'quality $a | $s | {$i <fromID> [<toID>]} [$< <angle>] [$> <angle>]'

   .    $a          - check angles of all elements in the multigrid
   .    $s          - check angles of the selected elements
   .    $i          - check angles of elements with an ID in the range <fromID> through <toID>
   .n                 if <fromID> is omitted only the element with <fromID> is listed

   .    $<~<angle>  - print info for all elements the minangle of which is < <angle>
   .    $>~<angle>  - print info for all elements the maxangle of which is > <angle>

     (angles in degree 0-360)

   KEYWORDS:
   multigrid, element, quality, angles, find
   D*/
/****************************************************************************/

/****************************************************************************/
/*
   QualityElement - calculate minimal and maximal angle of an element

   SYNOPSIS:
   static void QualityElement (MULTIGRID *theMG, ELEMENT *theElement);

   PARAMETERS:
   .  theMG - pointer to multigrid
   .  theElement - pointer to Element

   DESCRIPTION:
   This function calculates the minimal and the maximal angle of elements
   and lists elements with angle < or > given angles.

   RETURN VALUE:
   void
 */
/****************************************************************************/

INT QualityElement (MULTIGRID *theMG, ELEMENT *theElement)
{
  INT error;

  min = 360.0;
  max = 0.0;

  if ((error=MinMaxAngle(theElement,&min,&max))!=GM_OK)
    return(error);
  minangle = MIN(min,minangle);
  maxangle = MAX(max,maxangle);
  if ((lessopt && (min<themin)) && (greateropt && (max>themax)))
  {
    UserWrite(minmaxtext);
    ListElement(theMG,theElement,FALSE,FALSE,FALSE,FALSE);
    if (selectopt) AddElementToSelection(theMG,theElement);
  }
  else if (lessopt && (min<themin))
  {
    UserWrite(mintext);
    ListElement(theMG,theElement,FALSE,FALSE,FALSE,FALSE);
    if (selectopt) AddElementToSelection(theMG,theElement);
  }
  else if (greateropt && (max>themax))
  {
    UserWrite(maxtext);
    ListElement(theMG,theElement,FALSE,FALSE,FALSE,FALSE);
    if (selectopt) AddElementToSelection(theMG,theElement);
  }

  return(0);
}

static INT QualityCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *theGrid;
  ELEMENT *theElement;
  INT error,i,fromE,toE,res,mode;

  /* following variables: keep type for sscanf */
  double angle;
  long f,t;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"quality","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check options */
  lessopt = greateropt = selectopt = mode = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      if (mode!=FALSE)
      {
        PrintErrorMessage('E',"quality","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_ALL;
      break;

    case 'i' :
      if (mode!=FALSE)
      {
        PrintErrorMessage('E',"quality","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_ID;
      res = sscanf(argv[i]," i %ld %ld",&f,&t);
      fromE = f;
      toE   = t;
      if (res<1)
      {
        PrintErrorMessage('E',"quality","specify at least one id with the i option");
        return (PARAMERRORCODE);
      }
      else if (res==1)
        toE = fromE;
      else if (fromE>toE)
      {
        PrintErrorMessage('E',"quality","from ID > to ID");
        return (PARAMERRORCODE);
      }
      break;

    case '<' :
      lessopt = TRUE;
      if (sscanf(argv[i],"< %lf",&angle)!=1)
      {
        PrintErrorMessage('E',"quality","could not get angle of < option");
        return (CMDERRORCODE);
      }
      themin = angle;
      break;

    case '>' :
      greateropt = TRUE;
      if (sscanf(argv[i],"> %lf",&angle)!=1)
      {
        PrintErrorMessage('E',"quality","could not get angle of > option");
        return (CMDERRORCODE);
      }
      themax = angle;
      break;

    case 's' :
      if (mode!=FALSE)
      {
        PrintErrorMessage('E',"quality","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_SELECTION;
      break;

    case 'S' :
      selectopt = TRUE;
      ClearSelection(theMG);
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("quality",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  sprintf(mintext," < %g: ",(float)themin);
  sprintf(maxtext," > %g: ",(float)themax);
  sprintf(minmaxtext," < %g and > %g: ",(float)themin,(float)themax);

  minangle =      MAX_D;
  maxangle = -MAX_D;

  error=0;
  switch (mode)
  {
  case DO_ID :
    for (theGrid=GRID_ON_LEVEL(theMG,0); theGrid!=NULL; theGrid=UPGRID(theGrid))
      for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
        if ((ID(theElement)>=fromE) && (ID(theElement)<=toE))
          if ((error=QualityElement(theMG,theElement))!=0)
            break;
    break;

  case DO_ALL :
    for (theGrid=GRID_ON_LEVEL(theMG,0); theGrid!=NULL; theGrid=UPGRID(theGrid))
      for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
        if ((error=QualityElement(theMG,theElement))!=0)
          break;
    break;

  case DO_SELECTION :
    if (SELECTIONMODE(theMG)==elementSelection)
      for (i=0; i<SELECTIONSIZE(theMG); i++)
        if ((error=QualityElement(theMG,(ELEMENT *)SELECTIONOBJECT(theMG,i)))!=0)
          break;
    break;

  default :
    PrintErrorMessage('E',"quality","specify one option of a, s or i");
    return (PARAMERRORCODE);
  }

  if (error)
  {
    PrintErrorMessage('E',"quality","error in QualityElement/MinMaxAngle");
    return (CMDERRORCODE);
  }

  UserWriteF(" min angle = %20.12f\n max angle = %20.12f\n",(float)minangle,(float)maxangle);

  return(OKCODE);
}

/****************************************************************************/
/*D
   makegrid - generate grid

   2D advancing front generator:

   DESCRIPTION:
   This command generates the grid. First, the command bnodes must be called.
   It reads the environment variables ':gg:RelRasterSize', ':gg:h_global',
   ':gg:searchconst', ':gg:angle', ':gg:epsi'.

   'makegrid ${W|w|K|k} [$E] [$h <val>] [$m <no>] [$S <search>] [$A <angle>] [$d <subdom>]'

   .  ${W|w|K|k}	- W resp. K are using the quadtree accellerator,
   .  $W~resp.~w	- use the angle criterion,
   .  $K~resp.~k	- use the edge criterion
   .  $E                        - grid generator tries to create equilateral triangles (edgelength h)
   .n                                   default: isosceles triangles (height h)
   .  $h~<val>  - mesh size
   .  $m~<no>		- id of mesh size coefficient function
   .  $S~<search>	- search radius (experts only)
   .  $A~<angle>	- try to avoid angle smaller than <angle>
   .  $d~<subdom>	- restrict grid generation to subdomain with id <subdom>

   EXAMPLE:
   'makegrid $k $h 1.0;'


   3D advancing front generator (by J. Schoeberl):

   DESCRIPTION:
   This command invokes the advancing front tetrahedral grid generator.

   'makegrid [$s] [$h <meshsize>] [$d]'

   .   $s				- smooth generated grid
   .   $h <meshsize>	- preferred meshsize (default 1.0)
   .   $d				- ?

   KEYWORDS:
   multigrid, generate, create, mesh, net, grid, coarse, advancing front
   D*/
/****************************************************************************/

static INT MakeGridCommand  (INT argc, char **argv)
{
  MULTIGRID *theMG;
  INT i,Single_Mode,display;
  MESH *mesh;
  INT MarkKey;
#ifdef __TWODIM__
  CoeffProcPtr coeff;
  GG_ARG args;
  GG_PARAM params;
  INT smooth;
  long ElemID,m;
  int iValue;
  double tmp;
#endif
#if defined __THREEDIM__ && defined _NETGEN
  INT smooth, from, to, prism, save;
  DOUBLE h,vol_h;
  INT coeff;
#endif

  /* get current multigrid */
  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"makegrid","no open multigrid");
    return (CMDERRORCODE);
  }
        #ifdef ModelP
  if (me!=master)
  {
    if (FixCoarseGrid(theMG)) return (CMDERRORCODE);
    return (OKCODE);
  }
        #endif
  if ((CURRENTLEVEL(theMG)!=0)||(TOPLEVEL(theMG)!=0))
  {
    PrintErrorMessage('E',"MakeGridCommand","only for a multigrid with exactly one level a grid can be generated");
    RETURN(GM_ERROR);
  }
  MarkKey = MG_MARK_KEY(theMG);
  if (MG_COARSE_FIXED(theMG)) {
    MG_COARSE_FIXED(theMG) = FALSE;
    MarkTmpMem(MGHEAP(theMG),&MarkKey);
    MG_MARK_KEY(theMG) = MarkKey;
    if ((MGNDELEMPTRARRAY(theMG) =
           (ELEMENT***)GetTmpMem(MGHEAP(theMG),NDELEM_BLKS_MAX*sizeof(ELEMENT**),MarkKey))==NULL)
    {
      ReleaseTmpMem(MGHEAP(theMG),MarkKey);
      PrintErrorMessage('E',"makegrid","ERROR: could not allocate memory from the MGHeap");
      return (CMDERRORCODE);
    }
    for (i=0; i<NDELEM_BLKS_MAX; i++) MGNDELEMBLK(theMG,i) = NULL;
  }

  Single_Mode = 0;
  display = 0;

  /* check options */
#ifdef __TWODIM__
  args.doanimate = args.doupdate = args.dostep = args.equilateral = args.plotfront = args.printelem = args.doangle = args.doEdge = args.doAngle = args.doConstDel =  NO;
  args.doedge = YES;
  ElemID = -1;
#endif

  if (DisposeGrid(GRID_ON_LEVEL(theMG,0)))
  {
    UserWriteF("makegrid: cannot dispose coarse grid\n");
    DisposeMultiGrid(theMG);
    return (CMDERRORCODE);
  }

  if (CreateNewLevel(theMG,0)==NULL)
  {
    UserWriteF("makegrid: cannot create new level\n");
    DisposeMultiGrid(theMG);
    return (CMDERRORCODE);
  }
  mesh = BVP_GenerateMesh (MGHEAP(theMG),MG_BVP(theMG),argc,argv,MarkKey);
  if (mesh == NULL)
  {
    UserWriteF("makegrid: cannot generate boundary mesh\n");
    ReleaseTmpMem(MGHEAP(theMG),MarkKey);
    return (CMDERRORCODE);
  }
  else
    InsertMesh(theMG,mesh);

  if (mesh->nElements == NULL)
  {
#ifdef __TWODIM__
    coeff = NULL;
    params.h_global = .0;
    params.CheckCos = cos(PI/18.0);
    params.searchconst = 0.2;
    smooth = 5;
    for (i=1; i<argc; i++)
      switch (argv[i][0])
      {
      case 'a' :
        args.doanimate = YES;
        break;
      case 'u' :
        args.doupdate = YES;
        break;
      case 's' :
        args.dostep = YES;
        break;
      case 'f' :
        args.plotfront = YES;
        break;
      case 'p' :
        args.printelem = YES;
        break;
      case 'E' :
        args.equilateral = YES;
        break;
      case 'e' :
        if (sscanf(argv[i],"e %ld",&ElemID)!=1)
        {
          PrintHelp("makegrid",HELPITEM," (could not read <element id>)");
          return (PARAMERRORCODE);
        }
        break;
      case 'k' :
        args.doedge = YES;
        break;
      case 'w' :
        args.doangle = YES;
        break;
      case 'K' :
        args.doEdge = YES;
        break;
      case 'W' :
        args.doAngle = YES;
        break;
      case 'C' :
        args.doConstDel = YES;
        args.doedge = NO;
        break;
      case 'm' :
        if (sscanf(argv[i],"m %ld",&m)!=1)
        {
          PrintHelp("makegrid",HELPITEM," (could not read <element id>)");
          return (PARAMERRORCODE);
        }
        coeff = MG_GetCoeffFct(theMG,m);
        break;
      case 'h' :
        if (sscanf(argv[i],"h %lf",&tmp)!=1)
        {
          PrintHelp("makegrid",HELPITEM," (could not read <element id>)");
          return (PARAMERRORCODE);
        }
        if (tmp > 0) params.h_global = tmp;
        break;
      case 'A' :
        if (sscanf(argv[i],"A %lf",&tmp)!=1)
        {
          PrintHelp("makegrid",HELPITEM," (could not read <element id>)");
          return (PARAMERRORCODE);
        }
        if ((tmp > 0) && (tmp < 90)) params.CheckCos = cos(tmp*PI/180.0);
        break;
      case 'S' :
        if (sscanf(argv[i],"S %lf",&tmp)!=1)
        {
          PrintHelp("makegrid",HELPITEM," (could not read <element id>)");
          return (PARAMERRORCODE);
        }
        if ((tmp > 0) && (tmp < 1.0)) params.searchconst = tmp;
        break;
      case 'd' :
        if (sscanf(argv[i],"d %d",&iValue)!=1) break;
        Single_Mode = iValue;
        break;
      case 'D' :
        if (sscanf(argv[i],"D %d",&iValue)!=1) break;
        display = iValue;
        break;
      case 'g' :
        if (sscanf(argv[i],"g %d",&iValue)!=1) break;
        smooth = iValue;
        break;
      default :
        break;
      }
    params.epsi = params.h_global * 0.125;

    if (GenerateGrid(theMG, &args, &params, mesh, coeff, Single_Mode, display))
    {
      PrintErrorMessage('E',"makegrid","execution failed");
      ReleaseTmpMem(MGHEAP(theMG),MarkKey);
      return (CMDERRORCODE);
    }
    if (SmoothMultiGrid(theMG,smooth,GM_KEEP_BOUNDARY_NODES)!=GM_OK)
    {
      PrintErrorMessage('E',"makegrid","failed smoothing the multigrid");
      return (CMDERRORCODE);
    }
    if (CheckOrientationInGrid (GRID_ON_LEVEL(theMG,0)))
    {
      PrintErrorMessage('E',"makegrid","orientation wrong");
      return (CMDERRORCODE);
    }
#endif

#if defined __THREEDIM__ && defined _NETGEN
    if (ReadArgvINT("s",&smooth,argc,argv)) smooth = 0;
    if (ReadArgvDOUBLE("h",&h,argc,argv)) h = 1.0;
    if(h<0)
      if (ReadArgvINT("c",&coeff,argc,argv)) coeff = 0;

    if (ReadArgvINT("s",&save,argc,argv))
      save = 0;
    if (ReadArgvINT("p",&prism,argc,argv))
      prism = 0;
    if (ReadArgvINT("f",&from,argc,argv))
      from = 1;
    if(from<1)
      from = 1;
    if (ReadArgvINT("t",&to,argc,argv))
      to = theMG->theBVPD.nSubDomains;
    if(to>theMG->theBVPD.nSubDomains)
      to = theMG->theBVPD.nSubDomains;

    if (ReadArgvDOUBLE("v",&vol_h,argc,argv))
      vol_h = h;

    if (GenerateGrid3d(theMG,mesh,vol_h,smooth,ReadArgvOption("d",argc,argv),coeff, from, to, prism, save, argc, argv))
    {
      PrintErrorMessage('E',"makegrid","execution failed");
      ReleaseTmpMem(MGHEAP(theMG),MarkKey);
      return (CMDERRORCODE);
    }
#endif
  }

  if (FixCoarseGrid(theMG)) return (CMDERRORCODE);
  InvalidatePicturesOfMG(theMG);
  InvalidateUgWindowsOfMG(theMG);

  return (OKCODE);
}

/****************************************************************************/
/*D
   status - show status about (parallel) multigrid

   DESCRIPTION:
   This command outputs some statistics about red,green yellow element
   distribution and some loadbalacing measures for parallel.

   .  ${W|w|K|k}	- W resp. K are using the quadtree accellerator,

   KEYWORDS:
   multigrid, loadbalancing, mesh, net, grid, adaptive refinement, estimator
   D*/
/****************************************************************************/

static INT StatusCommand  (INT argc, char **argv)
{
  MULTIGRID       *theMG;
  INT i,grid,green,load,verbose;

  /* get current multigrid */
  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"status command","no open multigrid");
    return (CMDERRORCODE);
  }
  grid = green = load = 0;
  verbose = 1;

  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      grid = 1;
      green = 1;
                                #ifdef ModelP
      load = 1;
                                #endif
      break;
    case 'g' :
      green = 1;
      break;
                        #ifdef ModelP
    case 'l' :
      load = 1;
      sscanf(argv[i],"l %d",&load);
      break;
                        #endif
    case 'm' :
      grid = 1;
      break;
    default :
      break;
    }

  if (MultiGridStatus(theMG,grid,green,load,verbose) != 0)
  {
    PrintErrorMessage('E',"GridStatus()","execution failed");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}

/****************************************************************************/
/*D
   cadconvert - convert predefined CADgrid

   DESCRIPTION:
   This command converts a predefined CADgrid to an UG-multigrid.
   The complete boundary descriptions are created automatically.
   Additionally boundary conditions can be chosen from a boundary condition class library.

   'cadconvert $<filename> $h<heapsize>'

   .  $ <filename>  - filename = name of CADOutputfile ("*.ans", ANSYS/PREP7-Format)
   .  $f <format>   - one of the enroled formats matching with <problem>
   .  $h <heapsize> - the heapsize to be allocated


   EXAMPLE:
   'cadconvert $ wuerfel.ans $h 12000;'

   KEYWORDS:
   multigrid, generate, create, mesh, net, grid, coarse, CAD, ANSYS, PREP7
   D*/
/****************************************************************************/

#if defined(CAD) && defined(__THREEDIM__)
static INT CADGridConvertCommand(INT argc, char **argv)
{
  MULTIGRID *theMG;
  char CADOutputFileName[NAMESIZE];

  char Format[NAMESIZE];
  char *theFormat;

  unsigned long heapSize;
  INT i,hopt;

  /* get CADfile name */
  if ((sscanf(argv[0],expandfmt(CONCAT3(" cadconvert %",NAMELENSTR,"[ -~]")),CADOutputFileName)!=1) || (strlen(CADOutputFileName)==0))
    sprintf(CADOutputFileName,"untitled-%d",(int)untitledCounter++);

  /* get problem, domain and format */
  theFormat = NULL;
  heapSize = 0;
  hopt = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'f' :
      if (sscanf(argv[i],expandfmt(CONCAT3("f %",NAMELENSTR,"[ -~]")),Format)!=1)
      {
        PrintHelp("open",HELPITEM," (cannot read format specification)");
        return(PARAMERRORCODE);
      }
      theFormat = Format;
      break;

    case 'h' :
      if (sscanf(argv[i],"h %lu",&heapSize)!=1)
      {
        PrintHelp("new",HELPITEM," (cannot read heapsize specification)");
        return(PARAMERRORCODE);
      }
      hopt = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("new",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (!(hopt))
  {
    PrintHelp("new",HELPITEM," (the d, p, f and h arguments are mandatory)");
    return(PARAMERRORCODE);
  }

  /* check options */
  theMG = ConvertCADGrid(theFormat, CADOutputFileName, heapSize);
  if (theMG == NULL)
  {
    PrintErrorMessage('E',"cadconvert","execution failed");
    return (CMDERRORCODE);
  }

  if (SetCurrentMultigrid(theMG)!=0)
    return (CMDERRORCODE);



  return (OKCODE);
}
#endif

/****************************************************************************/
/*D
   grape - switch to interactive grape mode

   DESCRIPTION:
   This command switches to interactive grape mode. Quitting grape returns to ug.

   'screensize'

   KEYWORDS:
   multigrid, graphics, GRAPE, plot
   D*/
/****************************************************************************/

static INT CallGrapeCommand (INT argc, char **argv)
{
  MULTIGRID *theCurrMG;

  /* see if multigrid exists */
  theCurrMG = currMG;
  if (theCurrMG==NULL)
  {
    UserWrite("cannot call grape without multigrid\n");
    return (CMDERRORCODE);
  }

  /* call grape */
  if (CallGrape(theCurrMG)) return (CMDERRORCODE);

  return (OKCODE);
}


#ifdef _COVISE
/****************************************************************************/
/*D
   covise - enable covise interface

   DESCRIPTION:
   This command enables docking of a Covise user interface by a remote
   workstation.

   .  hostname		- hostname of workstation which runs Covise

   KEYWORDS:
   multigrid, graphics, Covise, plot
   D*/
/****************************************************************************/

static INT CoviseCommand (INT argc, char **argv)
{
  MULTIGRID *theCurrMG;
  char hostname[NAMESIZE];

  /* see if multigrid exists */
  theCurrMG = currMG;
  if (theCurrMG==NULL)
  {
    UserWrite("cannot start covise without multigrid\n");
    return (CMDERRORCODE);
  }


  if (sscanf(argv[0],"covise %s",hostname)!=1)
  {
    UserWrite("covise: specify hostname to connect to!\n");
    return(PARAMERRORCODE);
  }

  /* call covise */
  if (ConnectCovise(theCurrMG, hostname)) return (CMDERRORCODE);

  return (OKCODE);
}
#endif


/****************************************************************************/
/*D
   screensize - print the size of the monitor screen in pixels

   DESCRIPTION:
   This command prints the size of the monitor screen in pixels.
   It prints the size in pixels of the screen (if there) on the shell
   and in the variables ':screensize:width', ':screensize:height'

   'screensize'

   KEYWORDS:
   screen, size, width, height
   D*/
/****************************************************************************/

static INT ScreenSizeCommand (INT argc, char **argv)
{
  INT size[2];

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  NO_OPTION_CHECK(argc,argv);

  if (GetScreenSize(size)==FALSE)
  {
    PrintErrorMessage('W',"screensize","there is no monitor");
    return (OKCODE);
  }

  UserWriteF(" screen width: %d, screen height: %d\n",(int)size[0], (int)size[1]);

  if (    (SetStringValue(":screensize:width",size[0])!=0)
          ||      (SetStringValue(":screensize:height",size[1])!=0))
  {
    PrintErrorMessage('E',"screensize","could not set :screensize:width or :screensize:height");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}

/****************************************************************************/
/*D
   openwindow - open a new window

   DESCRIPTION:
   This command opens an ug-window on an outputdevice
   (this will be the current window then).
   It calls the function 'CreateUGWindow'.

   'openwindow <h> <v> <dh> <dv> [$d <output device>] [$n <window name>]'

   .  <h>~<v>                - the lower left corner of the plotting region in the 'standardRefSys'
   .  <dh>~<dv>              - the width and height resp. of the plotting region of the window
   .  $d~<output~device>     - specify the name of an output device (default: screen)
   .  $n~<window~name>       - optionally you can specify the window name
   .  $r~[0|1]               - if not default device: 0: replace (default), 1: rename existing graphics file (using modification date)

   SEE ALSO:
   'closewindow', 'openpicture', 'closepicture'

   KEYWORDS:
   graphics, plot, window, open, create
   D*/
/****************************************************************************/

static INT OpenWindowCommand (INT argc, char **argv)
{
  OUTPUTDEVICE *theOutDev;
  UGWINDOW *theWin;
  char devname[NAMESIZE],winname[NAMESIZE];
  INT i,rename,res;
  int ropt;

  /* following variables: keep type for sscanf */
  int x,y,w,h;
        #ifdef ModelP
  if (!CONTEXT(me))
    PRINTDEBUG(ui,0,("%2d: OpenWindowCommand(): me not in Context," \
                     "  no window structure allocated\n",me))
    /* Note: it is forbidden that a part of the processors leave this function premature
       since in the subsequent CreateUgWindow communication occures, whis requires
       all processors to participate. Thus ensure that all or none processor leaves.
       Christian Wrobel 990329 */
    for(i=0; i<procs; i++)
      if(!CONTEXT(i))                   /* each procesor has the same context; thus no communication is required */
        return(CMDERRORCODE);
        #endif

  /* scan parameters */
  if (sscanf(argv[0],"openwindow %d %d %d %d",&x,&y,&w,&h)!=4)
  {
    PrintHelp("openwindow",HELPITEM," could not get all mandatory parameters");
    return (PARAMERRORCODE);
  }

  /* check options */
  theOutDev  = GetDefaultOutputDevice();
  winname[0] = '\0';
  rename = 0;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'd' :
      if (sscanf(argv[i],expandfmt(CONCAT3("d %",NAMELENSTR,"[a-zA-Z0-9_-]")),devname)!=1)
      {
        PrintErrorMessage('E',"openwindow","specify device name with d option");
        return (PARAMERRORCODE);
      }
      if ((theOutDev=GetOutputDevice(devname))==NULL)
      {
        PrintErrorMessageF('E',"openwindow","there is no device named '%s'",devname);
        return (PARAMERRORCODE);
      }
      break;

    case 'n' :
      if (sscanf(argv[i],expandfmt(CONCAT3("n %",NAMELENSTR,"[a-zA-Z0-9_.-]")),winname)!=1)
      {
        PrintErrorMessage('E',"openwindow","specify window name with n option");
        return (PARAMERRORCODE);
      }
      break;

    case 'r' :
      res = sscanf(argv[i]," r %d",&ropt);
      if (res==0 || (res==1 && ropt==1)) rename = 1;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("openwindow",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (strlen(winname)==0)
    sprintf(winname,"window%d",(int) wincounter++);


        #ifdef ModelP
  if (me == master)
        #endif
  if (theOutDev==NULL) {
    PrintErrorMessage('E',"openwindow","no output device");
    return (PARAMERRORCODE);
  }

  if ((theWin=CreateUgWindow(theOutDev,winname,rename,x,y,w,h))==NULL)
  {
    PrintErrorMessage('E',"openwindow","failed to open a window");
    return (CMDERRORCODE);
  }

  SetCurrentUgWindow(theWin);

  return (OKCODE);
}

/****************************************************************************/
/*D
   closewindow - close the current ug-window

   DESCRIPTION:
   This command closes one (or all) ug-window(s)
   (including the pictures residing there, of course),
   calling the functions 'DisposeMultiGrid' and 'DisposePicture'.

   'closewindow [$n <window name> | $a]'

   .  $n~<window~name>  - close the window with the specified name
   .n                        (default: the current window)
   .  $a                - close all open windows

   KEYWORDS:
   graphics, plot, window, close, remove

   SEE ALSO:
   'openwindow', 'openpicture', 'closepicture'
   D*/
/****************************************************************************/

static INT CloseWindowCommand (INT argc, char **argv)
{
  UGWINDOW *theWin, *currWin;
  PICTURE *thePic,*currPic;
  char winname[NAMESIZE];
  INT i,aopt;

        #ifdef ModelP
  /*if (me!=master) return (OKCODE); since OpenWindow allocates structures for all procs, CloseWindow must deallocate for all those (and not only for master). Christian Wrobel 990326 */
  if (!CONTEXT(me))
    PRINTDEBUG(ui,0,("%2d: CloseWindowCommand(): me not in Context," \
                     "  no window structure closed\n",me))
    /* Note: it is forbidden that a part of the processors leave this function premature
       since all allocated structures must be deallocated.
       Thus ensure that all or none processor leaves.
       Christian Wrobel 990329 */
    for(i=0; i<procs; i++)
      if(!CONTEXT(i))                   /* each procesor has the same context; thus no communication is required */
        return(CMDERRORCODE);
        #endif

  currWin = GetCurrentUgWindow();
  theWin = currWin;

  /* check options */
  aopt = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'n' :
      if (sscanf(argv[i],expandfmt(CONCAT3("n %",NAMELENSTR,"[a-zA-Z0-9_.]")),winname)!=1)
      {
        PrintErrorMessage('E',"closewindow","specify a window name with n option");
        return (PARAMERRORCODE);
      }
      if ((theWin=GetUgWindow(winname))==NULL)
      {
        PrintErrorMessageF('W',"closewindow","there is no window named '%s'",winname);
        return (OKCODE);
      }
      break;

    case 'a' :
      aopt = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("closewindow",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (theWin==NULL)
  {
    PrintErrorMessage('W',"closewindow","there's no current window");
    return (OKCODE);
  }
  if (theWin == currWin) currWin = NULL;
  currPic = GetCurrentPicture();

  if (aopt)
  {
    while((theWin=GetFirstUgWindow())!=NULL)
    {
      /* first dispose all pictures of that window */
      while((thePic=GetFirstPicture(theWin))!=NULL)
      {
        if (thePic==currPic)
          SetCurrentPicture(NULL);
        if (DisposePicture(thePic))
        {
          PrintErrorMessage('E',"closewindow","could not close a picture of that window");
          return (CMDERRORCODE);
        }
      }

      /* now dispose the window itself */
      if (DisposeUgWindow(theWin))
      {
        PrintErrorMessage('E',"closewindow","could not close the window");
        return (CMDERRORCODE);
      }
    }

    currWin = GetFirstUgWindow();

    SetCurrentUgWindow(currWin);
    if (currWin!=NULL)
      SetCurrentPicture(GetFirstPicture(currWin));
    else
      SetCurrentPicture(NULL);

    return (OKCODE);
  }

  /* first dispose all pictures of that window */
  while ((thePic=GetFirstPicture(theWin))!=NULL)
  {
    if (thePic==currPic)
      SetCurrentPicture(NULL);
    if (DisposePicture(thePic))
    {
      PrintErrorMessage('E',"closewindow","could not close a picture of that window");
      return (CMDERRORCODE);
    }
  }

  /* now dispose the window itself */
  if (DisposeUgWindow(theWin)!=0)
  {
    PrintErrorMessage('E',"closewindow","could not close the window");
    return (CMDERRORCODE);
  }

  currWin = GetFirstUgWindow();
  SetCurrentUgWindow(currWin);
  if (currWin!=NULL)
    SetCurrentPicture(GetFirstPicture(currWin));
  else
    SetCurrentPicture(NULL);

  return (OKCODE);
}

/****************************************************************************/
/*D
   setcurrwindow - set the current window

   DESCRIPTION:
   This command makes a window the current window.
   It calls the function 'GetUGWindow'.

   'setcurrwindow <window name>'

   .  <window~name> - name of a window

   KEYWORDS:
   graphics, plot, window, current, active

   SEE ALSO:
   'openwindow'
   D*/
/****************************************************************************/

static INT SetCurrentWindowCommand (INT argc, char **argv)
{
  UGWINDOW *theWin;
  char winname[NAMESIZE];

  NO_OPTION_CHECK(argc,argv);

  /* get window name */
  if (sscanf(argv[0],expandfmt(CONCAT3(" setcurrwindow %",NAMELENSTR,"[a-zA-Z0-9_]")),winname)!=1)
  {
    PrintHelp("setcurrwindow",HELPITEM," (specify a window name)");
    return(PARAMERRORCODE);
  }

  theWin = GetUgWindow(winname);

  if (theWin==NULL)
  {
    PrintErrorMessage('E',"setcurrwindow","no window with this name open");
    return (CMDERRORCODE);
  }

  SetCurrentUgWindow(theWin);

  return (OKCODE);
}

/****************************************************************************/
/*D
   drawtext - draw text in a ug window

   DESCRIPTION:
   This command draws text into a ug window.

   'drawtext <xpos> <ypos> <text> [$w <window name>] [$c] [$i]'

   .  <xpos>                 - x-coordinate in pixels (origin is the lower left corner)
   .  <ypos>                 - y-coordinate in pixels (origin is the lower left corner)
   .  <text>                 - text to draw
   .  $w~<window~name>       - draw text into this window (default: current window)
   .  $i                     - draw text inverse
   .  $c                     - center text at <xpos> <ypos>
   .  $s~<size>              - text size

   KEYWORDS:
   graphics, plot, window, text

   SEE ALSO:
   'openwindow'
   D*/
/****************************************************************************/

static INT DrawTextCommand (INT argc, char **argv)
{
  UGWINDOW *theWin;
  char winname[NAMESIZE],text[NAMESIZE];
  COORD_POINT pos;
  INT i,mode,centeropt,size;
  double x,y;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  theWin = GetCurrentUgWindow();
  if (theWin==NULL)
  {
    PrintErrorMessage('E',"drawtext","there's no window to draw text");
    return (CMDERRORCODE);
  }

  if (sscanf(argv[0],expandfmt(CONCAT3("drawtext %lf %lf %",NAMELENSTR,"[ -~]")),&x,&y,text)!=3)
  {
    PrintErrorMessage('E',"drawtext","specify position with two integers and then the text");
    return (CMDERRORCODE);
  }
  pos.x = x; pos.y = y;

  /* check options */
  centeropt = FALSE;
  mode = TEXT_REGULAR;
  size   = 0;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'w' :
      if (sscanf(argv[i],expandfmt(CONCAT3("w %",NAMELENSTR,"[a-zA-Z0-9_]")),winname)!=1)
      {
        PrintErrorMessage('E',"drawtext","specify a window name with w option");
        return (PARAMERRORCODE);
      }
      if ((theWin=GetUgWindow(winname))==NULL)
      {
        PrintErrorMessageF('E',"drawtext","there is no window named '%s'",winname);
        return (PARAMERRORCODE);
      }
      break;

    case 's' :
      if (sscanf(argv[i],"s %d",&size)!=1)
      {
        PrintErrorMessage('E',"drawtext","specify a size with s option");
        return (PARAMERRORCODE);
      }
      break;

    case 'm' :
      if (strstr(argv[i],"reg")!=NULL)
        mode = TEXT_REGULAR;
      else if (strstr(argv[i],"inv")!=NULL)
        mode = TEXT_INVERSE;
      else if (strstr(argv[i],"ind")!=NULL)
        mode = TEXT_INDEXED;
      break;

    case 'c' :
      centeropt = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("drawtext",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  DrawWindowText(theWin,pos,text,size,centeropt,mode);

  return (OKCODE);
}

/****************************************************************************/
/*D
   openpicture - open a new picture

   DESCRIPTION:
   This command opens a picture on a window
   (these will be the current window and picture resp. then).
   It calls the function 'CreatePicture'.

   'openpicture [$w <window name>] [$s <h> <v> <dh> <dv>] [$n <picture name>]'

   .  $w~<window~name>       - open a picture on this window (default: current window)
   .  $s~<h>~<v>~<dh>~<dv>   - specify the location and size in the 'standardRefSys' with
                                                        the origin located in the lower left corner of the parent window
   .n                           (default: picture size = parent window size)

   .  $n~<picture~name>      - optionally you can specify the picture name

   KEYWORDS:
   graphics, plot, window, picture, open, create

   SEE ALSO:
   'openwindow', 'closepicture'
   D*/
/****************************************************************************/

static INT OpenPictureCommand (INT argc, char **argv)
{
  UGWINDOW *theWin;
  PICTURE *thePic;
  char picname[NAMESIZE],winname[NAMESIZE];
  INT local_LL[2],local_UR[2];
  INT i,sopt;

  /* following variables: keep type for sscanf */
  int h,v,dh,dv;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: OpenPictureCommand(): me not in Context,"\
                     " no picture stucture allocated\n",me))
    return(OKCODE);
  }
        #endif

  theWin = GetCurrentUgWindow();
  if (theWin==NULL)
  {
    PrintErrorMessage('E',"openpicture","there's no window to open a picture on");
    return (CMDERRORCODE);
  }

  /* check options */
  picname[0] = '\0';
  sopt = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'n' :
      if (sscanf(argv[i],expandfmt(CONCAT3("n %",NAMELENSTR,"[a-zA-Z0-9_]")),picname)!=1)
      {
        PrintErrorMessage('E',"openpicture","specify a picture name with n option");
        return (PARAMERRORCODE);
      }
      break;

    case 'w' :
      if (sscanf(argv[i],expandfmt(CONCAT3("w %",NAMELENSTR,"[a-zA-Z0-9_]")),winname)!=1)
      {
        PrintErrorMessage('E',"openpicture","specify a window name with w option");
        return (PARAMERRORCODE);
      }
      if ((theWin=GetUgWindow(winname))==NULL)
      {
        PrintErrorMessageF('E',"openpicture","there is no window named '%s'",winname);
        return (PARAMERRORCODE);
      }
      break;

    case 's' :
      sopt = TRUE;
      if (sscanf(argv[i],"s %d %d %d %d",&h,&v,&dh,&dv)!=4)
      {
        PrintErrorMessage('E',"openpicture","specify h, v, dh, dv with s option");
        return (PARAMERRORCODE);
      }
      local_LL[0] = h;
      local_LL[1] = v;
      local_UR[0] = h+dh;
      local_UR[1] = v+dv;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("openpicture",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (strlen(picname)==0)
    sprintf(picname,"picture%d",(int) piccounter++);

  if (!sopt)
  {
    /* default size is the window's size */
    local_LL[0] = 0;
    local_LL[1] = 0;
    local_UR[0] = ABS(UGW_LUR(theWin)[0]-UGW_LLL(theWin)[0]);
    local_UR[1] = ABS(UGW_LUR(theWin)[1]-UGW_LLL(theWin)[1]);
  }

  if ((thePic=CreatePicture(picname,theWin,local_LL,local_UR))==NULL)
  {
    PrintErrorMessage('E',"openpicture","failed to open a picture");
    return (CMDERRORCODE);
  }

  SetCurrentPicture(thePic);

  return (OKCODE);
}

/****************************************************************************/
/*D
   openppic - open a new window with placed pictures

   DESCRIPTION:
   ???

   'openppic ??? [$R [0|1]]'

   .  $R~[0|1]               - if not default device: 0: replace (default), 1: rename existing graphics file (using modification date)

   KEYWORDS:
   graphics, plot, window, picture, open, create

   SEE ALSO:
   'openwindow', 'closepicture'
   D*/
/****************************************************************************/

static INT OpenPlacedPicturesCommand (INT argc, char **argv)
{
  INT i,qopt,ropt,nPic,sopt,wopt,res,rename;
  PLACEMENT_TASK task;
  int iValue,v,h,dv,dh;
  OUTPUTDEVICE *theOutDev;
  char devname[NAMESIZE],qarray[NAMESIZE],rarray[NAMESIZE],buffer[NAMESIZE];
  UGWINDOW *theWin;

  /* get number of pictures */
  if (sscanf(argv[0],"openppic %d",&iValue)!=1)
  {
    PrintErrorMessage('E',"openppic","specify number of pictures with n option");
    return (PARAMERRORCODE);
  }
  nPic = iValue;

  /* check options */
  theOutDev  = GetDefaultOutputDevice();
  rename=sopt=wopt=qopt=ropt=0;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'w' :
      wopt=1;
      if (sscanf(argv[i],expandfmt(CONCAT3("w %",NAMELENSTR,"[a-zA-Z0-9_.]")),task.win_name)!=1)
      {
        PrintErrorMessage('E',"openppic","specify a window name with w option");
        return (PARAMERRORCODE);
      }
      break;

    case 'q' :
      qopt=1;
      if (sscanf(argv[i],expandfmt(CONCAT3("q %",NAMELENSTR,"[a-zA-Z0-9_:]")),qarray)!=1)
      {
        PrintErrorMessage('E',"openppic","specify an array name with q option");
        return (PARAMERRORCODE);
      }
      break;

    case 'r' :
      ropt=1;
      if (sscanf(argv[i],expandfmt(CONCAT3("r %",NAMELENSTR,"[a-zA-Z0-9_:]")),rarray)!=1)
      {
        PrintErrorMessage('E',"openppic","specify an array name with r option");
        return (PARAMERRORCODE);
      }
      break;

    case 'R' :
      res = sscanf(argv[i]," R %d",&iValue);
      if (res==0 || (res==1 && iValue==1)) rename = 1;
      break;

    case 's' :
      sopt = 1;
      if (sscanf(argv[i],"s %d %d %d %d",&h,&v,&dh,&dv)!=4)
      {
        PrintErrorMessage('E',"openpicture","specify h, v, dh, dv with s option");
        return (PARAMERRORCODE);
      }
      task.winLL[0] = h;
      task.winLL[1] = v;
      task.winUR[0] = h+dh;
      task.winUR[1] = v+dv;
      break;

    case 'd' :
      if (sscanf(argv[i],expandfmt(CONCAT3("d %",NAMELENSTR,"[a-zA-Z0-9_-]")),devname)!=1)
      {
        PrintErrorMessage('E',"openppic","specify device name with d option");
        return (PARAMERRORCODE);
      }
      if ((theOutDev=GetOutputDevice(devname))==NULL)
      {
        PrintErrorMessageF('E',"openppic","there is no device named '%s'",devname);
        return (PARAMERRORCODE);
      }
      break;

    default :
      PrintErrorMessage('E',"openppic","unknown option");
      return (PARAMERRORCODE);
    }

  /* check if size initialized */
  if (!sopt)
  {
    PrintErrorMessage('E',"openppic","size not specified");
    return (PARAMERRORCODE);
  }

  /* check if window name initialized */
  if (!wopt)
  {
    PrintErrorMessage('E',"openppic","window name not specified");
    return (PARAMERRORCODE);
  }

  /* check if aspect-ratio-array name initialized */
  if (!qopt)
  {
    PrintErrorMessage('E',"openppic","q-array name not specified");
    return (PARAMERRORCODE);
  }

  /* check if ratio-array name initialized */
  if (!ropt)
  {
    PrintErrorMessage('E',"openppic","r-array name not specified");
    return (PARAMERRORCODE);
  }

  task.n = nPic;

  /* set names of pictues */
  for (i=0; i<nPic; i++)
  {
    sprintf(task.pic_name[i],"pic_%d",(int)i);
    sprintf(buffer,"%s%d",qarray,i);
    if (GetStringValueDouble (buffer,&(task.aspect_ratio[i])))
    {
      PrintErrorMessage('E',"openppic","q-array entry not found");
      return (PARAMERRORCODE);
    }
    sprintf(buffer,"%s%d",rarray,i);
    if (GetStringValueDouble (buffer,&(task.rel_size[i])))
    {
      PrintErrorMessage('E',"openppic","r-array entry not found");
      return (PARAMERRORCODE);
    }
  }

  /* check device */
  if (theOutDev==NULL)
  {
    PrintErrorMessage('E',"openppic","cannot find outputdevice");
    return (PARAMERRORCODE);
  }

  /* place pictures */
  theWin = OpenPlacedPictures(theOutDev,&task,rename);
  if (theWin==NULL) return (PARAMERRORCODE);
  SetCurrentUgWindow(theWin);

  return (OKCODE);
}

/****************************************************************************/
/*D
   closepicture - close a picture

   DESCRIPTION:
   This command closes one (or all) picture(s) on a window.
   It calls the function 'DeletePicture'.

   'closepicture [$a | {$w <window name> {<picture name> | $a}}]'

   .  $w~<window~name>        - close a picture of this window
                                                                (default: the current picture)
   .  $a                      - close all pictures of the current window
   .  {<picture~name>~|~$a}   - close the picture with the specified name or all pictures of that window

   KEYWORDS:
   graphics, plot, window, picture, close, remove

   SEE ALSO:
   'openwindow', 'openpicture'
   D*/
/****************************************************************************/

static INT ClosePictureCommand (INT argc, char **argv)
{
  UGWINDOW *theWin;
  PICTURE *thePic,*NextPic;
  char picname[NAMESIZE],winname[NAMESIZE];
  INT i,aopt,wopt;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  theWin = GetCurrentUgWindow();
  if (theWin==NULL)
  {
    PrintErrorMessage('W',"closepicture","there's no open window");
    return (OKCODE);
  }

  thePic = GetCurrentPicture();
  if (thePic==NULL)
  {
    PrintErrorMessage('W',"closepicture","there's no picture to dispose");
    return (OKCODE);
  }

  /* check options */
  aopt = wopt = FALSE;
  picname[0] = '\0';
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'w' :
      wopt = TRUE;
      if (sscanf(argv[i],expandfmt(CONCAT5("w %",NAMELENSTR,"[a-zA-Z0-9_] %",NAMELENSTR,"[a-zA-Z0-9_]")),winname,picname)<1)
      {
        PrintErrorMessage('E',"closepicture","specify a window name with w option");
        return (PARAMERRORCODE);
      }
      if ((theWin=GetUgWindow(winname))==NULL)
      {
        PrintErrorMessageF('E',"closepicture","there is no window named '%s'",winname);
        return (PARAMERRORCODE);
      }
      break;

    case 'a' :
      aopt = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("closepicture",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (wopt)
  {
    if (!aopt && (strlen(picname)==0))
    {
      PrintErrorMessage('E',"closepicture","specify picture name or $a with window name");
      return (PARAMERRORCODE);
    }
    else if (strlen(picname)==0)
      if ((thePic=GetUgPicture(theWin,picname))==NULL)
      {
        PrintErrorMessageF('E',"closepicture","there is no picture named '%s'",picname);
        return (PARAMERRORCODE);
      }
  }

  if (aopt)
  {
    for (thePic=GetFirstPicture(theWin); thePic!=NULL; thePic=NextPic)
    {
      NextPic = GetNextPicture(thePic);
      if (DisposePicture(thePic)!=0)
      {
        PrintErrorMessage('E',"closepicture","could not close the picture");
        return (CMDERRORCODE);
      }
    }
    SetCurrentPicture(NULL);

    return (OKCODE);
  }

  SetCurrentPicture(NULL);

  if (DisposePicture(thePic)!=0)
  {
    PrintErrorMessage('E',"closepicture","could not close the picture");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}

/****************************************************************************/
/*D
   setcurrpicture - make a picture the current picture

   DESCRIPTION:
   This command makes a picture the current picture.

   'setcurrpicture <picture name> [$w <window name>]'

   .  <picture~name>         - name of the picture
   .  $w~<window~name>       - picture resides in this window (default: current window)

   KEYWORDS:
   graphics, plot, window, picture, current, active

   SEE ALSO:
   'openwindow', 'openpicture'
   D*/
/****************************************************************************/

static INT SetCurrentPictureCommand (INT argc, char **argv)
{
  UGWINDOW *theWin;
  PICTURE *thePic;
  char picname[NAMESIZE], winname[NAMESIZE];
  INT i;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: OpenPictureCommand(): me not in Context,"\
                     " no picture stucture allocated\n",me))
    return(OKCODE);
  }
        #endif

  theWin = GetCurrentUgWindow();
  if (theWin==NULL)
  {
    PrintErrorMessage('E',"setcurrpicture","there's no open window (and therefore no picture)");
    return (CMDERRORCODE);
  }

  /* get picture name */
  if (sscanf(argv[0],expandfmt(CONCAT3(" setcurrpicture %",NAMELENSTR,"[a-zA-Z0-9_]")),picname)!=1)
  {
    PrintHelp("setcurrpicture",HELPITEM," (specify a picture name)");
    return(PARAMERRORCODE);
  }

  /* check options */
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'w' :
      if (sscanf(argv[i],expandfmt(CONCAT3("w %",NAMELENSTR,"[a-zA-Z0-9_]")),winname)!=1)
      {
        PrintErrorMessage('E',"setcurrpicture","specify a window name with w option");
        return (PARAMERRORCODE);
      }
      if ((theWin=GetUgWindow(winname))==NULL)
      {
        PrintErrorMessageF('E',"setcurrpicture","there is no window named '%s'",winname);
        return (PARAMERRORCODE);
      }
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("setcurrpicture",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  thePic = GetUgPicture(theWin,picname);

  if (thePic==NULL)
  {
    PrintErrorMessage('E',"setcurrpicture","no picture with this name open");
    return (CMDERRORCODE);
  }

  SetCurrentPicture(thePic);

  return (OKCODE);
}

/****************************************************************************/
/*D
   picwin - move the current picture to a new window

   DESCRIPTION:
   This command moves the current picture to a newly created window. All
   settings will be kept.

   'picwin'

   KEYWORDS:
   graphics, plot, window, picture, move

   SEE ALSO:
   'openwindow', 'openpicture'
   D*/
/****************************************************************************/

static INT PictureWindowCommand (INT argc, char **argv)
{
  PICTURE *thePic;

  thePic = GetCurrentPicture();
  if (thePic==NULL)
  {
    PrintErrorMessage('W',"picwin","there's no picture to move");
    return (OKCODE);
  }
  if (ErasePicture(thePic)) return (CMDERRORCODE);
  if (MovePictureToNewWindow(thePic))
  {
    PrintErrorMessage('E',"picwin","failed to create a new window for the picture");
    return (CMDERRORCODE);
  }

  SetCurrentUgWindow(PIC_UGW(thePic));
  SetCurrentPicture(thePic);

  return (OKCODE);
}

/****************************************************************************/
/*D
   clearpicture - clear current picture

   DESCRIPTION:
   This command clears current picture.
   It calls the function 'ErasePicture'.

   'clearpicture'

   KEYWORDS:
   graphics, plot, window, picture, clear, erase

   SEE ALSO:
   'openwindow', 'openpicture'
   D*/
/****************************************************************************/

static INT ClearPictureCommand (INT argc, char **argv)
{
  PICTURE *thePicture;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  NO_OPTION_CHECK(argc,argv);

  thePicture = GetCurrentPicture();
  if (thePicture==NULL)
  {
    UserWrite("WARNING: there is no current picture\n");
    return (OKCODE);
  }
  ErasePicture(thePicture);
  DrawPictureFrame(thePicture,WOP_ACTIVE);

  /* picture has changed */
  if (InvalidatePicture(thePicture))
    return (CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   picframe - toggle framing of pictures

   DESCRIPTION:
   This command toggles the framing of pictures. If on a black frame is drawn
   around each picture to show its bounds. The current picture is indicated
   by an orange frame, while the frame of a currently drawn picture turns red.
   It is recommended to switch off framing for other devices than 'screen'.

   SYMTAX:
   'picframe 0|1'

   KEYWORDS:
   graphics, plot, window, picture, frame

   SEE ALSO:
   'openwindow'
   D*/
/****************************************************************************/

static INT PicFrameCommand (INT argc, char **argv)
{
        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  NO_OPTION_CHECK(argc,argv);

  if (strchr(argv[0],'0')!=NULL)
    SetDoFramePicture (NO);
  else
    SetDoFramePicture (YES);

  return (OKCODE);
}

/****************************************************************************/
/*D
   setview - specifies the view on the object

   DESCRIPTION:
   This command specifies or changes the observer view of the object
   of the current picture.
   It calls the function 'SetView'.

    in 2D:~
   .vb
   setview [$i] [$t <x> <y>] [$x  <x> <y>]
   .ve

    in 3D:~
   .vb
   setview [$i] [$o <x> <y> <z> $t <x> <y> <z>] [$x <x> <y> [<z>]] [$p < | =]
              {$C | $R | {$P <x> <y> <z> $N <x> <y> <z>}}
   .ve

   .n                         all coordinates have to be given in physical coordinates

   .   $i                     - return to default settings first
   .   $o <x> <y> <z>         - 3D objects ONLY: specify the observer stand
   .   $p < | =               - 3D objects ONLY: choose central (<) or parallel (=) perspective
   .   $t <x> <y> [<z>]       - specify the target point in the viewplane
   .n	                         (NB: the viewplane is then defined to be normal to the line observer-target)

   .   $x <x> <y> [<z>]       - define an x-axis in the viewplane (which will be to the right in the picture)


   Some 3D plot objects allow to define a cut. It can be set and manipulated using the
   following options:~

   .   $C                     - define a default cut facing to x-direction and cutting the objects midpoint
   .   $P~<x>~<y>~<z>         - a point on the cut plane
   .   $N~<x>~<y>~<z>         - the normal of the cut plane
   .   $R					   - remove cut

   For first definition of cut specify either C-option or both P- and N-option.

   KEYWORDS:
   graphics, plot, window, picture, view, cutting plane

   SEE ALSO:
   'vdisplay'
   D*/
/****************************************************************************/

static INT SetViewCommand (INT argc, char **argv)
{
  PICTURE *thePic;
  VIEWEDOBJ *theViewedObj;
  DOUBLE *viewPoint,*targetPoint,*xAxis;
  DOUBLE vP[3],tP[3],xA[3];
  DOUBLE PlanePoint[3],PlaneNormal[3];
  DOUBLE *CutPoint,*CutNormal,*scaleptr, scale[3];
  INT *perspective;
  INT per;
  INT i,j,veclen,res,RemoveCut;

  /* following variables: keep type for sscanf */
  double x[3],help[3];

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: SetViewCommand(): me not in Context,"\
                     " view not specified",me))
    return(OKCODE);
  }
        #endif

  /* current picture */
  thePic = GetCurrentPicture();
  if (thePic==NULL)
  {
    PrintErrorMessage('E',"setview","there's no current picture");
    return (CMDERRORCODE);
  }

  theViewedObj = PIC_VO(thePic);
  veclen = (VO_DIM(theViewedObj)==TYPE_2D) ? 2 : 3;

  /* check options */
  viewPoint = targetPoint = xAxis = NULL;
  perspective = NULL;
  CutPoint = CutNormal = NULL;
  RemoveCut = NO;
  scaleptr = NULL;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'o' :
      if (VO_DIM(theViewedObj)!=TYPE_3D)
      {
        PrintErrorMessage('E',"setview","the o option applies ONLY with 3D objects");
        return (PARAMERRORCODE);
      }
      if (sscanf(argv[i],"o %lf %lf %lf",x,x+1,x+2)!=veclen)
      {
        PrintErrorMessageF('E',"setview","o option: %d coordinates required for a %dD object",(int)veclen,(int)veclen);
        return (PARAMERRORCODE);
      }
      for (j=0; j<veclen; j++)
        vP[j] = x[j];
      viewPoint = vP;
      break;

    case 't' :
      if (sscanf(argv[i],"t %lf %lf %lf",x,x+1,x+2)!=veclen)
      {
        PrintErrorMessageF('E',"setview","t option: %d coordinates required for a %dD object",(int)veclen,(int)veclen);
        return (PARAMERRORCODE);
      }
      for (j=0; j<veclen; j++)
        tP[j] = x[j];
      targetPoint = tP;
      break;

    case 's' :
      if (sscanf(argv[i],"s %lf %lf %lf",x,x+1,x+2)!=veclen)
      {
        PrintErrorMessageF('E',"setview","s option: %d scalings required for a %dD object",(int)veclen,(int)veclen);
        return (PARAMERRORCODE);
      }
      for (j=0; j<veclen; j++)
        scale[j] = x[j];
      scaleptr = scale;
      break;

    case 'x' :
      if (sscanf(argv[i],"x %lf %lf %lf",x,x+1,x+2)!=veclen)
      {
        PrintErrorMessageF('E',"setview","x option: %d coordinates required for a %dD object",(int)veclen,(int)veclen);
        return (PARAMERRORCODE);
      }
      for (j=0; j<veclen; j++)
        xA[j] = x[j];
      xAxis = xA;
      break;

    case 'p' :
      if (VO_DIM(theViewedObj)!=TYPE_3D)
      {
        PrintErrorMessage('E',"setview","the p option applies ONLY with 3D objects");
        return (PARAMERRORCODE);
      }
      if ((strchr(argv[i],'<')!=NULL) && (strchr(argv[i],'=')!=NULL))
      {
        PrintErrorMessage('E',"setview","specify EITHER < OR = for the perspective");
        return (PARAMERRORCODE);
      }
      else if (strchr(argv[i],'<')!=NULL)
        per = YES;
      else if (strchr(argv[i],'=')!=NULL)
        per = NO;
      else
      {
        PrintErrorMessage('E',"setview","specify AT LEAST < OR = for the perspective");
        return (PARAMERRORCODE);
      }
      perspective = &per;
      break;

    case 'i' :
      VO_STATUS(theViewedObj) = NOT_INIT;
      break;


    /* capitals for cut definition */

    case 'C' :
      if (!PO_USESCUT(PIC_PO(thePic)))
      {
        PrintErrorMessage('E',"setview","plot object does not use a cut");
        return(PARAMERRORCODE);
      }

      /* set default cut */
      V3_COPY(PO_MIDPOINT(PIC_PO(thePic)),PlanePoint);
      V3_CLEAR(PlaneNormal);
      CutPoint = PlanePoint;
      CutNormal = PlaneNormal;
      break;

    case 'R' :
      if (!PO_USESCUT(PIC_PO(thePic)))
      {
        PrintErrorMessage('E',"setview","plot object does not use a cut");
        return(PARAMERRORCODE);
      }

      RemoveCut = YES;
      break;

    case 'P' :
      if (!PO_USESCUT(PIC_PO(thePic)))
      {
        PrintErrorMessage('E',"setview","plot object does not use a cut");
        return(PARAMERRORCODE);
      }
      res = sscanf(argv[i],"P %lg %lg %lg",help, help+1, help+2);
      if (res!=3)
      {
        PrintErrorMessage('E',"setview","specify three values for cut plane point");
        return(PARAMERRORCODE);
      }
      V3_COPY(help,PlanePoint);
      CutPoint = PlanePoint;
      break;

    case 'N' :
      if (!PO_USESCUT(PIC_PO(thePic)))
      {
        PrintErrorMessage('E',"setview","plot object does not use a cut");
        return(PARAMERRORCODE);
      }
      res = sscanf(argv[i],"N %lg %lg %lg",help, help+1, help+2);
      if (res!=3)
      {
        PrintErrorMessage('E',"setview","specify three values for cut normal point");
        return(PARAMERRORCODE);
      }
      V3_COPY(help,PlaneNormal);
      CutNormal = PlaneNormal;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("setview",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (SetView(thePic,viewPoint,targetPoint,xAxis,perspective,RemoveCut,CutPoint,CutNormal,scaleptr)!=0)
  {
    PrintErrorMessage('E',"setview","error during SetView");
    return (CMDERRORCODE);
  }

  /* picture has changed */
  if (InvalidatePicture(thePic))
    return (CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   vdisplay - display view of current picture

   DESCRIPTION:
   This command displays view of current picture.
   It calls the function 'DisplayViewOfDisplayedObject'.

   'vdisplay [$s]'

   .   $s		- print settings in 'setview'-command style. This is especially
                                useful after interactive zoom and pan. Pasting the output into
                                a script file will reproduce the current view of the picture.

   KEYWORDS:
   graphics, plot, window, picture, view, cutting plane, display, show, print

   SEE ALSO:
   'setview'
   D*/
/****************************************************************************/

static INT DisplayViewCommand (INT argc, char **argv)
{
  PICTURE *thePic;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  /* current picture */
  thePic = GetCurrentPicture();
  if (thePic==NULL)
  {
    PrintErrorMessage('E',"vdisplay","there's no current picture");
    return (CMDERRORCODE);
  }

  /* check options */
  switch (argc)
  {
  case 1 :
    if (DisplayViewOfViewedObject(thePic))
    {
      PrintErrorMessage('E',"vdisplay","error during DisplayView");
      return (CMDERRORCODE);
    }
    break;
  case 2 :
    if (argv[1][0]!='s')
    {
      sprintf(buffer,"(invalid option '%s')",argv[1]);
      PrintHelp("vdisplay",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }
    PrintViewSettings(thePic);
    break;
  default :
    PrintErrorMessage('E',"vdisplay","too many options");
    return (CMDERRORCODE);
  }

  return(OKCODE);
}

/****************************************************************************/
/*D
   cpview - copy view settings of current picture to other ones

   DESCRIPTION:
   This command copies the view settings of current picture to all other pictures of
   the same window (default) or all windows provided that they belong to the
   same MG and they have the same dimension.

   'cpview [$a] [$c]'
   .  a - set views of all pictures in all windows
   .  c - set also cut (if defined for plot object)

   KEYWORDS:
   graphics, plot, window, picture, view, cutting plane, copy

   SEE ALSO:
   'setview', 'arrowtool'
   D*/
/****************************************************************************/

static INT CopyViewCommand (INT argc, char **argv)
{
  PICTURE *thePic;
  INT i,all,cut;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  /* current picture */
  thePic = GetCurrentPicture();
  if (thePic==NULL)
  {
    PrintErrorMessage('E',"cpview","there's no current picture");
    return (CMDERRORCODE);
  }
  all = cut = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      all = TRUE;
      break;

    case 'c' :
      cut = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("cpview",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (CopyView(thePic,all,cut))
    return (CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   walk - let the observer walk relative to the viewRefSys

   DESCRIPTION:
   This command lets the observer walk relative to the 'viewRefSys'
   in the current picture. It calls the function 'walk'.

   'walk <x> <y> [<z>]'

   .   <x>~<y>~[<z>] - coordinates

   KEYWORDS:
   graphics, plot, window, picture, view, observer, move

   SEE ALSO:
   'arrowtool'
   D*/
/****************************************************************************/

static INT WalkCommand (INT argc, char **argv)
{
  PICTURE *thePic;
  VIEWEDOBJ *theViewedObj;
  DOUBLE dx[3];
  INT i,veclen;

  /* following variables: keep type for sscanf */
  double x[3];


        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  NO_OPTION_CHECK(argc,argv);

  /* current picture */
  thePic = GetCurrentPicture();
  if (thePic==NULL)
  {
    PrintErrorMessage('E',"walk","there's no current picture");
    return (CMDERRORCODE);
  }

  theViewedObj = PIC_VO(thePic);
  veclen = (VO_DIM(theViewedObj)==TYPE_2D) ? 2 : 3;

  if (sscanf(argv[0],"walk %lf %lf %lf",x,x+1,x+2)!=veclen)
  {
    PrintErrorMessageF('E',"walk","%d coordinates required for a %dD object",(int)veclen,(int)veclen);
    return (PARAMERRORCODE);
  }
  for (i=0; i<veclen; i++)
    dx[i] = x[i];

  if (Walk(thePic,dx)!=0)
  {
    PrintErrorMessage('E',"walk","error during Walk");
    return (CMDERRORCODE);
  }

  /* picture has changed */
  if (InvalidatePicture(thePic))
    return (CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   walkaround - let the observer walk on a sphere around the target point

   DESCRIPTION:
   This command lets the observer walk on a sphere around the target point.
   It calls the function 'RunAroundTargetPoint'.

   'walkaround <viewplane angle> <rotation angle>'

   .  <viewplane~angle>      - this angle runs in the view plane math pos from the x-axis and defines
   .n                        - together with the target-observer direction a plane

   .  <rotation~angle>       - the observer will be rotated around the target point in the above plane

   (angles in degree 0 - 360)

   KEYWORDS:
   graphics, plot, window, picture, view, observer, move, rotate

   SEE ALSO:
   'arrowtool'
   D*/
/****************************************************************************/

/****************************************************************************/
/*
   WalkAroundCommand - let the observer walk on a sphere around the target point

   SYNOPSIS:
   static INT WalkAroundCommand (INT argc, char **argv);

   PARAMETERS:
   .  argc - number of arguments (incl. its own name)
   .  argv - array of strings giving the arguments

   DESCRIPTION:
   This function lets the observer walk on a sphere around
   the target point in the current picture. (3D ONLY)
   It lets the observer walk on a sphere around the target point in the current picture.

   walkaround <viewplane angle> <rotation angle>

   .  <viewplane~angle>      - this angle runs in the view plane math pos from the x-axis and defines
   .n                        - together with the target-observer direction a plane

   .  <rotation~angle>       - the observer will be rotated around the target point in the above plane

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
 */
/****************************************************************************/

static INT WalkAroundCommand (INT argc, char **argv)
{
  PICTURE *thePic;
  VIEWEDOBJ *theViewedObj;

  /* following variables: keep type for sscanf */
  double dirAngle,Angle;


        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  NO_OPTION_CHECK(argc,argv);

  /* current picture */
  thePic = GetCurrentPicture();
  if (thePic==NULL)
  {
    PrintErrorMessage('E',"walkaround","there's no current picture");
    return (CMDERRORCODE);
  }

  theViewedObj = PIC_VO(thePic);

  if (VO_DIM(theViewedObj)!=TYPE_3D)
  {
    PrintErrorMessage('E',"walkaround","walkaround only possible for 3D objects");
    return (CMDERRORCODE);
  }

  if (sscanf(argv[0],"walkaround %lf %lf",&dirAngle,&Angle)!=2)
  {
    PrintErrorMessage('E',"walkaround","2 angles required");
    return (PARAMERRORCODE);
  }

  /* transform degree into arclength */
  dirAngle *= PI/180.0;
  Angle    *= PI/180.0;

  if (RunAroundTargetPoint(thePic,dirAngle,Angle)!=0)
  {
    PrintErrorMessage('E',"walkaround","error during WalkAroundTargetPoint");
    return (CMDERRORCODE);
  }

  /* picture has changed */
  if (InvalidatePicture(thePic))
    return (CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   zoom - zoom the current picture

   DESCRIPTION:
   This command zooms the current picture.
   the zoom factor is always relative to the current setting.
   It calls the function 'Zoom'.

   'zoom <factor>'

   .  <factor>  - values < 1 magnify picture

   KEYWORDS:
   graphics, plot, window, picture, view, zoom, magnify

   SEE ALSO:
   'arrowtool'
   D*/
/****************************************************************************/

static INT ZoomCommand (INT argc, char **argv)
{
  PICTURE *thePic;

  /* following variables: keep type for sscanf */
  double factor;

  NO_OPTION_CHECK(argc,argv);

  /* current picture */
  thePic = GetCurrentPicture();
  if (thePic==NULL)
  {
    PrintErrorMessage('E',"zoom","there's no current picture");
    return (CMDERRORCODE);
  }

  if (sscanf(argv[0],"zoom %lf",&factor)!=1)
  {
    PrintErrorMessage('E',"zoom","zoom factor required");
    return (PARAMERRORCODE);
  }

  if (Zoom(thePic,factor)!=0)
  {
    PrintErrorMessage('E',"zoom","error during Zoom");
    return (CMDERRORCODE);
  }

  /* picture has changed */
  if (InvalidatePicture(thePic))
    return (CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   drag - drag the projection plane of the current picture

   DESCRIPTION:
   This command drags the projection plane of the current picture relative
   to its x-axis.
   It calls the function 'DragProjectionPlane'.

   'drag <dx> <dy>'

   .  <dx>~<dy>  - displacement vector

   KEYWORDS:
   graphics, plot, window, picture, view, observer, pan, move

   SEE ALSO:
   'arrowtool'
   D*/
/****************************************************************************/

static INT DragCommand (INT argc, char **argv)
{
  PICTURE *thePic;

  /* following variables: keep type for sscanf */
  double dx,dy;

  NO_OPTION_CHECK(argc,argv);

  /* current picture */
  thePic = GetCurrentPicture();
  if (thePic==NULL)
  {
    PrintErrorMessage('E',"drag","there's no current picture");
    return (CMDERRORCODE);
  }

  if (sscanf(argv[0],"drag %lf %lf",&dx,&dy)!=2)
  {
    PrintErrorMessage('E',"drag","dx, dy required");
    return (PARAMERRORCODE);
  }

  if (DragProjectionPlane(thePic,dx,dy)!=0)
  {
    PrintErrorMessage('E',"drag","error during DragProjectionPlane");
    return (CMDERRORCODE);
  }

  /* picture has changed */
  if (InvalidatePicture(thePic))
    return (CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   rotate - rotate the projection plane of the current picture

   DESCRIPTION:
   This command rotates the projection plane of the current picture around
   the target point.
   It calls the function 'RotateProjectionPlane'.

   'rotate <angle>'

   .  <angle>  - <angle> runs in the view plane math pos from the x-axis

   KEYWORDS:
   graphics, plot, window, picture, view, observer, rotate

   SEE ALSO:
   'arrowtool'
   D*/
/****************************************************************************/

static INT RotateCommand (INT argc, char **argv)
{
  PICTURE *thePic;
  VIEWEDOBJ *theVO;
  DOUBLE ex,ey,norm;

  /* following variables: keep type for sscanf */
  double angle;

  NO_OPTION_CHECK(argc,argv);

  /* current picture */
  thePic = GetCurrentPicture();
  if (thePic==NULL)
  {
    PrintErrorMessage('E',"rotate","there's no current picture");
    return (CMDERRORCODE);
  }
  theVO=PIC_VO(thePic);

  if (sscanf(argv[0],"rotate %lf",&angle)!=1)
  {
    V_DIM_EUKLIDNORM(VO_PXD(theVO),norm);
    if (norm==0.0) return(CMDERRORCODE);
    ex=VO_PXD(theVO)[DIM-1]/norm;
    V_DIM_EUKLIDNORM(VO_PYD(theVO),norm);
    if (norm==0.0) return(CMDERRORCODE);
    ey=VO_PYD(theVO)[DIM-1]/norm;
    if (ex==0.0 && ey==0.0) return(CMDERRORCODE);
    angle=-atan2(ex,ey);
    if (ey*cos(angle)<ex*sin(angle))
      angle+=PI;
  }
  else
    angle *= PI/180.0;

  if (RotateProjectionPlane(thePic,angle)!=0)
  {
    PrintErrorMessage('E',"rotate","error during RotateProjectionPlane");
    return (CMDERRORCODE);
  }

  /* picture has changed */
  if (InvalidatePicture(thePic))
    return (CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   textfac - set factor to zoom text sizes

   DESCRIPTION:
   This command sets factor to zoom text sizes (default 1).

   SYMTAX:
   'textfac <factor>'

   KEYWORDS:
   graphics, plot, window, picture, view, textsize
   D*/
/****************************************************************************/

static INT TextFacCommand (INT argc, char **argv)
{
  double Value;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  NO_OPTION_CHECK(argc,argv);

  if (sscanf(argv[0],"textfac %lf",&Value)!=1)
  {
    PrintErrorMessage('E',"textfac","specify a factor");
    return (PARAMERRORCODE);
  }

  SetTextFactor(Value);

  InvalidatePicturesOfMG(currMG);

  return (OKCODE);
}

/****************************************************************************/
/*D
   setplotobject - set plotting specification

   DESCRIPTION:
   This command specifies the object which will be plotted in the current
   picture and associates the current multigrid with it (if not done yet).
   It calls the function 'SpecifyPlotObjOfViewedObject'.

   'setplotobject [<object type name>] [$a] ... '

   .    <object~type~name>   - possible are:
   .n								2 and 3D:~
   .n								'Matrix'

   .n								2 and 3D (with possibly different options):~
   .n								'Grid',			for help see 'Grid2D', 'Grid3D'
   .n								'EScalar',		for help see 'EScalar2D', 'EScalar3D'
   .n								'EVector'		for help see 'EVector2D', 'EVector3D'
   .n								'VecMat'		for help see 'VecMat2D', 'VecMat3D'

   .n								2D only:~
   .n								'line'

   .    $a					  - (for 3d plot objects) moves the observer to a place outside
                                    the bounding sphere of the plot object


   The remaining options depend on which object you specified.
   See the help for one of the abobe plotobjects for further help.

   EXAMPLE:
   .vb
   openpicture $n p0 $s 5 5  300 300;
   setplotobject Grid $b 1 $n 0 $e 0 $c 1 $w a;
   setview;
   zoom 0.75;

   openpicture $n p1 $s 5 310 300 300;
   setplotobject EScalar $e u_sol $d 1;
   setview;
   zoom 0.75;

   openpicture $n p2 $s 5 615 300 300;
   setplotobject EScalar $e v_sol $d 1;
   .ve

   KEYWORDS:
   graphics, plot, window, picture, view, plotobject

   SEE ALSO:
   Matrix, Grid, EScalar, EVector, VecMat, line
   D*/

static INT SetPlotObjectCommand (INT argc, char **argv)
{
  PICTURE *thePic;
  MULTIGRID *theMG;
  char potname[NAMESIZE],*thePlotObjTypeName;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: SetPlotObjectCommand(): me not in Context," \
                     " plot object not specified\n",me))
    return(OKCODE);
  }
        #endif

  /* current picture */
  thePic = GetCurrentPicture();
  if (thePic==NULL)
  {
    PrintErrorMessage('E',"setplotobject","there's no current picture");
    return (CMDERRORCODE);
  }

  /* get plot object name */
  if (sscanf(argv[0],expandfmt(CONCAT3(" setplotobject %",NAMELENSTR,"[a-zA-Z0-9_]")),potname)==1)
  {
    thePlotObjTypeName = potname;

    /* current multigrid */
    theMG = currMG;
    if (theMG==NULL)
    {
      PrintErrorMessage('E',"setplotobject","no current multigrid\n");
      return (CMDERRORCODE);
    }
  }
  else
  {
    thePlotObjTypeName = NULL;
    theMG = NULL;
  }

  if (theMG!=NULL)
    UserWriteF(" picture '%s' and multigrid '%s' coupled\n",ENVITEM_NAME(thePic),ENVITEM_NAME(theMG));

  if (SpecifyPlotObjOfViewedObject(thePic,theMG,thePlotObjTypeName,argc,argv)!=0)
  {
    PrintErrorMessage('E',"setplotobject","error during SpecifyPlotObjOfViewedObject");
    return (CMDERRORCODE);
  }

  /* picture has changed */
  if (InvalidatePicture(thePic))
    return (CMDERRORCODE);


  return (OKCODE);
}

/****************************************************************************/
/*D
   polist - print the specifications of the object

   DESCRIPTION:
   This command prints the specifications of the object defined in the
   current picture. It calls the function 'DisplayObject'.

   'polist'

   KEYWORDS:
   graphics, plot, window, picture, plotobject, list, show, print, display

   SEE ALSO:
   'setplotobject'
   D*/
/****************************************************************************/

static INT PlotObjectListCommand (INT argc, char **argv)
{
  PICTURE *thePic;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  NO_OPTION_CHECK(argc,argv);

  /* current picture */
  thePic = GetCurrentPicture();
  if (thePic==NULL)
  {
    PrintErrorMessage('W',"listplotobject","there's no current picture");
    return (OKCODE);
  }

  if (DisplayObject(PIC_PO(thePic))!=0)
  {
    PrintErrorMessage('E',"listplotobject","error during DisplayPlotObjOfViewedObject");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}

/****************************************************************************/
/*D
   plot - plot an object

   DESCRIPTION:
   This command plots the object of the current picture according to
   its specifications.
   It calls the function 'WorkOnPicture'.

   'plot [$o [0|1|2]] [$b [<factor>]] [$a]'

   .   $o	- ordering strategy (only used for 3D hidden surface)
   .   $b  - use bullet plotter
   .   $a  - plot all pictures in all windows

   KEYWORDS:
   graphics, plot, window, picture, plotobject, draw
   D*/
/****************************************************************************/

static INT PlotCommand (INT argc, char **argv)
{
  UGWINDOW *theUgW;
  PICTURE *thePic,*currPic;
  INT i,OrderStrategy,all,bullet,noframe;
  DOUBLE zOffsetFactor;

  /* scan for options */

  OrderStrategy = 0;
  all = bullet = noframe = NO;
  zOffsetFactor = 1.0;

  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'o' :
      sscanf(argv[i],"o %d", &OrderStrategy);
      break;
    case 'a' :
      all = YES;
      break;
    case 'b' :
      bullet = YES;
      sscanf(argv[i],"b %lf", &zOffsetFactor);
      break;
    case 'n' :
      noframe = YES;
      break;

    default :
      break;
    }
  if (SetOrderStrategy(OrderStrategy)!=0)
  {
    PrintErrorMessage('E',"plot","invalid order mode");
    return (CMDERRORCODE);
  }

  if (all)
  {
    currPic = GetCurrentPicture();

    /* plot all pictures in all windows */
    for (theUgW=GetFirstUgWindow(); theUgW!=NULL; theUgW=GetNextUgWindow(theUgW))
      for (thePic=GetFirstPicture(theUgW); thePic!=NULL; thePic=GetNextPicture(thePic))
      {
        if (bullet) {
          if (BulletDrawUgPicture(thePic, zOffsetFactor)!=0)
          {
            PrintErrorMessage('E',"plot","error during WorkOnPicture");
            return (CMDERRORCODE);
          }
        }
        else {
          if (DrawUgPicture(thePic)!=0)
          {
            PrintErrorMessage('E',"plot","error during WorkOnPicture");
            return (CMDERRORCODE);
          }
        }

                                #ifdef ModelP
        if (me == master)
                                #endif

        if (thePic==currPic)
          DrawPictureFrame(thePic,WOP_ACTIVE);
        else
          DrawPictureFrame(thePic,WOP_NOT_ACTIVE);
      }

    return (OKCODE);
  }

  /* current picture */
  thePic = GetCurrentPicture();
  if (thePic==NULL)
  {
    PrintErrorMessage('E',"plot","there's no current picture");
    return (CMDERRORCODE);
  }

  if (bullet) {
    if (BulletDrawUgPicture(thePic, zOffsetFactor)!=0)
    {
      PrintErrorMessage('E',"plot","error during WorkOnPicture");
      return (CMDERRORCODE);
    }
  }
  else {
    if (DrawUgPicture(thePic)!=0)
    {
      PrintErrorMessage('E',"plot","error during WorkOnPicture");
      return (CMDERRORCODE);
    }
  }

        #ifdef ModelP
  if (me == master)
        #endif

  /* picture is current */
  if (noframe==NO)
    DrawPictureFrame(thePic,WOP_ACTIVE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   findrange - find the range of values to be plotted

   DESCRIPTION:
   This command computes the range of  values to be plotted for the object
   in the current picture. The result will be printed and
   stored into :findrange:min and :findrange:max.
   It calls the function 'WorkOnPicture'.

   'findrange [$z <zoom factor>] [$s] [$p]'

   .  $z~<zoom factor>       - zoom the range by this factor
   .  $s                     - symmetrize the range (min=max)
   .  $p                     - store range in plot object

   KEYWORDS:
   graphics, plot, window, picture, plotobject, range, values

   SEE ALSO:
   'plot'
   D*/
/****************************************************************************/

static INT FindRangeCommand (INT argc, char **argv)
{
  PICTURE *thePic;
  WORK myWork,*theWork;
  INT i,sym,put;
  DOUBLE min,max;

  /* following variables: keep type for sscanf */
  double zoom;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: FindRangeCommand(): me not in Context,"\
                     " range found\n",me))
    return(OKCODE);
  }
        #endif

  theWork = &myWork;

  /* current picture */
  thePic = GetCurrentPicture();
  if (thePic==NULL)
  {
    PrintErrorMessage('E',"findrange","there's no current picture");
    return (CMDERRORCODE);
  }

  /* check options */
  put = sym = NO;
  zoom = 1.0;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 's' :
      sym = YES;
      break;

    case 'p' :
      put = YES;
      break;

    case 'z' :
      if (sscanf(argv[i],"z %lf",&zoom)!=1)
      {
        PrintErrorMessage('E',"findrange","specify a zoom factor with z option");
        return (PARAMERRORCODE);
      }
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("findrange",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  /* fill work struct */
  W_ID(theWork) = FINDRANGE_WORK;

  W_FINDRANGE_WORK(theWork)->symmetric = sym;
  W_FINDRANGE_WORK(theWork)->zoom          = zoom;
  W_FINDRANGE_WORK(theWork)->put           = put;

  if (WorkOnPicture(thePic,theWork)!=0)
  {
    PrintErrorMessage('E',"findrange","error during WorkOnPicture");
    return (CMDERRORCODE);
  }

  /* read out min and max */
  min = W_FINDRANGE_WORK(theWork)->min;
        #ifdef ModelP
  Broadcast(&min,sizeof(double));
  W_FINDRANGE_WORK(theWork)->min = min;
        #endif

  max = W_FINDRANGE_WORK(theWork)->max;
        #ifdef ModelP
  Broadcast(&max,sizeof(double));
  W_FINDRANGE_WORK(theWork)->max = max;
        #endif

  UserWriteF(" FR_min = %20.16e\n FR_max = %20.16e\n",min,max);

  if (put)
    if (InvalidatePicture(thePic))
      return (CMDERRORCODE);

  if (    (SetStringValue(":findrange:min",min)!=0)
          ||      (SetStringValue(":findrange:max",max)!=0))
  {
    PrintErrorMessage('E',"findrange","could not set :findrange:min or :findrange:max");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}

/****************************************************************************/
/*D
   setcurrmg - change the current multigrid

   DESCRIPTION:
   This command sets the current multigrid.

   'setcurrmg <mgname>'

   .  <mgname>  - name of the open multigrid which will be made the current one

   KEYWORDS:
   multigrid, current, active

   SEE ALSO:
   'open', 'new'
   D*/
/****************************************************************************/

static INT SetCurrentMultigridCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char mgname[NAMESIZE];

  NO_OPTION_CHECK(argc,argv);

  /* get multigrid name */
  if (sscanf(argv[0],expandfmt(CONCAT3(" setcurrmg %",NAMELENSTR,"[ -~]")),mgname)!=1)
  {
    PrintHelp("setcurrmg",HELPITEM," (specify current multigrid name)");
    return(PARAMERRORCODE);
  }

  theMG = GetMultigrid(mgname);

  if (theMG==NULL)
  {
    PrintErrorMessage('E',"setcurrmg","no multigrid with this name open");
    return (CMDERRORCODE);
  }

  if (SetCurrentMultigrid(theMG)!=0)
    return (CMDERRORCODE);

  return(OKCODE);
}

/****************************************************************************/
/*D
   updateDoc - reset the windows and pictures of the current multigrid to invalid

   DESCRIPTION:
   This command runs 'InvalidatePicturesOfMG' and
   'InvalidateUgWindowsOfMG'.
   If the refresh state is on, the pictures will be replotted.

   'updateDoc'

   KEYWORDS:
   graphics, plot, window, picture, plotobject, invalidate
   D*/
/****************************************************************************/

static INT UpdateDocumentCommand (INT argc, char **argv)
{
        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  NO_OPTION_CHECK(argc,argv);

  InvalidatePicturesOfMG(currMG);
  InvalidateUgWindowsOfMG(currMG);

  return (OKCODE);
}

/****************************************************************************/
/*D
   rotmode - rotation mode for 3D pictures (arrow tool)

   DESCRIPTION:
   This command toggles the rotation mode for 3D plot objects between
   Euler angle mode and Virtual Sphere. Just try both to find the most
   convenient one.

   'rotmode E[uler] | S[phere]'

   KEYWORDS:
   graphics, plot, window, picture, plotobject, arrowtool

   SEE ALSO:
   arrowtool
   D*/
/****************************************************************************/

static INT RotModeCommand (INT argc, char **argv)
{
  INT mode;

  NO_OPTION_CHECK(argc,argv);

  if (strchr(argv[0],'E')!=NULL)
    mode = ROTMODE_EULER;
  else if (strchr(argv[0],'S')!=NULL)
    mode = ROTMODE_SPHERE;
  else
  {
    PrintHelp("rotmode",HELPITEM," (specify Euler or Sphere)");
    return(PARAMERRORCODE);
  }

  SetRotMode(mode);

  return (OKCODE);
}

/****************************************************************************/
/*D
   setpalette - switch palette of default device

   DESCRIPTION:
   This command switches the palette of the default device.

   'setpalette c|bw|g [$d <device>]'
   .  $d~<device>     - specify the name of an output device (default: screen)

   KEYWORDS:
   graphics, plot, window, picture, device
   D*/
/****************************************************************************/

static INT SetPaletteCommand (INT argc, char **argv)
{
  OUTPUTDEVICE *theOutDev;
  INT i,palette;
  char plt,devname[NAMESIZE];

  if (sscanf(argv[0],"setpalette %c",&plt)!=1)
  {
    PrintHelp("setpalette",HELPITEM," (specify c|bw|g)");
    return(PARAMERRORCODE);
  }
  switch (plt)
  {
  case 'c' : palette = COLOR_PALETTE; break;
  case 'b' : palette = BLACK_WHITE_PALETTE; break;
  case 'g' : palette = GRAY_PALETTE; break;
  default :
    PrintHelp("setpalette",HELPITEM," (specify c|bw|g)");
    return(PARAMERRORCODE);
  }

  /* check options */
  theOutDev  = GetDefaultOutputDevice();
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'd' :
      if (sscanf(argv[i],expandfmt(CONCAT3("d %",NAMELENSTR,"[a-zA-Z0-9_-]")),devname)!=1)
      {
        PrintErrorMessage('E',"setpalette","specify device name with d option");
        return (PARAMERRORCODE);
      }
      if ((theOutDev=GetOutputDevice(devname))==NULL)
      {
        PrintErrorMessageF('E',"setpalette","there is no device named '%s'",devname);
        return (PARAMERRORCODE);
      }
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("setpalette",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  if (UgSetPalette(theOutDev,palette))
    REP_ERR_RETURN(CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   clear - assign a value to a symbolic vector

   DESCRIPTION:
   This function sets the values of a grid function specified by a vec data descriptor.
   The data descriptor is created if it does not exist yet.
   It clears or assigns a constant value.

   'clear <symbol name> [$a] [$u] [$v <value>] [$x] [$y] [$z]'

   .  $a         - from level 0 through current level (default: current level only)
   .  $s         - do not change skip (Dirichlet) values
   .  $v~<value> - assign this value (instead of 0.0)

   KEYWORDS:
   multigrid, numerics, userdata, vecdata, clear, set

   SEE ALSO:
   'cv', 'cm', 'copy'
   D*/
/****************************************************************************/

static INT ClearCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VECDATA_DESC *theVD;
  VECTOR *v;
  INT i,l,fl,tl,n,skip,xflag,yflag,zflag;
  int j;
  double value;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"clear","no current multigrid");
    return(CMDERRORCODE);
  }
  theVD = ReadArgvVecDesc(theMG,"clear",argc,argv);
  if (theVD == NULL) {
    PrintErrorMessage('E',"clear","could not read data descriptor");
    return (PARAMERRORCODE);
  }
  if (ReadArgvOption("d",argc,argv)) {
    for (i=theMG->bottomLevel; i<=TOPLEVEL(theMG); i++)
      ClearVecskipFlags(GRID_ON_LEVEL(theMG,i),theVD);
    return (OKCODE);
  }
  if (ReadArgvOption("r",argc,argv)) {
    i = CURRENTLEVEL(theMG);
    l_dsetrandom(GRID_ON_LEVEL(theMG,i),theVD,EVERY_CLASS,1.0);
    if (ReadArgvOption("d",argc,argv))
      ClearDirichletValues(GRID_ON_LEVEL(theMG,i),theVD);
    return (OKCODE);
  }
  /* check options */
  fl = tl = CURRENTLEVEL(theMG);
  skip = FALSE;
  xflag = -1;
  value = 0.0;
  j = -1;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      fl = 0;
      break;

    case 's' :
      skip = TRUE;
      break;

    case 'x' :
      xflag = 0;
      break;

    case 'y' :
      xflag = 1;
      break;

    case 'z' :
      xflag = 2;
      break;

    case 'i' :
      if (sscanf(argv[i],"i %d",&j)!=1)
      {
        PrintErrorMessage('E',"clear","could not read value");
        return(CMDERRORCODE);
      }
      break;

    case 'v' :
      if (sscanf(argv[i],"v %lf",&value)!=1)
      {
        PrintErrorMessage('E',"clear","could not read value");
        return(CMDERRORCODE);
      }
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("clear",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }
  if (j >= 0) {
    for (v = FIRSTVECTOR(GRID_ON_LEVEL(theMG,CURRENTLEVEL(theMG)));
         v != NULL; v = SUCCVC(v)) {
      n = VD_NCMPS_IN_TYPE(theVD,VTYPE(v));
      if (j < n) {
        VVALUE(v,VD_CMP_OF_TYPE(theVD,VTYPE(v),j)) = value;
        return (OKCODE);
      }
      j -= n;
    }
    return (CMDERRORCODE);
  }
  if (xflag != -1) {
    for (l=fl; l<=tl; l++)
      for (v=FIRSTVECTOR(GRID_ON_LEVEL(theMG,l)); v!=NULL; v=SUCCVC(v))
      {
        DOUBLE_VECTOR pos;

        if (VD_NCMPS_IN_TYPE(theVD,VTYPE(v)) == 0) continue;
        if (VectorPosition(v,pos)) continue;
        VVALUE(v,VD_CMP_OF_TYPE(theVD,VTYPE(v),0)) = pos[xflag];
      }
    return (OKCODE);
  }
  if (skip) {
    if (a_dsetnonskip(theMG,fl,tl,theVD,EVERY_CLASS,value)
        !=NUM_OK)
      return (CMDERRORCODE);
  }
  else {
    if (dset(theMG,fl,tl,ALL_VECTORS,theVD,value)!=NUM_OK)
      return (CMDERRORCODE);
  }

  return (OKCODE);
}

/****************************************************************************/
/*D
   makevdsub - make a sub descriptor for a vector descriptor

   DESCRIPTION:
   This function makes a sub descriptor for a vector descriptor.

   'makevdsub <vd> $sub <vt> <sub>'

   .  <vd>				- existing descriptor
   .  $sub~<vt>~<sub>	- template and name of sub of this template

   KEYWORDS:
   multigrid, numerics, userdata, vecdata, descriptor
   D*/
/****************************************************************************/

static INT MakeVDsubCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VECDATA_DESC *theVD,*subVD;
  VEC_TEMPLATE *vt;
  INT sub;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"makevdsub","no current multigrid");
    return(CMDERRORCODE);
  }

  theVD = ReadArgvVecDescX(theMG,"makevdsub",argc,argv,NO);

  if (theVD == NULL) {
    PrintErrorMessage('E',"makevdsub","could not read data descriptor");
    return (PARAMERRORCODE);
  }
  vt = ReadArgvVecTemplateSub(MGFORMAT(theMG),"sub",argc,argv,&sub);
  if (vt==NULL)
    REP_ERR_RETURN(PARAMERRORCODE);

  if (VDsubDescFromVT(theVD,vt,sub,&subVD))
    REP_ERR_RETURN(CMDERRORCODE);

  UserWriteF("sub descriptor '%s' for '%s' created\n",ENVITEM_NAME(subVD),ENVITEM_NAME(theVD));

  return (OKCODE);
}

/****************************************************************************/
/*D
   mflops - floating point speed meassuring

   DESCRIPTION:
   This function tests the performance of the UG specific blas routines.

   'mflops $x <vec> [$y <tmp>] [$A <mat>] [$l <loop>]'

   .  $x~<vec>   - vector
   .  $y~<tmp>   - second vector
   .  $A~<mat>   - matrix
   .  $l~<loop>  - loop number

   REMARK:
   Due to the inaccuracy of the most UNIX clock systems take a huge loop number
   to get an good average.
   D*/
/****************************************************************************/

static INT MFLOPSCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *g;
  INT i,l;
  INT ncomp;
  VECDATA_DESC *x,*y;
  MATDATA_DESC *A;
  VECTOR *v;
  MATRIX *mat;
  INT n,m,loop;
  DOUBLE nop;
  VEC_SCALAR scal;
  DOUBLE time_ddot, time_matmul;

  /* get MG */
  theMG = GetCurrentMultigrid();
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"value","no current multigrid");
    return(CMDERRORCODE);
  }
  l = CURRENTLEVEL(theMG);
  g = GRID_ON_LEVEL(theMG,l);

  /* read arguments, allocate two vectors and a matrix */
  A = ReadArgvMatDesc(theMG,"A",argc,argv);
  x = ReadArgvVecDesc(theMG,"x",argc,argv);
  y = ReadArgvVecDesc(theMG,"y",argc,argv);
  if (x == NULL)
  {
    PrintErrorMessage('E',"x","could not read symbol");
    return (PARAMERRORCODE);
  }
  if (AllocVDFromVD(theMG,l,l,x,&y))
    return (CMDERRORCODE);
  if (AllocMDFromVD(theMG,l,l,x,x,&A))
    return (CMDERRORCODE);
  if (ReadArgvINT("loop",&loop,argc,argv))
  {
    loop = 100;
  }

  /* gather statistics */
  n = m = 0;
  for (v=FIRSTVECTOR(g); v!= NULL; v=SUCCVC(v))
  {
    n++;
    for (mat=VSTART(v); mat!=NULL; mat = MNEXT(mat))
      m++;
  }
  ncomp = VD_ncmps_in_otype(x,NODEVEC);
  if ((ncomp <= 0) || (VD_NCOMP(x) != ncomp)) {
    PrintErrorMessage('E',"mflops","only for NODEVEC");
    return (PARAMERRORCODE);
  }

  /* initialize */
  dset(theMG,l,l,ALL_VECTORS,x,1.0);
  dset(theMG,l,l,ALL_VECTORS,y,1.0);
  dmatset(theMG,l,l,ALL_VECTORS,A,1.0);

  /* loop */
  time_ddot = CURRENT_TIME;
  for (i=1; i<=loop; i++)
    ddot(theMG,l,l,ALL_VECTORS,x,x,scal);
  time_ddot = CURRENT_TIME - time_ddot;

  time_matmul = CURRENT_TIME;
  for (i=1; i<=loop; i++)
    dmatmul(theMG,l,l,ALL_VECTORS,y,A,x);
  time_matmul = CURRENT_TIME - time_matmul;

  if (FreeMD(theMG,l,l,A)) REP_ERR_RETURN(CMDERRORCODE);
  if (FreeVD(theMG,l,l,y)) REP_ERR_RETURN(CMDERRORCODE);

  nop = 2*n*ncomp*loop;
  UserWriteF("DDOT t=%12.4E op=%12.4E MFLOPs=%12.6f\n",
             (double)time_ddot,(double)nop,
             (double)0.000001*nop/time_ddot);
  nop = m*ncomp*ncomp*2*loop;
  UserWriteF("MMUL t=%12.4E op=%12.4E MFLOPs=%12.6f\n",
             (double)time_matmul,(double)nop,
             (double)0.000001*nop/time_matmul);

  return (OKCODE);
}

/****************************************************************************/
/*D
   rand - assign a value to a symbolic vector

   DESCRIPTION:
   This function sets the random values of a grid function specified by a vec data descriptor.
   The data descriptor is created if it does not exist yet.

   'rand <symbol name> [$a] [$s] [$f <value>] [$t <value>]'

   .  $a         - from level 0 through current level (default: current level only)
   .  $s         - set skip (Dirichlet) values to zero, default is no skip
   .  $f~<value> - low bound of random range, default is 0
   .  $t~<value> - upper bound of random range, default is 1

   KEYWORDS:
   multigrid, numerics, userdata, vecdata, clear, set

   SEE ALSO:
   'cv', 'cm', 'copy'
   D*/
/****************************************************************************/

static INT RandCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *g;
  VECDATA_DESC *theVD;
  INT i,fl,tl,skip;
  double from_value,to_value;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"rand","no current multigrid");
    return(CMDERRORCODE);
  }

  /* check options */
  fl = tl = CURRENTLEVEL(theMG);
  skip = 0;
  from_value = 0.0;
  to_value = 1.0;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      fl = 0;
      break;

    case 's' :
      skip = 1;
      break;

    case 'f' :
      if (sscanf(argv[i],"f %lf",&from_value)!=1)
      {
        PrintErrorMessage('E',"rand","could not read from value");
        return(CMDERRORCODE);
      }
      break;

    case 't' :
      if (sscanf(argv[i],"t %lf",&to_value)!=1)
      {
        PrintErrorMessage('E',"rand","could not read to value");
        return(CMDERRORCODE);
      }
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("rand",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }

  theVD = ReadArgvVecDesc(theMG,"rand",argc,argv);

  if (theVD == NULL)
  {
    PrintErrorMessage('E',"rand","could not read data descriptor");
    return (PARAMERRORCODE);
  }

  for (i=fl; i<=tl; i++)
  {
    g = GRID_ON_LEVEL(theMG,i);
    if (l_dsetrandom2(g,theVD,EVERY_CLASS,(DOUBLE)from_value,(DOUBLE)to_value,skip)) return (CMDERRORCODE);
  }

  return (OKCODE);
}

/****************************************************************************/
/*D
   copy - copy from one vector symbol to another one

   DESCRIPTION:
   This command copies from one vector symbol to another one.
   The data descriptor is created if it does not exist yet.

   'copy $f <from vec sym> $t <to vec sym> [$a]'

   .  $f~<from~vec~sym>      - from vector symbol
   .  $t~<from~vec~sym>      - to vector symbol
   .  $a                     - all levels

   EXAMPLE:
   'copy $f sol $t old;'

   KEYWORDS:
   multigrid, numerics, userdata, vecdata, copy, set

   SEE ALSO:
   'cv', 'cm', 'clear'
   D*/
/****************************************************************************/

static INT CopyCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VECDATA_DESC *from,*to;
  INT fl,tl;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"copy","no current multigrid");
    return(CMDERRORCODE);
  }
  fl = tl = CURRENTLEVEL(theMG);
  if (argc<3 || argc>4)
  {
    PrintErrorMessage('E',"copy","specify exactly the f and t option");
    return(PARAMERRORCODE);
  }

  from = ReadArgvVecDescX(theMG,"f",argc,argv,NO);
  to = ReadArgvVecDesc(theMG,"t",argc,argv);

  if (from == NULL) {
    PrintErrorMessage('E',"copy","could not read 'f' symbol");
    return (PARAMERRORCODE);
  }
  if (to == NULL) {
    PrintErrorMessage('E',"copy","could not read 't' symbol");
    return (PARAMERRORCODE);
  }

  if (ReadArgvOption("a",argc,argv)) fl = 0;

  if (dcopy(theMG,fl,tl,ALL_VECTORS,to,from) != NUM_OK)
    return (CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   add - add two vector symbols

   DESCRIPTION:
   This command adds two vectors (x = x + y).

   'add $x <vec sym> $y <vec sym> [$a]'

   .  $x~<vec~sym>      - vector symbol
   .  $y~<vec~sym>      - vector symbol
   .  $a                - all levels

   EXAMPLE:
   'add $x sol $y old;'

   KEYWORDS:
   multigrid, numerics, userdata, vecdata, copy, set

   SEE ALSO:
   'cv', 'cm', 'clear', 'sub'
   D*/
/****************************************************************************/

static INT AddCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VECDATA_DESC *x,*y;
  INT fl,tl;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"copy","no current multigrid");
    return(CMDERRORCODE);
  }
  fl = tl = CURRENTLEVEL(theMG);
  if (argc<3 || argc>4)
  {
    PrintErrorMessage('E',"copy","specify exactly the f and t option");
    return(PARAMERRORCODE);
  }

  x = ReadArgvVecDesc(theMG,"x",argc,argv);
  y = ReadArgvVecDesc(theMG,"y",argc,argv);

  if (x == NULL) {
    PrintErrorMessage('E',"copy","could not read 'f' symbol");
    return (PARAMERRORCODE);
  }
  if (y == NULL) {
    PrintErrorMessage('E',"copy","could not read 't' symbol");
    return (PARAMERRORCODE);
  }
  if (ReadArgvOption("a",argc,argv)) fl = 0;

  if (dadd(theMG,fl,tl,ALL_VECTORS,x,y) != NUM_OK)
    return (CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   sub - subtract two vector symbols

   DESCRIPTION:
   This command subtracts two vectors (x = x - y).

   'sub $x <vec sym> $y <vec sym> [$a]'

   .  $x~<vec~sym>      - vector symbol
   .  $y~<vec~sym>      - vector symbol
   .  $a                - all levels

   EXAMPLE:
   'sub $x sol $y old;'

   KEYWORDS:
   multigrid, numerics, userdata, vecdata, copy, set

   SEE ALSO:
   'cv', 'cm', 'clear', 'add'
   D*/
/****************************************************************************/

static INT SubCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VECDATA_DESC *x,*y;
  INT fl,tl;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"copy","no current multigrid");
    return(CMDERRORCODE);
  }
  fl = tl = CURRENTLEVEL(theMG);
  if (argc<3 || argc>4)
  {
    PrintErrorMessage('E',"copy","specify exactly the f and t option");
    return(PARAMERRORCODE);
  }

  x = ReadArgvVecDesc(theMG,"x",argc,argv);
  y = ReadArgvVecDesc(theMG,"y",argc,argv);

  if (x == NULL) {
    PrintErrorMessage('E',"copy","could not read 'f' symbol");
    return (PARAMERRORCODE);
  }
  if (y == NULL) {
    PrintErrorMessage('E',"copy","could not read 't' symbol");
    return (PARAMERRORCODE);
  }
  if (ReadArgvOption("a",argc,argv)) fl = 0;

  if (dsub(theMG,fl,tl,ALL_VECTORS,x,y) != NUM_OK)
    return (CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   homotopy - convex combination of two vector symbols

   DESCRIPTION:

   This command sets 'x := (1-v)*x + v*y'.

   'homotopy $v <val> $x <x vec sym> $y <y vec sym> [$a]'

   .  $v~<val>               - value
   .  $x~<x~vec~sym>         - vector symbol
   .  $y~<y~vec~sym>         - vector symbol
   .  $a                     - all levels

   EXAMPLE:
   'homotopy $v 0.5 $x sol $y old;'

   KEYWORDS:
   multigrid, numerics, userdata, vecdata, weighted sum, interpolate

   SEE ALSO:
   'cv', 'cm', 'copy', 'clear'
   D*/
/****************************************************************************/

static INT HomotopyCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VECDATA_DESC *x,*y;
  DOUBLE mu;
  DOUBLE v[MAX_VEC_COMP];
  INT i;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"homotopy","no current multigrid");
    return(CMDERRORCODE);
  }

  x = ReadArgvVecDesc(theMG,"x",argc,argv);
  if (x == NULL) {
    PrintErrorMessage('E',"homotopy","could not read 'x' symbol");
    return (PARAMERRORCODE);
  }
  y = ReadArgvVecDesc(theMG,"y",argc,argv);
  if (y == NULL) {
    PrintErrorMessage('E',"homotopy","could not read 'y' symbol");
    return (PARAMERRORCODE);
  }

  if (ReadArgvDOUBLE("v",&mu,argc,argv))
    return (PARAMERRORCODE);

  if (ReadArgvOption("a",argc,argv))
  {
    for (i=0; i<VD_NCOMP(x); i++)
      v[i] = 1.0 - mu;
    if (a_dscale(theMG,0,CURRENTLEVEL(theMG),x,EVERY_CLASS,v)!=NUM_OK)
      return (CMDERRORCODE);
    for (i=0; i<VD_NCOMP(x); i++)
      v[i] = mu;
    if (a_daxpy(theMG,0,CURRENTLEVEL(theMG),x,EVERY_CLASS,v,y)!=NUM_OK)
      return (CMDERRORCODE);
  }
  else
  {
    for (i=0; i<VD_NCOMP(x); i++)
      v[i] = 1.0 - mu;
    if (l_dscale(GRID_ON_LEVEL(theMG,CURRENTLEVEL(theMG)),
                 x,EVERY_CLASS,v)!=NUM_OK)
      return (CMDERRORCODE);
    for (i=0; i<VD_NCOMP(x); i++)
      v[i] = mu;
    if (l_daxpy(GRID_ON_LEVEL(theMG,CURRENTLEVEL(theMG)),
                x,EVERY_CLASS,v,y)!=NUM_OK)
      return (CMDERRORCODE);
  }

  return (OKCODE);
}

/****************************************************************************/
/*D
   interpolate - (standard) interpolate a vector symbol to new vectors on the current level

   DESCRIPTION:
   The data descriptor is created if it does not exist yet.

   'interpolate <vec sym>'

   . <vec~sym>  - vector symbol to be interpolated

   EXAMPLE:
   'interpolate sol;'

   KEYWORDS:
   multigrid, numerics, userdata, vecdata, interpolate, prolongate

   SEE ALSO:
   'clear', 'cv'
   D*/
/****************************************************************************/

static INT InterpolateCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VECDATA_DESC *theVD;
  INT lev,currlev;

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"interpolate","no current multigrid");
    return(CMDERRORCODE);
  }

  theVD = ReadArgvVecDescX(theMG,"interpolate",argc,argv,NO);

  if (theVD == NULL) {
    PrintErrorMessage('E',"interpolate","could not read symbol");
    return (PARAMERRORCODE);
  }

  currlev = CURRENTLEVEL(theMG);
  for (lev=1; lev<=currlev; lev++)
    if (StandardInterpolateNewVectors(GRID_ON_LEVEL(theMG,lev),theVD)!=NUM_OK)
      return (CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   reinit - reinitialize a boundary value problem

   DESCRIPTION:
   This command reinitializes the problem with the user defined reinit of the
   problem. All arguments are passed to the reinit function

   'reinit [$b <boundary value problem>] ...'

   .  $b~<boundary~value~problem> - problem to initialize
                                                             (default is the problem of the current mg)

   KEYWORDS:
   multigrid, boundary value problem, configure, change
   D*/
/****************************************************************************/

static INT ReInitCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  BVP *theBVP;
  BVP_DESC theBVPDesc,*theBVPD;
  INT i,bopt;
  char BVPName[NAMESIZE];

  /* check options */
  bopt = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'b' :
      if (argv[i][1]!=' ') continue;
      if (sscanf(argv[i],expandfmt(CONCAT3("b %",NAMELENSTR,"[0-9a-zA-Z/_ ]")),BVPName)!=1)
      {
        PrintErrorMessage('E',"reinit","could not read BndValProblem string");
        return(PARAMERRORCODE);
      }
      bopt = TRUE;
      break;

      /* no default because param list is passed to reinit function */
    }

  /* set the document */
  if (bopt)
  {
    theBVP = BVP_GetByName(BVPName);
    if(theBVP==NULL)
    {
      PrintErrorMessageF('E',"reinit","could not interpret '%s' as a BVP name",BVPName);
      return (CMDERRORCODE);
    }
    if (BVP_SetBVPDesc(theBVP,&theBVPDesc)) return (CMDERRORCODE);
    theBVPD = &theBVPDesc;
  }
  else
  {
    theMG = currMG;
    if (theMG==NULL)
    {
      PrintErrorMessage('E',"reinit","no open multigrid (specify problem and domain instead)");
      return (CMDERRORCODE);
    }
    theBVP = MG_BVP(theMG);
    theBVPD = MG_BVPD(theMG);
  }

  if (BVPD_CONFIG(theBVPD)!=NULL)
    if ((*BVPD_CONFIG (theBVPD))(argc,argv)!=0)
      return (CMDERRORCODE);

  return(OKCODE);
}

/* see formats.c for the man page */

static INT CreateFormatCommand (INT argc, char **argv)
{
  INT err;

  err = CreateFormatCmd(argc,argv);

  switch (err)
  {
  case 0 : return (OKCODE);
  case 1 : PrintHelp("newformat",HELPITEM,NULL);
    return (PARAMERRORCODE);
  default : return (CMDERRORCODE);
  }
}

/****************************************************************************/
/*D
   delformat - delete a previously enroled format

   DESCRIPTION:
   This command deletes a previously enroled format.

   'delformat <format name>'

   .  <format~name> - name of the fomat

   KEYWORDS:
   storage, format
   D*/
/****************************************************************************/

static INT DeleteFormatCommand (INT argc, char **argv)
{
  INT err;
  char fmtname[NAMESIZE];

  NO_OPTION_CHECK(argc,argv);

  if (sscanf(argv[0],"delformat %s",fmtname)!=1)
  {
    PrintErrorMessage('E',"delformat","specify format to delete");
    return (PARAMERRORCODE);
  }

  err = RemoveFormatWithSubs(fmtname);

  switch (err)
  {
  case GM_OK : return (OKCODE);
  default : return (CMDERRORCODE);
  }
}

/****************************************************************************/
/*D
        setpf -  command to change current settings of the data
                                        listing functions of a format

        SYNTAX:
        setpf <format_name> [$V{0 | {+|-} {<vecsym_name>}+}]+ [$M{0 | {+|-} {<matsym_name>}+}]+

        DESCRIPTION:
        For a format previously enroled by use of the 'newformat' command the 'setpf'
        command specifies the symbols that are displayed when 'vmlist' is called with
        '$d' (list vector data) or '$d $m' (list vector and matrix data).
        '0' clears the list of symbols, '+' adds further symbols, and '-'
        removes symbols from that list.

        EXAMPLE:
   .vb
        newformat ns $V n3: vt 5 $M n3xn3: mt 2;

        open grid $f ns $h 1000000;

        clear sol;		# creates sol vec data desc from vt template

 # suppose rhs, MAT, LU data descriptors have been created by the initialization of
 # the discretization and solver

 # print sol, rhs vector data and MAT, LU matrix data of vector with index 10
        setpf ns $V0 $M0 $V+ sol rhs $M+ MAT LU;
        vmlist $i 10 $m $d;

 # print sol vector data and MAT matrix data of vector with index 12
        setpf ns $V- rhs $M- LU;
        vmlist $i 12 $m $d;
   .ve

   KEYWORDS:
   multigrid, numerics, userdata, vecdata, matdata, print, show, display

        SEE ALSO:
        'showpf'
   D*/
/****************************************************************************/

static INT SetPrintingFormatCommand (INT argc, char **argv)
{
  INT err;
  MULTIGRID *theMG;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"setpf","there is no current multigrid\n");
    return (CMDERRORCODE);
  }
  err = SetPrintingFormatCmd(theMG,argc,argv);

  switch (err)
  {
  case 0 : return (OKCODE);
  case 1 : PrintHelp("setpf",HELPITEM,NULL);
    return (PARAMERRORCODE);
  default : return (CMDERRORCODE);
  }
}

/****************************************************************************/
/*D
        showpf - command to display current settings of data
                                        listing functions

        DESCRIPTION:
        This command shows which user data will bbe listed by vml $d $m...

        'showpf'

    KEYWORDS:
    multigrid, numerics, userdata, vecdata, matdata, print, show, display

        SEE ALSO:
        setpf
   D*/
/****************************************************************************/

static INT ShowPrintingFormatCommand (INT argc, char **argv)
{
  NO_OPTION_CHECK(argc,argv);

  DisplayPrintingFormat();

  return (OKCODE);
}

/****************************************************************************/
/*D
   GetCurrentNumProc - return a pointer to the current numproc

   SYNOPSIS:
   static NUM_PROC *GetCurrentNumProc (void);

   PARAMETERS:
   .  void -

   DESCRIPTION:
   This function returns a pointer to the current numproc.

   RETURN VALUE:
   NUM_PROC *
   .n      pointer to numproc
   .n      NULL if there is no current numproc.
   D*/
/****************************************************************************/

static NP_BASE *GetCurrentNumProc (void)
{
  return (currNumProc);
}

/****************************************************************************/
/*D
   SetCurrentNumProc -	Set the current NumProc if it is valid

   SYNOPSIS:
   static INT SetCurrentNumProc (NUM_PROC *theNumProc);

   PARAMETERS:
   .  theNumProc - pointer to multigrid

   DESCRIPTION:
   This function sets the current NumProc if it is valid, i. e.
   the function checks whether 'theNumProc' acually points to a numproc.
   It can be NULL only if no numproc is defined.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if theNumProc is not in the numproc list
   D*/
/****************************************************************************/

static INT SetCurrentNumProc (NP_BASE *theNumProc)
{
  currNumProc = theNumProc;
  return (0);
}

/****************************************************************************/
/*D
   npexecute - execute a NumProc

   DESCRIPTION:
   This command executes a NumProc.
   It calls the function 'ExecuteNumProc'.

   'npexecute [<num proc name>] <argument list to be passed>'

   .  <num~proc~name> - name of an existing NumProc

   KEYWORDS:
   multigrid, numerics, userdata, vecdata, matdata, numproc, execute

   SEE ALSO
   'numerics', 'NUM_PROC'
   D*/
/****************************************************************************/

static INT ExecuteNumProcCommand (INT argc, char **argv)
{
  char theNumProcName[NAMESIZE];
  NP_BASE *theNumProc;
  MULTIGRID *theMG;
  INT err;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"npexecute","there is no current multigrid\n");
    return (CMDERRORCODE);
  }

  /* get NumProc */
  if ((sscanf(argv[0],expandfmt(CONCAT3(" npexecute %",NAMELENSTR,"[ -~]")),theNumProcName)!=1) || (strlen(theNumProcName)==0))
  {
    theNumProc = GetCurrentNumProc();
    if (theNumProc == NULL)
    {
      PrintErrorMessage('E',"npexecute",
                        "there is no current numerical procedure");
      return (CMDERRORCODE);
    }
  }
  else
  {
    theNumProc = GetNumProcByName (theMG,theNumProcName,"");
    if (theNumProc == NULL)
    {
      PrintErrorMessage('E',"npexecute",
                        "cannot find specified numerical procedure");
      return (CMDERRORCODE);
    }
  }
  if (theNumProc->status != NP_EXECUTABLE) {
    PrintErrorMessage('E',"npexecute",
                      "the num proc is not executable");
    return (CMDERRORCODE);
  }
  if ((err=((*theNumProc->Execute)(theNumProc,argc,argv)))!=0)
  {
    PrintErrorMessageF('E',"npexecute","execution of '%s' failed (error code %d)",theNumProcName,err);
    return (CMDERRORCODE);
  }

  return(OKCODE);
}

/****************************************************************************/
/*D
   npdisplay - display a NumProc

   DESCRIPTION:
   This command displays a NumProc.
   It calls the function 'DisplayNumProc'.

   'npdisplay [<num proc name>] [$c [<class>] | $a]'

   .  <num~proc~name> - name of an existing NumProc (default is the current NumProc)
   .  $c~[<class>]    - list all num procs of <class> or list classes if <class> omitted
   .  $a              - list all num procs

   KEYWORDS:
   multigrid, numerics, userdata, vecdata, matdata, numproc, display, show, print

   SEE ALSO:
   'npcreate', 'npexecute'
   D*/
/****************************************************************************/

static INT NumProcDisplayCommand (INT argc, char **argv)
{
  NP_BASE *theNumProc;
  MULTIGRID *theMG;
  INT i,All,Class,err;
  char theNumProcName[NAMESIZE],ClassName[NAMESIZE];

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"npdisplay","there is no current multigrid\n");
    return (CMDERRORCODE);
  }

  /* check options */
  All = Class = FALSE;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      All = TRUE;
      break;

    case 'c' :
      if (sscanf(argv[i],expandfmt(CONCAT3("c %",NAMELENSTR,"[ -~]")),ClassName)!=1)
      {
        PrintErrorMessage('W',"npdisplay","no class specified\n");
        UserWrite("enroled classes are:\n");
        if (MGListNPClasses(theMG))
          return (CMDERRORCODE);
        return (OKCODE);
      }
      Class = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintHelp("npdisplay",HELPITEM,buffer);
      return (PARAMERRORCODE);
    }
  if (All && Class)
  {
    PrintErrorMessage('E',"npdisplay","a and c option are mutually exclusive");
    return (CMDERRORCODE);
  }
  if (Class)
  {
    if (MGListNPsOfClass(theMG,ClassName))
      return (CMDERRORCODE);
    return (OKCODE);
  }
  if (All)
  {
    if (MGListAllNPs(theMG))
      return (CMDERRORCODE);
    return (OKCODE);
  }

  /* get NumProc */
  if ((sscanf(argv[0],expandfmt(CONCAT3(" npdisplay %",NAMELENSTR,"[ -~]")),theNumProcName)!=1) || (strlen(theNumProcName)==0))
  {
    theNumProc = GetCurrentNumProc();
    if (theNumProc == NULL)
    {
      PrintErrorMessage('E',"npdisplay","there is no current numerical procedure");
      return (CMDERRORCODE);
    }
  }
  else
  {
    theNumProc = GetNumProcByName (theMG,theNumProcName,"");
    if (theNumProc == NULL)
    {
      PrintErrorMessage('E',"npdisplay","cannot find specified numerical procedure");
      return (CMDERRORCODE);
    }
  }
  if ((err=ListNumProc(theNumProc))!=0)
  {
    PrintErrorMessageF('E',"npdisplay","execution of '%s' failed (error code %d)",theNumProcName,err);
    return (CMDERRORCODE);
  }

  return(OKCODE);
}

/****************************************************************************/
/*D
   npcreate - creating a NumProc

   DESCRIPTION:
   This command creates a NumProc for the current multigrid with a given constructor.
   It calls the function 'CreateNumProc'.

   'npcreate <num proc name> $c <constructor>'

   .  <num~proc~name>           - name of the new NumProc
   .  $c~<num~proc~type>	- name of an existing constructor

   KEYWORDS:
   multigrid, numerics, userdata, vecdata, matdata, numproc, create, install

   SEE ALSO:
   'npdisplay', 'npexecute', 'NUM_PROC'
   D*/
/****************************************************************************/

static INT NumProcCreateCommand (INT argc, char **argv)
{
  char theNumProcName[NAMESIZE];
  char ConstructName[NAMESIZE];
  NP_BASE *theNumProc;
  MULTIGRID *theMG;
  INT err,ignore;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"npexecute","there is no current multigrid\n");
    return (CMDERRORCODE);
  }

  /* get NumProc name */
  if ((sscanf(argv[0],expandfmt(CONCAT3(" npcreate %",NAMELENSTR,"[ -~]")),theNumProcName)!=1) || (strlen(theNumProcName)==0))
  {
    PrintErrorMessage('E',"npcreate","specify the name of the theNumProcName to create");
    return (PARAMERRORCODE);
  }
  if (ReadArgvChar("c",ConstructName,argc,argv))
  {
    PrintErrorMessage('E',"npcreate","specify the name of the constructor");
    return (PARAMERRORCODE);
  }
  ignore=ReadArgvOption("i",argc,argv);
  if (ignore)
  {
    theNumProc = GetNumProcByName(theMG,theNumProcName,"");
  }
  if (!ignore || theNumProc==NULL)
    if ((err=CreateObject(theMG,theNumProcName,ConstructName))!=0)
    {
      UserWriteF("creating of '%s' failed (error code %d)\n",theNumProcName,err);
      return (CMDERRORCODE);
    }
  theNumProc = GetNumProcByName(theMG,theNumProcName,"");
  if (SetCurrentNumProc(theNumProc)) return(PARAMERRORCODE);

  return(OKCODE);
}

/****************************************************************************/
/*D
   npinit - inizialize a NumProc

   DESCRIPTION:
   This command inizializes a NumProc.
   It calls the function 'SetNumProc'.

   'npinit <argument list to be passed>'

   KEYWORDS:
   multigrid, numerics, userdata, vecdata, matdata, numproc, initialize, parameters, configure

   SEE ALSO:
   'npcreate', 'npdisplay', 'numerics', 'NUM_PROC'
   D*/
/****************************************************************************/

static INT NumProcInitCommand (INT argc, char **argv)
{
  char theNumProcName[NAMESIZE];
  NP_BASE *theNumProc;
  MULTIGRID *theMG;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"npinit","there is no current multigrid\n");
    return (CMDERRORCODE);
  }

  /* get NumProc */
  if ((sscanf(argv[0],expandfmt(CONCAT3(" npinit %",NAMELENSTR,"[ -~]")),theNumProcName)!=1) || (strlen(theNumProcName)==0))
  {
    theNumProc = GetCurrentNumProc();
    if (theNumProc == NULL)
    {
      PrintErrorMessage('E',"npinit","there is no current numerical procedure");
      return (CMDERRORCODE);
    }
  }
  else
  {
    theNumProc = GetNumProcByName (theMG,theNumProcName,"");
    if (theNumProc == NULL)
    {
      sprintf(buffer,"cannot find specified numerical procedure '%s'",theNumProcName);
      PrintErrorMessage('E',"npinit",buffer);
      return (CMDERRORCODE);
    }
  }
  theNumProc->status = (*theNumProc->Init)(theNumProc,argc,argv);
  switch (theNumProc->status) {
  case NP_NOT_INIT :
    UserWriteF("num proc %s has status NOT_INIT\n",theNumProcName);
    return (CMDERRORCODE);
  case NP_NOT_ACTIVE :
    UserWriteF("num proc %s has status NOT_ACTIVE\n",theNumProcName);
    return (CMDERRORCODE);
  case NP_ACTIVE :
    UserWriteF("num proc %s has status ACTIVE\n",theNumProcName);
    break;
  case NP_EXECUTABLE :
    UserWriteF("num proc %s has status EXECUTABLE\n",theNumProcName);
    break;
  default :
    PrintErrorMessage('E',"npinit","unknown status");
    return (CMDERRORCODE);
  }

  return(OKCODE);
}

/****************************************************************************/
/*D
   scnp - make a NumProc the current NumProc

   DESCRIPTION:
   This command makes a NumProc the current NumProc.
   It sets current num proc.
   It calls the function 'GetNumProcFromName'.

   'scnp <num proc name>'

   . <num~proc~name> - name of an existing NumProc

   KEYWORDS:
   multigrid, numerics, userdata, vecdata, matdata, numproc, active, current

   SEE ALSO:
   'npcreate', 'npdisplay', 'numerics', 'NUM_PROC'
   D*/
/****************************************************************************/

static INT SetCurrentNumProcCommand (INT argc, char **argv)
{
  NP_BASE *theNumProc;
  MULTIGRID *theMG;
  char npname[NAMESIZE];

  NO_OPTION_CHECK(argc,argv);

  /* get solver name */
  if (sscanf(argv[0],expandfmt(CONCAT3(" scnp %",NAMELENSTR,"[ -~]")),npname)!=1)
  {
    PrintHelp("scnp",HELPITEM," (specify current NumProc name)");
    return(PARAMERRORCODE);
  }
  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"npexecute","there is no current multigrid\n");
    return (CMDERRORCODE);
  }
  theNumProc = GetNumProcByName (theMG,npname,"");
  if (theNumProc == NULL)
  {
    PrintErrorMessage('E',"npexecute","cannot find specified numerical procedure");
    return (CMDERRORCODE);
  }

  if (SetCurrentNumProc(theNumProc))
    return (CMDERRORCODE);

  return(OKCODE);
}

/****************************************************************************/
/*D
   createvector - construct vector descriptors

   DESCRIPTION:
   This function creates vector descriptors using templates defined
   in the format.

   'createvector <v1> [<v2> ...] [$t <template>] [$m <name>]'

   .  v1                - vector name
   .  template - template name (default is the first vector template)
   .  name      - multigrid name (default is the current multigrid)

   KEYWORDS:
   multigrid, numerics, userdata, vecdata, create

   SEE ALS0:
   'newformat', 'creatematrix'
   D*/
/****************************************************************************/

static INT CreateVecDescCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char name[NAMESIZE];

  if (ReadArgvChar("m",name,argc,argv))
    theMG = currMG;
  else
    theMG = GetMultigrid(name);
  if (theMG==NULL) {
    PrintErrorMessage('E',"createvector","no current multigrid");
    return(CMDERRORCODE);
  }
  if (CreateVecDescCmd(theMG,argc,argv))
    return(CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   creatematrix - construct matrix

   DESCRIPTION:
   This function creates matrix descriptors using templates defined
   in the format.

   'creatematrix <M1> [<M2> ...] [$t <template>] [$m <name>]'

   .  M1                - matrix name
   .  template - template name (default is the first matrix template)
   .  name      - multigrid name (default is the current multigrid)

   KEYWORDS:
   multigrid, numerics, userdata, matdata, create

   SEE ALS0:
   'newformat', 'createvector'
   D*/
/****************************************************************************/

static INT CreateMatDescCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char name[NAMESIZE];

  if (ReadArgvChar("m",name,argc,argv))
    theMG = currMG;
  else
    theMG = GetMultigrid(name);
  if (theMG==NULL) {
    PrintErrorMessage('E',"creatematrix","no current multigrid");
    return(CMDERRORCODE);
  }
  if (CreateMatDescCmd(theMG,argc,argv))
    return(CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   freematrix - free matrix

   DESCRIPTION:
   This function frees matrix descriptors.

   'freematrix <M1> [<M2> ...] [$m <name>]'

   .  M1                - matrix name
   .  name      - multigrid name (default is the current multigrid)

   KEYWORDS:
   multigrid, numerics, userdata, matdata, create

   SEE ALS0:
   'newformat', 'creatematrix'
   D*/
/****************************************************************************/

static INT FreeMatDescCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char name[NAMESIZE];

  if (ReadArgvChar("m",name,argc,argv))
    theMG = currMG;
  else
    theMG = GetMultigrid(name);
  if (theMG==NULL) {
    PrintErrorMessage('E',"freematrix","no current multigrid");
    return(CMDERRORCODE);
  }
  if (FreeMatDescCmd(theMG,argc,argv))
    return(CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   symlist - list contents of vector and matrix symbols

   DESCRIPTION:
   This command lists the contents of vector and matrix data descriptors of the
   current multigrid

   'symlist {$V [<vd>] | $M [<md>]}'

        Specify either option V or M with or without:~
   .  <vd> - name of a vec data desc, if omitted ALL vec data descs of the current
                        multigrid are listed
   .  <md> - name of a mat data desc, if omitted ALL mat data descs of the current
                        multigrid are listed

   KEYWORDS:
   multigrid, numerics, userdata, vecdata, matdata, list, show, display, print

   SEE ALSO:
   'newformat', 'createvector', 'creatematrix'
   D*/
/****************************************************************************/

static INT SymListCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VECDATA_DESC *vd;
  MATDATA_DESC *md;
  char name[NAMESIZE];
  INT i,res,modifiers;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"npinit","there is no current multigrid\n");
    return (CMDERRORCODE);
  }

  /* first get modifiers */
  modifiers = 0;
  if (ReadArgvOption("scal",argc,argv))
    SET_FLAG(modifiers,SCAL_PROP);
  if (ReadArgvOption("alloc",argc,argv))
    SET_FLAG(modifiers,ALLOC_STAT);

  /* scan V and M options */
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'V' :
      res = sscanf(argv[1],"V %s",name);
      if (res!=1)
      {
        /* print all vectors */
        for (vd = GetFirstVector(theMG); vd != NULL; vd = GetNextVector(vd))
        {
          DisplayVecDataDesc(vd,modifiers,buffer);
          UserWrite(buffer);
        }
        return (OKCODE);
      }
      vd = GetVecDataDescByName(theMG,name);
      if (vd!=NULL)
      {
        DisplayVecDataDesc(vd,modifiers,buffer);
        UserWrite(buffer);
      }
      break;

    case 'M' :
      res = sscanf(argv[1],"M %s",name);
      if (res!=1)
      {
        /* print all matrices */
        for (md = GetFirstMatrix(theMG); md != NULL; md = GetNextMatrix(md))
        {
          DisplayMatDataDesc(md,buffer);
          UserWrite(buffer);
        }
        return (OKCODE);
      }
      md = GetMatDataDescByName(theMG,name);
      if (md!=NULL)
      {
        DisplayMatDataDesc(md,buffer);
        UserWrite(buffer);
        return (OKCODE);
      }
      break;
    }

  return (OKCODE);
}

/****************************************************************************/
/*D
   setkey - associate a command key with a ug command

   DESCRIPTION:
   This command associates a command key with a ug command.
   It calls the function 'SetCmdKey'.

   'setkey $<command char> [$c <comment text>] [$-] $"<command sequence>"*'

   .  $k~<command~char>      - specifiy a single character which will be the command key
   .  $c~<coment~text>       - will be displayed by keylist rather than command(s)
   .  $-                     - keylist prints a hor bar befor this key
   .  $"<command~sequence>"  - give an arbitrary sequence of statements which is to be executed when the command key is pressed

   EXAMPLE:
   'setkey $r $"mark $a" $"refine";'

   Typing <alt> and then 'r' refines all elements on a UNIX system,
   <cmd>+<r> on a Macintosh.

   NOTICE:
   When the command sequence contains a @ character, the following token is interpreted
   as string variable and expanded as usual instantaneously when the command key is created.
   If the sequence contains a ? instead of the @, the string variable will be expanded with its current
   contents at execution time of the command sequence.

   The special character ? is limited however to the 'setkey' command.

   KEYWORDS:
   shortcut, hotkey, create, define

   SEE ALSO:
   'setkey', 'delkey'
   D*/
/****************************************************************************/

static INT SetCommandKeyCommand (INT argc, char **argv)
{
  INT opt,begin,i,j,odd,ShowBar;
  char *ChatPtr;
  char CmdBuffer[INPUTBUFFERLEN],comment[KEY_COMMENT_SIZE];

  /* check input */
  if (argc < 3 )
    return(CMDERRORCODE);

  if (strlen(argv[1])!=1)
  {
    PrintErrorMessage('E',"setkey","only one character for cmd key");
    return (PARAMERRORCODE);
  }

  begin = 2;

  /* comment given? */
  comment[0] = '\0';
  if (argv[begin][0]=='c')
  {
    if (sscanf(argv[begin],expandfmt(CONCAT3("c %",KEY_COMMENT_LEN_STR,"[ -~]")),comment)!=1)
    {
      PrintErrorMessage('E',"setkey","could not read comment");
      return (PARAMERRORCODE);
    }
    begin++;
  }

  /* show bar before in keylist? */
  ShowBar = FALSE;
  if (argv[begin][0]=='-')
  {
    ShowBar = TRUE;
    begin++;
  }

  /* store input */
  ChatPtr = CmdBuffer;
  for (opt=begin; opt<argc; opt++)
  {
    *ChatPtr = '$';
    ChatPtr++;
    strcpy(ChatPtr,argv[opt]);
    ChatPtr += strlen(argv[opt]);
  }

  /* check input */
  if ((argv[begin][0] != '\"') || (argv[argc-1][strlen(argv[argc-1])-1] != '\"'))
    return (CMDERRORCODE);
  j=0;
  for (i=0; i<strlen(CmdBuffer); i++)
    if (CmdBuffer[i] == '\"') j++;
  if (j%2 == 1)
    return (CMDERRORCODE);

  /* reconstruct command */
  odd = 0;
  for (i=0; i<strlen(CmdBuffer); i++)
  {
    if (CmdBuffer[i] != '\"') continue;
    odd = 1-odd;
    if (odd)
    {
      if (CmdBuffer[i-1] != '$')
        return (CMDERRORCODE);
      if (i==1)
        CmdBuffer[0] = ' ';
      else
        CmdBuffer[i-1] = ';';
    }
    CmdBuffer[i]   = ' ';
  }

  /* create cmd key */
  if (SetCmdKey(argv[1][0],comment,ShowBar,CmdBuffer)!=0)
  {
    PrintErrorMessage('E',"setkey","cannot create cmd key");
    return(CMDERRORCODE);
  }

  return(OKCODE);
}

/****************************************************************************/
/*D
   delkey - delete an existing command key

   DESCRIPTION:
   This command deletes an existing command key.
   It calls the function 'DelCmdKey'.

   'delkey $all | $<command char>'

   .  $all                   - delete all command keys allocated before
   .  $<command~char>        - delete only the command key associated with <command char>

   KEYWORDS:
   shortcut, hotkey, remove, undefine

   SEE ALSO:
   'setkey', 'delkey'
   D*/
/****************************************************************************/

static INT DeleteCommandKeyCommand (INT argc, char **argv)
{
  /* check input */
  if (argc!=2)
  {
    PrintHelp("delkey",HELPITEM," (give exactly one argument)");
    return(CMDERRORCODE);
  }

  /* remove key(s) */
  if (strcmp(argv[1],"all")==0)
  {
    if (DelAllCmdKeys()!=0)
    {
      PrintErrorMessage('E',"delkey","failed deleting all cmd keys");
      return(CMDERRORCODE);
    }
  }
  else
  {
    if (DelCmdKey(argv[1][0])!=0)
    {
      PrintErrorMessage('E',"delkey","failed deleting cmd key");
      return(CMDERRORCODE);
    }
  }

  return (OKCODE);
}

/****************************************************************************/
/*D
   keylist - list all existing command keys

   DESCRIPTION:
   This command lists all existing command keys.
   It calls the function 'ListCmdKeys'.

   'keylist [$l]'

   .  $l - show commands and comment (iff)

   KEYWORDS:
   shortcut, hotkey, list, show, display, print

   SEE ALSO:
   'setkey', 'delkey'
   D*/
/****************************************************************************/

static INT ListCommandKeysCommand (INT argc, char **argv)
{
  if (argc>2)
  {
    PrintErrorMessage('E',"setkey","max of one option exceeded");
    return (PARAMERRORCODE);
  }
  if (argc==2)
    if (argv[1][0]=='l')
    {
      ListCmdKeys(YES);
      return (OKCODE);
    }

  ListCmdKeys(NO);

  return (OKCODE);
}

/****************************************************************************/
/*D
   refreshon - sets the refresh state on

   DESCRIPTION:
   This command sets the refresh state on: The pictures on the screen
   device will be updated instantaneously.

   'refreshon [$b [factor]]'

   .  $b - use bullet plotter for refresh

   KEYWORDS:
   graphics, plot, window, picture, plotobject, invalid, update

    SEE ALSO:
        'refreshoff', 'InvalidatePicturesOfMG', 'InvalidateUgWindowsOfMG'
   D*/
/****************************************************************************/

static INT RefreshOnCommand (INT argc, char **argv)
{
  DOUBLE factor = 1.0;

    #ifdef ModelP
  UserWrite("refreshon: not implemented in parallel\n");
    #else
  if (argc >= 2 && argv[1][0] == 'b') {
    sscanf(argv[1],"b %lf", &factor);
    SetRefreshState(ON, YES, factor);
  }
  else
    SetRefreshState(ON, NO, factor);
    #endif

  return(OKCODE);
}

/****************************************************************************/
/*D
   refreshoff - sets the refresh state off

   DESCRIPTION:
   This command sets the refresh state off.

   'refreshoff'

   KEYWORDS:
   graphics, plot, window, picture, plotobject, invalid, update

   SEE ALSO:
   'refreshon'
   D*/
/****************************************************************************/

static INT RefreshOffCommand (INT argc, char **argv)
{
  NO_OPTION_CHECK(argc,argv);

  SetRefreshState(OFF, NO, 1.0);
  return(OKCODE);
}

/****************************************************************************/
/*
   MachineTestCommand - perform a machine test

   SYNOPSIS:
   static INT MachineTestCommand (INT argc, char **argv);

   PARAMETERS:
   .  argc - number of arguments (incl. its own name)
   .  argv - array of strings giving the arguments

   DESCRIPTION:
   This function tests interesting machine parameters.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
 */
/****************************************************************************/

static INT MachineTestCommand (INT argc, char **argv)
{
  /* determine parameters of memory */
        #ifdef ModelP
  if (me == master)
        #endif
  MemoryParameters();

        #ifdef ModelP
  /* determine performance of network */
  /*	NetworkPerformance(); */
        #endif

  return (OKCODE);
}

/****************************************************************************/
/*
   SystemCommand - calls system routine

   SYNOPSIS:
   static INT SystemCommand (INT argc, char **argv);

   PARAMETERS:
   .  argc - number of arguments (incl. its own name)
   .  argv - array of strings giving the arguments

   DESCRIPTION:
   This function calls a system routine.

   'system <system~command>'

   .   <system~command>	- this command string is passed to the system

   KEYWORDS:
   system, cshell, UNIX
   D*/
/****************************************************************************/

static INT SystemCommand (INT argc, char **argv)
{
  char *p;

  if (strlen(argv[0])<8) return (PARAMERRORCODE);
  p = argv[0]+7;

  printf("system \n%s\n",p);

  if (system(p)==-1)
    UserWrite("system-error\n");

  return (OKCODE);
}

/****************************************************************************/
/*D
   resetCEstat - reset counters of control entry usage

   DESCRIPTION:
   This command resets the counters of control entry usage.

   'resetCEstat'

   KEYWORDS:
   control entries

   SEE ALSO:
   'printCEstat'
   D*/
/****************************************************************************/

static INT ResetCEstatCommand (INT argc, char **argv)
{
  NO_OPTION_CHECK(argc,argv);

  ResetCEstatistics();

  return (OKCODE);
}

/****************************************************************************/
/*D
   printCEstat - print counters of control entry usage

   DESCRIPTION:
   This command prints the counters of control entry usage.

   'printCEstat'

   KEYWORDS:
   control entries

   SEE ALSO:
   'resetCEstat'
   D*/
/****************************************************************************/

static INT PrintCEstatCommand (INT argc, char **argv)
{
  NO_OPTION_CHECK(argc,argv);

  PrintCEstatistics();

  return (OKCODE);
}

/****************************************************************************/
/*D
   heapstat - list information about heap of current multigrid

   DESCRIPTION:
   This command lists information on the heap of the current multigrid, calling
   the function 'HeapStat'.

   'heapstat'

   KEYWORDS:
   heap, multigrid, freelists
   D*/
/****************************************************************************/

static INT HeapStatCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: HeapStatCommand(): me not in Context,"\
                     " no heap stat\n",me))
    return(OKCODE);
  }
        #endif

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    UserWrite("no multigrid open\n");
    return (OKCODE);
  }

  HeapStat((const HEAP *)MGHEAP(theMG));

  return (OKCODE);
}

/****************************************************************************/
/*D
   getheapused - put size of used heap of current multigrid into environment variabl :HEAPUSED

   DESCRIPTION:
   This command gets the size of used heap of the current multigrid and puts
   the result in the environment variable :HEAPUSED.

   'getheapused'

   KEYWORDS:
   heap, multigrid, freelists, memory, used, get, free
   D*/
/****************************************************************************/

static INT GetHeapUsedCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  INT used;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: GetHeapUsedCommand(): me not in Context,"\
                     " no heap info\n",me))
    return(OKCODE);
  }
        #endif

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    UserWrite("no multigrid open\n");
    return (OKCODE);
  }

  used = (INT)HeapUsed(MGHEAP(theMG));

        #ifdef ModelP
  used = UG_GlobalMaxINT(used);
        #endif

  if (SetStringValue(":HEAPUSED",used)!=0) {
    PrintErrorMessage('E',"getheapused","could not get string variable :HEAPUSED");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}
/****************************************************************************/
/*
   InitFindRange - create struct where findrange stores results (min and max)

   SYNOPSIS:
   static INT InitFindRange ();

   PARAMETERS:
   .  void

   DESCRIPTION:
   This function creates the struct ':findrange'.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
 */
/****************************************************************************/

static INT InitFindRange (void)
{
  /* install a struct for findrange */
  if (MakeStruct(":findrange")!=0)
    return (1);
  return (0);
}

/****************************************************************************/
/*
   InitScreenSize - create struct :screensize

   SYNOPSIS:
   static INT InitScreenSize ();

   PARAMETERS:
   .  void

   DESCRIPTION:
   This function creates the struct _screensize for the 'screensize' command.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if error occured.
 */
/****************************************************************************/

static INT InitScreenSize (void)
{
  /* install a struct for screensize */
  if (MakeStruct(":screensize")!=0)
    return (1);
  return (0);
}


/****************************************************************************/
/*
   lb - a simple load balancing front end to chaco
                        based on the clustering technique

   DESCRIPTION:
   The lb command performs load balancing.  If not run on a parallel machine
   it will do nothing.  If run on a parallel machine it will try to use Chaco,
   provided the CHACO option for ug was turned on.  If Chaco is not available
   a simple RCB load balancing will be employed.  In the latter case some of
   the optional arguments will be ignored.

   'lb  [<strategy>] [$c <minlevel>] [$d <depth>] [$f <maxlevel>] [$e <minelem>]'

   .  <strategy>		- load balancing strategy
   .  $c <minlevel>	- start load balancing at this level
   .  $d <depth>		- depth of clusters
   .  $f <maxlevel>	- no load balancing above this level
   .  $e <minelem>		- minimal number of elements on each processor

   KEYWORDS:
   parallel, processors, load balance, chaco
 */
/****************************************************************************/

#ifdef ModelP
static INT LBCommand (INT argc, char **argv)
{
                #ifndef ModelP
  /* dummy command in seriell version */
  return(OKCODE);
                #endif

                #ifdef ModelP
  INT res,cmd_error,maxlevel,i;
  int minlevel,cluster_depth,threshold,Const,n,c,
      strategy,eigen,loc,dims,weights,coarse,mode,iopt;
  char levelarg[32];
  MULTIGRID *theMG;
                #ifdef CHACOT
  INT error;
                #endif

  theMG = currMG;

  if (theMG == NULL)
  {
    UserWrite("LBCommand: no open multigrid\n");
    return(OKCODE);
  }

  if (procs==1) return(OKCODE);

  /* defaults */
  minlevel                = 1;
  cluster_depth   = 2;
  maxlevel                = MAXLEVEL;
  threshold               = 1;
  Const                   = 1;
  n                               = 500;
  c                               = 10;
  strategy                = 0;
  eigen                   = 0;
  loc                             = 0;
  dims                    = 1;
  weights                 = 0;
  coarse                  = 0;
  mode                    = 0;
  iopt                    = 10000;

  /* scan lb strategy*/
  res = sscanf(argv[0]," lb %d", &strategy);
  if (res > 1)
  {
    UserWriteF("lb [<strategy>] [$c <minlevel>] [$d <depth>] [$f <maxlevel>] [$e <minelem>] \n");
    UserWriteF("default lb 0 $c 1 $d 2 $e 1\n");
    return(OKCODE);
  }

  /* parse options */
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'c' :
      sscanf(argv[i],"c %d",&minlevel);
      break;

    case 'd' :
                                                #ifdef CHACOT
      sscanf(argv[i],"d %d",&cluster_depth);
                                                #endif
                                                #ifndef CHACOT
      UserWriteF("lb: depth parameter skipped\n");
                                                #endif
      break;

    case 'f' :
      sscanf(argv[i],"f %d",&maxlevel);
      break;

    case 'e' :
                                                #ifdef CHACOT
      sscanf(argv[i],"e %d",&Const);
                                                #endif
                                                #ifndef CHACOT
      UserWriteF("lb: minelem parameter skipped\n");
                                                #endif
      break;

    default :
      UserWriteF("lb [<strategy>] [$c <minlevel>] [$d <depth>] [$f <maxlevel>] [$e <minelem>]\n");
      UserWriteF("default lb 0 $c 1 $d 2\n");
      break;
    }

  /* check for parameter consistency! */
  cmd_error = 0;

  if ((minlevel<0)||(minlevel>TOPLEVEL(theMG)))
  {
    UserWriteF("Choose <minlevel>: 0-%d (toplevel)\n",TOPLEVEL(theMG));
    cmd_error = 1;
  }

                #ifdef CHACOT
  if (strategy<0 || strategy>6)
  {
    UserWriteF("<strategy>: 0-6\n");
    cmd_error = 1;
  }
                #endif
                #ifndef CHACOT
  if (strategy != 0)
  {
    UserWriteF("don't specify <strategy> without Chaco\n!");
    UserWriteF("default <strategy> will be 0 (=RCB)\n!");
    cmd_error = 1;
  }
                #endif

  if (maxlevel < minlevel)
  {
    UserWriteF("Choose <maxlevel>: %d-%d (MAXLEVEL)\n",minlevel,MAXLEVEL);
    cmd_error = 1;
  }
  if (maxlevel < TOPLEVEL(theMG))
  {
    UserWriteF("%s: maxlevel reached: no loadbalancing done!\n"
               "    maxlevel=%d toplevel=%d\n",argv[0],maxlevel,TOPLEVEL(theMG));
    return(OKCODE);
  }

  if (Const < 1)
  {
    UserWriteF("Choose <minelem> > 0\n");
    cmd_error = 1;
  }

  if (strategy==1 || strategy==2)
  {
    if (strategy == 1)
    {
      coarse = 50;
      loc = 1;
    }
    eigen = 1;
  }

  if (strategy>2 || strategy<6)
  {
    weights = 1;
  }

  if (cmd_error) return(CMDERRORCODE);

                #ifdef CHACOT
  PRINTDEBUG(ui,1,("mg %x minlevel %d cluster_depth %d threshold %d\n"
                   "Const %d n %d c %d strategy %d eigen %d loc %d\n"
                   "dims %d weights %d coarse %d mode %d iopt %d\n",
                   theMG,minlevel,cluster_depth,threshold,Const,n,c,
                   strategy,eigen,loc,dims,weights,coarse,mode,iopt));
  error = Balance_CCPTM(theMG,minlevel,cluster_depth,threshold,Const,n,c,
                        strategy,eigen,loc,dims,weights,coarse,mode,iopt);
  if (error>0) return(CMDERRORCODE);
                #endif
                #ifndef CHACOT
  sprintf(levelarg,"%d",minlevel);
  lbs(levelarg, theMG);
                #endif

  return(OKCODE);
                #endif
}


/****************************************************************************/
/*
   lbs - simple or special load balancing functionality

   DESCRIPTION:
   ...

   'lbs ...'

   KEYWORDS:
   parallel, processors, check, load balancing
 */
/****************************************************************************/

static INT LBSCommand (INT argc, char **argv)
{
  MULTIGRID *theCurrMG;

  theCurrMG = currMG;
  if (theCurrMG==NULL)
  {
    PrintErrorMessage('W',"mglist","no multigrid open\n");
    return (OKCODE);
  }

  if (argc==2)
    lbs(argv[1], theCurrMG);
  else
    lbs("0", theCurrMG);

  return(OKCODE);
}

/****************************************************************************/
/*
   context - manipulate current processor context

   DESCRIPTION:
   This command adds/removes processors from the current context.

   'context <processor> | $a | $e'

   .  <processor>		- processor id
   .  $a				- add all processors
   .  $e				- remove all processors (empty context)

   KEYWORDS:
   parallel, processors, display, show, print, DDD, configure
 */
/****************************************************************************/

static INT ContextCommand (INT argc, char **argv)
{
  INT proc = INT_MAX;
  INT flag_all, flag_empty, flag_invert;


  flag_all   = ReadArgvOption("a", argc, argv);
  flag_empty = ReadArgvOption("e", argc, argv);
  flag_invert= ReadArgvOption("i", argc, argv);

  ReadArgvINT("context", &proc, argc, argv);
  if (proc<0 || proc>=procs)
  {
    if (proc!=INT_MAX && me==0)
      UserWriteF("context: invalid processor id (procs=%d)\n", procs);
  }
  else
  {
    /* switch context for proc on/off */
    CONTEXT(proc) = 1-CONTEXT(proc);
  }

  if (proc==INT_MAX)
  {
    if (flag_all && !flag_empty)
    {
      int p; for(p=0; p<procs; p++) CONTEXT(p) = 1;
    }
    if (flag_empty && !flag_all)
    {
      int p; for(p=0; p<procs; p++) CONTEXT(p) = 0;
    }
    if (flag_empty && flag_all)
    {
      if (me==0)
        UserWriteF("context: invalid option combination\n");
    }

    if (flag_invert)
    {
      int p; for(p=0; p<procs; p++) CONTEXT(p) = 1-CONTEXT(p);
    }
  }

  ddd_DisplayContext();
  return(OKCODE);
}

/****************************************************************************/
/*
   pstat - gives information about parallel data structures

   DESCRIPTION:
   ...

   'pstat ...'

   first argument will be passed to DDD

   KEYWORDS:
   parallel, processors, display, show, print, DDD, status, interfaces
 */
/****************************************************************************/

static INT PStatCommand (INT argc, char **argv)
{
  if (argc!=2)
    return (CMDERRORCODE);

  ddd_pstat(argv[1]);
  return(OKCODE);
}

#ifdef __DLB__
/****************************************************************************/
/*
   dlb_eval - gives information current load balancing situation

   DESCRIPTION:
   information on the current load balancing situation of MultiGrid

   'dlb_eval $v <0-n>'


   KEYWORDS:
   load balancing, status, load, interfaces, partition graph
 */
/****************************************************************************/

static INT DLB_EvalCommand (INT argc, char **argv)
{
  INT verbose;
  MULTIGRID       *theMG;

  if (argc > 2) return(CMDERRORCODE);

  theMG = currMG;
  verbose = 0;
  ReadArgvINT("v",&verbose,argc,argv);

  if (dlb_eval(theMG,verbose)!=0) return(CMDERRORCODE);

  return(OKCODE);
}
#endif

#ifdef CHACOT
/****************************************************************************/
/*
   lb4 - load balancer using different (high level) strategies
                        based on the clustering technique

   DESCRIPTION:
   ...

   'lb4 ...'

   KEYWORDS:
   parallel, processors, load balance, chaco
 */
/****************************************************************************/

static INT LB4Command (INT argc, char **argv)
{
  int cmd_error,error,minlevel,cluster_depth,threshold,Const;
  int n,c;
  int strategy,eigen,loc,dims,weights,coarse,mode;
  MULTIGRID *theMG;
  int iopt,i,res,copt;
  char buffer[100];

  theMG = currMG;

  if (theMG == NULL)
  {
    UserWrite("LB4Command: no open multigrid\n");
    return(OKCODE);
  }

  if (procs==1) return(OKCODE);

  /* $i option ? */
  iopt = 100000;
  for (i=1; i<argc; i++)
    if (argv[i][0]=='i')
    {
      sscanf(argv[i]," i %d",&iopt);
      break;
    }

  /* $c option ? */
  copt = 0;
  for (i=1; i<argc; i++)
    if (argv[i][0]=='c')
    {
      copt = 1;
      break;
    }

  /* scan command line arguments */
  res = sscanf(argv[0]," lb4 %d %d %d %d %d %d %d %d %d %d %d %d %d",
               &minlevel,&cluster_depth,
               &threshold,&Const,&n,&c,&strategy,&eigen,&loc,&dims,&weights,
               &coarse,&mode);
  if (res!=13)
  {
    UserWriteF("Wrong number of arguments: need exactly 13!");
    PrintHelp("lb4",HELPITEM,NULL);
    return(CMDERRORCODE);
  }

  /* check for parameter consistency! */
  cmd_error = 0;

  if ((minlevel<0)||(minlevel>TOPLEVEL(theMG)))
  {
    UserWriteF("Choose <minlevel>: 0-%d (toplevel)\n",TOPLEVEL(theMG));
    cmd_error = 1;
  }

  if (strategy<0 || strategy>6)
  {
    UserWriteF("<strategy>: 0-6\n");
    cmd_error = 1;
  }

  if (eigen<0 || eigen>8)
  {
    UserWriteF("<eigen>: 0-8\n");
    cmd_error = 1;
  }

  if (loc<0 || loc>1)
  {
    UserWriteF("<loc>: 0 (no KL) / 1 (use KL local refinement)\n");
    cmd_error = 1;
  }

  if (dims<1 || dims>3)
  {
    UserWriteF("Choose <ndims>: 1-3, 1 bi-, 2 quadri-, 3 octasection\n");
    cmd_error = 1;
  }

  if (weights<0 || weights>3)
  {
    UserWriteF("Choose <weights>: 0-3, 0 no, 1 vertex, 2 edge, 3 both weights\n");
    cmd_error = 1;
  }

  if (strategy==1 && eigen>5)
  {
    UserWriteF("For multlevel strategy (1) choose <eigen>: 1-4\n");
    cmd_error = 1;
  }

  if (strategy==2 && eigen==0)
  {
    UserWriteF("For spectral strategy (2) choose <eigen>: 1-8\n");
    cmd_error = 1;
  }

  if (strategy>2 && (eigen!=0 || coarse!=0))
  {
    UserWriteF("For inertial, linear, random, scattered strategy (3/4/5/6) set <eigen>, <coarse> = 0\n");
    cmd_error = 1;
  }

  if (strategy==1 && loc==0)
  {
    UserWriteF("For multlevel strategy (1) set <loc> = 1\n");
    cmd_error = 1;
  }

  if ((strategy==1 || strategy==2 && eigen>4) && coarse<1)
  {
    UserWriteF("For multilevel method <coarse>: normally 50-500\n");
    cmd_error = 1;
  }
  else if (strategy==2 && eigen<5 && coarse!=0 )
  {
    UserWriteF("NOT using a multilevel method <coarse> = 0\n");
    cmd_error = 1;
  }

  if ((strategy>2 || strategy<6) && weights >1)
  {
    UserWriteF("For inertial, linear, random strategy choose <weights>: 0 no, 1 vertex weights\n");
    cmd_error = 1;
  }

  if (strategy==6 && weights>0)
  {
    UserWriteF("For scattered strategy set <weights> = 0\n");
    cmd_error = 1;
  }

  if (cmd_error) return(CMDERRORCODE);


  /* close if before refinement */
  /*
     if (copt) CloseOnlyTags(theMG);
   */

  error = Balance_CCPTM(theMG,minlevel,cluster_depth,threshold,Const,n,c,
                        strategy,eigen,loc,dims,weights,coarse,mode,iopt);

  if (error>0) return(CMDERRORCODE);

  return(OKCODE);
}
#endif /* CHACOT */

#ifdef USE_FAMG
static INT pamgCheckCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  int level;

  theMG = currMG;
  level = 0;
  if (argv[1][0]=='l')
  {
    if (sscanf(argv[1],"l %d",&level)!=1)
      return (CMDERRORCODE);
  }

  if (pamgCheckDo( theMG, level ) > 0)
    return(CMDERRORCODE);

  return (OKCODE);
}
#endif

#endif /* ModelP */

/****************************************************************************/
/*D
   debug - set or display debug level for ug kernel subsystem

   DESCRIPTION:
   This command sets the debug level for a ug kernel subsystem.

   'debug $<module> [$<level>]'

   .   $<module>	- module can be one of
   .n					init
   .n					dddif
   .n					dev
   .n					dom
   .n					gm
   .n					graph
   .n					low
   .n					machines
   .n					np
   .n					ui
   .n					time
   .n                  pclib
   .n                  appl
   .   $<level>	- assign this level (if omitted display current level for the
                                        specified module)

   KEYWORDS:
   debug, configure, set, display, show, print
   D*/
/****************************************************************************/

#ifdef Debug
static INT DebugCommand (INT argc, char **argv)
{
  char *module;
  int l;

  if (argc<2 || argc>3)
  {
    UserWriteF("usage: debug $<module> [$<level>]\n");
    return (CMDERRORCODE);
  }

  if (argc==3)
  {
    if              (strcmp("init",argv[1])==0) Debuginit               = atoi(argv[2]);
    else if (strcmp("dddif",argv[1])==0) Debugdddif              = atoi(argv[2]);
    else if (strcmp("dev",argv[1])==0) Debugdev                = atoi(argv[2]);
    else if (strcmp("dom",argv[1])==0) Debugdom                = atoi(argv[2]);
    else if (strcmp("gm",argv[1])==0) Debuggm                 = atoi(argv[2]);
    else if (strcmp("graph",argv[1])==0) Debuggraph              = atoi(argv[2]);
    else if (strcmp("low",argv[1])==0) Debuglow                = atoi(argv[2]);
    else if (strcmp("machines",argv[1])==0) Debugmachines   = atoi(argv[2]);
    else if (strcmp("np",argv[1])==0) Debugnp             = atoi(argv[2]);
    else if (strcmp("ui",argv[1])==0) Debugui                 = atoi(argv[2]);
    else if (strcmp("time",argv[1])==0) Debugtime               = atoi(argv[2]);
    else if (strcmp("pclib",argv[1])==0) Debugpclib              = atoi(argv[2]);
    else if (strcmp("appl",argv[1])==0) Debugappl               = atoi(argv[2]);
    else
    {
      UserWriteF("no debug variable for module %s found!\n",argv[1]);
      return (CMDERRORCODE);
    }
    UserWriteF("set debuglevel for module %s to %d\n",argv[1],atoi(argv[2]));
  }
  else
  {
    if              (strcmp("init",argv[1])==0)             {module="init";         l=Debuginit;}
    else if (strcmp("dddif",argv[1])==0)    {module="dddif";        l=Debugdddif;}
    else if (strcmp("dev",argv[1])==0)              {module="dev";          l=Debugdev;}
    else if (strcmp("dom",argv[1])==0)              {module="dom";          l=Debugdom;}
    else if (strcmp("gm",argv[1])==0)               {module="gm";           l=Debuggm;}
    else if (strcmp("graph",argv[1])==0)    {module="graph";        l=Debuggraph;}
    else if (strcmp("low",argv[1])==0)              {module="low";          l=Debuglow;}
    else if (strcmp("machines",argv[1])==0) {module="machines";     l=Debugmachines;}
    else if (strcmp("np",argv[1])==0)           {module="np";           l=Debugnp;}
    else if (strcmp("ui",argv[1])==0)               {module="ui";           l=Debugui;}
    else if (strcmp("time",argv[1])==0)             {module="time";         l=Debugtime;}
    else if (strcmp("pclib",argv[1])==0)    {module="pclib";        l=Debugpclib;}
    else if (strcmp("appl",argv[1])==0)             {module="appl";         l=Debugappl;}
    else
    {
      UserWriteF("no debug variable for module %s found!\n",argv[1]);
      return (CMDERRORCODE);
    }

    UserWriteF("debuglevel for module %s is %d\n",module,l);
  }
  return(OKCODE);
}
#endif

/****************************************************************************/
/*D
   trace - trace a module

   DESCRIPTION:
   This command sets the trace level of modules (blas only up to now).

   'trace [$blas <n>]'
   .   $blas~<n>	- 0: off, 1: show parameters (default),
                                        2: parameters and vector contents (blas level 1)

   KEYWORDS:
   debug, trace, ugblas
   D*/
/****************************************************************************/

#ifdef Debug
static INT TraceCommand (INT argc, char **argv)
{
  int i;

  for (i=1; i<argc; i++)
    STR_SWITCH(argv[i])
    STR_CASE("blas")
    int n;
  if (sscanf(argv[i],"blas %d",&n)==1)
    TraceUGBlas(n);
  else
    TraceUGBlas(TRBL_PARAMS);
  STR_BREAK

  STR_DEFAULT
  sprintf(buffer,"(invalid option '%s')",argv[i]);
  PrintHelp("trace",HELPITEM,buffer);
  return (PARAMERRORCODE);
  STR_BREAK

    STR_SWITCH_END

  return (OKCODE);
}
#endif

/****************************************************************************/
/*D
   reperr - prints the error stack

   DESCRIPTION:
   This command prints the error stack which is created when functios use
   the REP_ERR_RETURN macro. The stack is cleared before each call of a ug-command.

   File and line of the returning functions are printed.

   'reperr'

   KEYWORDS:
   debug, stack, error, display, show, print
   D*/
/****************************************************************************/

#ifdef Debug
static INT RepErrCommand (INT argc, char **argv)
{
  NO_OPTION_CHECK(argc,argv);

  PrintRepErrStack(UserWriteF);

  return (OKCODE);
}
#endif


/****************************************************************************/
/*D
   timing - prints time history

   DESCRIPTION:
   File, line and times are printed.

   'timing'

   KEYWORDS:
   debug
   D*/
/****************************************************************************/

#ifdef Debug
static INT TimingCommand (INT argc, char **argv)
{
  INT i;

  if (ReadArgvOption("r",argc,argv)) {
    DEBUG_TIME_RESET;
    return (OKCODE);
  }
  if (__debug_time_count==0)
    UserWrite("no timing\n");
  else
  {
    UserWrite("timing:\n\n");
    for (i=0; i<__debug_time_count; i++) {
      UserWriteF("%2d: File:%15s, Line:%5d elapsed time%10.4f",
                 i,__debug_time_file[i],__debug_time_line[i],
                 __debug_time[i]-__debug_time[0]);
      if (i > 0) UserWriteF(" diff%8.4f",
                            __debug_time[i]-__debug_time[i-1]);
      UserWriteF("\n");
    }
  }
  return (OKCODE);
}
#endif

/****************************************************************************/
/*D
   showconfig - show the main configuration settings of this ug-program

   DESCRIPTION:
   This command shows the main configuration options having been active
   when this ug was compiled.

   'showconfig'

   KEYWORDS:
   debug, check, configure, show, display, print
   D*/
/****************************************************************************/

static INT ShowConfigCommand (INT argc, char **argv)
{
  NO_OPTION_CHECK(argc,argv);

  UserWrite("Configuration of this programm:\n");

#ifdef _2
  UserWrite("    Dimension:    2\n");
#elif defined _3
  UserWrite("    Dimension:    3\n");
#else
  UserWrite("    Dimension:    unknown\n");
#endif

#ifdef ModelP
  UserWrite("    Model:        parallel\n");
#else
  UserWrite("    Model:        sequential\n");
#endif

#ifdef __AIX__
  UserWrite("    Architecture: AIX");
#elif defined __C90__
  UserWrite("    Architecture: Cray_C90");
#elif defined __CC__
  UserWrite("    Architecture: Parsytec_CC");
#elif defined __DEC__
  UserWrite("    Architecture: DEC");
#elif defined __HP__
  UserWrite("    Architecture: HP");
#elif defined __NECSX4__
  UserWrite("    Architecture: NECSX4");
#elif defined __PARAGON__
  UserWrite("    Architecture: PARAGON");
#elif defined __PC__
  UserWrite("    Architecture: Linux_PC");
#elif defined __POWERGC__
  UserWrite("    Architecture: Parsytec_PowerGC");
#elif defined __SGI__
  UserWrite("    Architecture: SGI");
#elif defined __SUN__
  UserWrite("    Architecture: SUN_Solaris");
#elif defined __SUN4GCC__
  UserWrite("    Architecture: SUN_OS");
#elif defined __T3D__
  UserWrite("    Architecture: Cray_T3D");
#elif defined __T3E__
  UserWrite("    Architecture: Cray_T3E");
#elif defined __MPW32__
  UserWrite("    Architecture: Mac_MPW");
#elif defined __MWCW__
  UserWrite("    Architecture: Mac_CodeWarrior");
#elif defined __PARIX__
  UserWrite("    Architecture: PARIX");
#else
  UserWrite("    Architecture: unknown");
#endif
  UserWriteF(" (%s)\n",ARCHNAME);

#ifdef Debug
  UserWrite("    Debugging:    ON\n");
#elif defined NDEBUG
  UserWrite("    Debugging:    OFF\n");
#else
  UserWrite("    Debugging:    unknown\n");
#endif

#ifdef RIF_SOCKETS
  UserWrite("    remote:       ON\n");
#else
  UserWrite("    remote:       OFF\n");
#endif

  return (OKCODE);
}


/****************************************************************************/
/*D
   array - family of ug-commands to handle n-dimensional arrays of doubles

   DESCRIPTION:
   Each array is a struct in the directory '/Array'. Besides some
   administrational information it contains an ordinary, n-dimensional array
   of doubles as a 'double[n_1][n_2]...[n_k]' definition in C would allocate.
   The maximum number 'k' of dimensions is restricted to 'AR_NVAR_MAX'.

   The provided commands to work with array are the following. The name of
   each command conists of the 2 first letters of its action (e.g. 'sa' for
   'save') and the postfix 'ar' for 'array'.

   CONSTRUCTION and DESTRUCTION:
   .  crar~$n~<name>~{$n_i}+                     - create array of specified size
   .  dear~$n~<name>                                     - delete array

   ACCESS to values:
   .  wrar~$n~<name>~{$n_i}+~$v~<value> - write array[n_1][n_2]...[n_k] := <value>
   .  rear~$n~<name>~{$n_i}+			 - read array[n_1][n_2]...[n_k] to ':ARRAY_VALUE'
   .  clar~$n~<name>					 - clear array, all entries := 0.0

   FILEOPERATIONS:
   .  saar~$n~<name>					 - save array to file '<name>.array'
   .  loar~$n~<name>					 - load array from file '<name>.array'

   EXAMPLE:
   Use the array commands to realize a consecutively numbering of logfiles
   across several runs of the programm.
   .vb
       loar $n filenumber;			# try to load a previously stored array
       if ( :cmdstatus != "0" )
       {                            # no previous array -> the first run
        fnr = -1;					# init the number
        crar $n filenumber $1;      # create an array for only one component
       }
       else
       {                            # previous array is loaded
        rear $n filenumber $0;      # put the filenumber into :ARRAY_VALUE
        fnr = :ARRAY_VALUE;         # get this number
       }
       fnr = fnr + 1;                  # calculate the next number
       wrar $n filenumber $0 $v @fnr;  # write this number (back) to the array
       saar $n filenumber;             # save the array (back) to file
       dear $n filenumber;             # delete array since not longer needed
       set logname log@fnr;

       logon @logname;
   .ve

   KEYWORDS:
   values, array, manipulate, load, store, data, indexed

   SEE ALSO:
   'crar', 'dear', 'wrar', 'rear', 'saar', 'loar', 'clar', 'defaults'
   D*/
/****************************************************************************/

static INT ClearArray (ARRAY *theAR)
{
  INT i, size;

  size = 1;
  for (i=0; i<AR_NVAR(theAR); i++)
    size *= AR_VARDIM(theAR,i);
  for (i=0; i<size; i++)
    AR_DATA(theAR,i) = 0.0;

  return (0);
}

/****************************************************************************/
/*
   CreateArray - allocate a new array structure

   SYNOPSIS:
   static ARRAY *CreateArray (char *name, INT nVar, INT *VarDim);

   PARAMETERS:
   .  name - name under which the array is allocated in '/Array'
   .  nVar - number of dimensions of the data field
   .  VarDim - extension of the data field in each dimension

   DESCRIPTION:
   Allocate a new array structure in the directory '/Array' and
   allocate the data field. The maximum number of dimensions is
   'AR_NVAR_MAX'.

   RETURN VALUE:
   ARRAY *
   .n     pointer to allocated array
   .n     NULL on error.
 */
/****************************************************************************/

static ARRAY *CreateArray (char *name, INT nVar, INT *VarDim)
{
  INT i, size;
  ARRAY *theAR;

  if (nVar<1 || nVar>AR_NVAR_MAX) return (NULL);

  /* change to directory */
  if (ChangeEnvDir("/Array")==NULL)
    return(NULL);

  /* get size */
  size = sizeof(DOUBLE);
  for (i=0; i<nVar; i++)
    size *= VarDim[i];
  size += sizeof(ARRAY) - sizeof(DOUBLE);

  /* allocate structure */
  theAR = (ARRAY*)MakeEnvItem (name,theArrayVarID,size);
  if (theAR==NULL) return (NULL);

  /* init structure */
  ENVITEM_LOCKED(theAR) = 0;
  AR_NVAR(theAR) = nVar;
  for (i=0; i<nVar; i++)
    AR_VARDIM(theAR,i) = VarDim[i];

  if (ClearArray(theAR)) return (NULL);

  return (theAR);
}

/****************************************************************************/
/*
   WriteArray - set one single entry of the data field of the array

   SYNOPSIS:
   static INT WriteArray (ARRAY *theAR, INT *Point, DOUBLE value);

   PARAMETERS:
   .  theAR - array structure to work on
   .  Point - specify the coordinate of the entry in each dimension
   .  value - value to be stored

   DESCRIPTION:
   Set one single entry of the data field of the array to the given value.

   RETURN VALUE:
   INT
   .n    0 always
 */
/****************************************************************************/

static INT WriteArray (ARRAY *theAR, INT *Point, DOUBLE value)
{
  INT i, pos;

  pos = Point[AR_NVAR(theAR)-1];
  for (i=AR_NVAR(theAR)-2; i>=0; i--)
    pos = Point[i] + AR_VARDIM(theAR,i)*pos;
  AR_DATA(theAR,pos) = value;

  return (0);
}

/****************************************************************************/
/*
   ReadArray - read one single entry of the data field of the array

   SYNOPSIS:
   static INT ReadArray (ARRAY *theAR, INT *Point, DOUBLE *value);

   PARAMETERS:
   .  theAR - array structure to work on
   .  Point - specify the coordinate of the entry in each dimension
   .  value - read value

   DESCRIPTION:
   Read one single entry of the data field of the array.

   RETURN VALUE:
   INT
   .n    0 always
 */
/****************************************************************************/

static INT ReadArray (ARRAY *theAR, INT *Point, DOUBLE *value)
{
  INT i, pos;

  pos = Point[AR_NVAR(theAR)-1];
  for (i=AR_NVAR(theAR)-2; i>=0; i--)
    pos = Point[i] + AR_VARDIM(theAR,i)*pos;
  value[0] = AR_DATA(theAR,pos);

  return (0);
}

/****************************************************************************/
/*D
   crar - create a new array structure

   DESCRIPTION:
   Allocate a new array structure in the directory '/Array' and
   allocate the data field of the specified size with the function
   'CreateArray'. The data field is the same as a 'double[n_1][n_2]...[n_k]'
   definition in C would allocate. The maximum number 'k' of dimensions
   is 'AR_NVAR_MAX'. Give the 'n_i' only for the dimensions 'i' you need.

   'crar $n <name> {$<n_i>}+'

   .  <name> - name of the array structure
   .  <n_i>  - extension in the i.th dimension, 1 <= i <= 'AR_NVAR_MAX'

   EXAMPLE:
   .vb
 # Create a 3x7 (2-dimensional) array
   crar $n example_array $3$7;
   .ve

   KEYWORDS:
   values, array, manipulate, load, store, data, indexed

   SEE ALSO:
   'array', 'dear', 'wrar', 'rear', 'saar', 'loar', 'clar'
   D*/
/****************************************************************************/

static INT CreateArrayCommand (INT argc, char **argv)
{
  INT i, nVar, VarDim[AR_NVAR_MAX];
  int iValue;
  char name[128];

  nVar = argc-2;
  if (nVar<1 || nVar>AR_NVAR_MAX)
    return (CMDERRORCODE);
  if (argv[1][0]=='n')
  {
    if (sscanf(argv[1],"n %s",name)!=1)
      return (CMDERRORCODE);
  }
  for (i=0; i<nVar; i++)
  {
    if (sscanf(argv[i+2],"%d",&iValue)!=1)
      return (CMDERRORCODE);
    if (iValue<1)
      return (CMDERRORCODE);
    VarDim[i] = iValue;
  }

  /* create Array */
  if (CreateArray(name,nVar,VarDim)==NULL)
    return (CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   dear - delete an existing array structure

   DESCRIPTION:
   Delete the already existing array. The entry in the directory '/Array'
   is removed and the data field of the array is freed.

   'dear $n <name>'

   .  <name> - name of the array structure

   KEYWORDS:
   values, array, manipulate, load, store, data, indexed

   SEE ALSO:
   'array', 'dear', 'wrar', 'rear', 'saar', 'loar', 'clar'
   D*/
/****************************************************************************/

static INT DeleteArrayCommand (INT argc, char **argv)
{
  char name[128];
  ARRAY *theAR;

  if (argv[1][0]=='n')
  {
    if (sscanf(argv[1],"n %s",name)!=1)
      return (CMDERRORCODE);
  }

  /* find array */
  if (ChangeEnvDir("/Array")==NULL)
  {
    PrintErrorMessage('F',"DeleteArrayCommand","could not changedir to /Array");
    return(CMDERRORCODE);
  }
  theAR = (ARRAY *)SearchEnv(name,".",theArrayVarID,SEARCHALL);
  if (theAR==NULL)
    return (CMDERRORCODE);

  /* delete Array */
  if (RemoveEnvItem((ENVITEM *)theAR))
    return (CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   saar - save an array to file

   DESCRIPTION:
   Store the content of the array into a file with name '<array name>.array'.
   The 'arraypathes' entry in the 'defaults' file is considered.

   'saar $n <name>'

   .  <name> - name of the array structure

   REMARK:
   The file is written in the binary mode, thus be careful when exchanging
   the computer architecture.

   KEYWORDS:
   values, array, manipulate, load, store, data, indexed

   SEE ALSO:
   'array', 'dear', 'wrar', 'rear', 'saar', 'loar', 'clar', 'defaults'
   D*/
/****************************************************************************/

static INT SaveArrayCommand (INT argc, char **argv)
{
  INT i, size;
  char name[128];
  ARRAY *theAR;
  FILE *stream;

  if (argv[1][0]=='n')
  {
    if (sscanf(argv[1],"n %s",name)!=1)
      return (CMDERRORCODE);
  }

  /* find array */
  if (ChangeEnvDir("/Array")==NULL)
  {
    PrintErrorMessage('F',"SaveArrayCommand","could not changedir to /Array");
    return(CMDERRORCODE);
  }
  theAR = (ARRAY *)SearchEnv(name,".",theArrayVarID,SEARCHALL);
  if (theAR==NULL)
    return (CMDERRORCODE);

  /* save Array */
  strcat(name,".array");
  if (arraypathes_set)
    stream = FileOpenUsingSearchPaths(name,"w","arraypathes");
  else
    stream = fileopen(name,"w");
  if (stream==NULL)
  {
    PrintErrorMessage('E',"SaveArrayCommand","cannot open file");
    return(CMDERRORCODE);
  }

  /* store */
  if (fwrite((void*)(&(theAR->nVar)),sizeof(INT),1,stream)!=1) return(CMDERRORCODE);
  if (fwrite((void*)theAR->VarDim,sizeof(INT),AR_NVAR(theAR),stream)!=AR_NVAR(theAR)) return(CMDERRORCODE);
  size = 1;
  for (i=0; i<AR_NVAR(theAR); i++)
    size *= AR_VARDIM(theAR,i);
  if (fwrite((void*)(theAR->data),sizeof(DOUBLE),size,stream)!=size) return(CMDERRORCODE);
  if (fclose(stream)) return(CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   loar - load and allocates an array from file

   DESCRIPTION:
   Load the content of the array from the file with name '<array name>.array'.
   The 'arraypathes' entry in the 'defaults' file is considered. A new array
   structure with the given name is allocated in the directory '/Array'.

   'loar $n <name>'

   .  <name> - name of the array structure

   REMARK:
   The file is written in the binary mode, thus be careful when exchanging
   the computer architecture.

   KEYWORDS:
   values, array, manipulate, load, store, data, indexed

   SEE ALSO:
   'array', 'dear', 'wrar', 'rear', 'saar', 'loar', 'clar', 'defaults'
   D*/
/****************************************************************************/

static INT LoadArrayCommand (INT argc, char **argv)
{
  INT i, size, nVar, VarDim[AR_NVAR_MAX];
  char name[128], filename[128];
  ARRAY *theAR;
  FILE *stream;

  if (argv[1][0]=='n')
  {
    if (sscanf(argv[1],"n %s",name)!=1)
      return (CMDERRORCODE);
  }

  strcpy(filename,name);
  strcat(filename,".array");
  if (arraypathes_set)
    stream = FileOpenUsingSearchPaths(filename,"r","arraypathes");
  else
    stream = fileopen(filename,"r");
  if (stream==NULL)
  {
    PrintErrorMessage('E',"LoadArrayCommand","cannot open file");
    return(CMDERRORCODE);
  }
  if (fread((void*)(&nVar),sizeof(INT),1,stream)!=1)
    return(CMDERRORCODE);
  if (nVar>AR_NVAR_MAX)
    return (CMDERRORCODE);
  if (fread((void*)VarDim,sizeof(INT),nVar,stream)!=nVar)
    return(CMDERRORCODE);
  theAR = CreateArray (name,nVar,VarDim);
  if (theAR==NULL)
    return(CMDERRORCODE);
  size = 1;
  for (i=0; i<AR_NVAR(theAR); i++)
    size *= AR_VARDIM(theAR,i);
  if (fread((void*)(theAR->data),sizeof(DOUBLE),size,stream)!=size)
    return(CMDERRORCODE);
  if (fclose(stream))
    return(CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   wrar - write value into one single entry of the array

   DESCRIPTION:
   Write the given double-value into the specified entry of the array.

   'wrar $n <name> {$<n_i>}+ $v <value>'

   .  <name>  - name of the array structure
   .  <n_i>   - i.th coordinate of the entry, 0 <= n_i < allocated extension
   .  <value> - double value to be stored

   REMARK:
   Like in C if the array is allocated with 'n' components in a dimension,
   the valid indices for writing are 0,..,'n'-1. An index outside this
   range causes an error. Give coordinate for all allocated dimensions.

   EXAMPLE:
   .vb
 # perform array[2][5] := 1.0
   wrar $n example_array $2$5 $v 1.0;
   .ve

   KEYWORDS:
   values, array, manipulate, load, store, data, indexed

   SEE ALSO:
   'array', 'dear', 'wrar', 'rear', 'saar', 'loar', 'clar', 'defaults'
   D*/
/****************************************************************************/

static INT WriteArrayCommand (INT argc, char **argv)
{
  INT i, Point[AR_NVAR_MAX];
  int iValue;
  double Value;
  char name[128];
  ARRAY *theAR;

  if (argv[1][0]=='n')
  {
    if (sscanf(argv[1],"n %s",name)!=1)
      return (CMDERRORCODE);
  }
  if (ChangeEnvDir("/Array")==NULL)
  {
    PrintErrorMessage('F',"WriteArrayCommand","could not changedir to /Array");
    return(CMDERRORCODE);
  }
  theAR = (ARRAY *)SearchEnv(name,".",theArrayVarID,SEARCHALL);
  if (theAR==NULL)
    return (CMDERRORCODE);

  if (AR_NVAR(theAR) != argc-3)
    return (CMDERRORCODE);
  for (i=0; i<AR_NVAR(theAR); i++)
  {
    if (sscanf(argv[i+2],"%d",&iValue)!=1)
      return (CMDERRORCODE);
    if (iValue<0 || iValue>=AR_VARDIM(theAR,i))
    {
      PrintErrorMessage( 'E', "WriteArrayCommand", "Index Range Error" );
      return (CMDERRORCODE);
    }
    Point[i] = iValue;
  }

  /* write */
  if (sscanf(argv[argc-1],"v %lf",&Value)!=1)
    return (CMDERRORCODE);
  if (WriteArray(theAR,Point,(DOUBLE)Value))
    return (CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   rear - read the value from one single entry of the array

   DESCRIPTION:
   Read the the specified value from the array and store it in the
   environment variable ':ARRAY_VALUE'.

   'rear $n <name> {$<n_i>}+'

   .  <name>  - name of the array structure
   .  <n_i>   - i.th coordinate of the entry, 0 <= n_i < allocated extension

   REMARK:
   Like in C if the array is allocated with 'n' components in a dimension,
   the valid indices for reading are 0,..,'n'-1. An index outside this
   range causes an error. Give coordinate for all allocated dimensions.

   EXAMPLE:
   .vb
 # retrieve array[2][5]
   rear $n example_array $2$5;
 # and display the value
   set :ARRAY_VALUE;
   .ve

   KEYWORDS:
   values, array, manipulate, load, store, data, indexed

   SEE ALSO:
   'array', 'dear', 'wrar', 'rear', 'saar', 'loar', 'clar', 'defaults'
   D*/
/****************************************************************************/

static INT ReadArrayCommand (INT argc, char **argv)
{
  INT i, Point[AR_NVAR_MAX];
  int iValue;
  DOUBLE value;
  char name[128];
  ARRAY *theAR;

  if (argv[1][0]=='n')
  {
    if (sscanf(argv[1],"n %s",name)!=1)
      return (CMDERRORCODE);
  }
  if (ChangeEnvDir("/Array")==NULL)
  {
    PrintErrorMessage('F',"ReadArrayCommand","could not changedir to /Array");
    return(CMDERRORCODE);
  }
  theAR = (ARRAY *)SearchEnv(name,".",theArrayVarID,SEARCHALL);
  if (theAR==NULL)
    return (CMDERRORCODE);

  if (AR_NVAR(theAR) != argc-2)
    return (CMDERRORCODE);
  for (i=0; i<AR_NVAR(theAR); i++)
  {
    if (sscanf(argv[i+2],"%d",&iValue)!=1)
      return (CMDERRORCODE);
    if (iValue<0 || iValue>=AR_VARDIM(theAR,i))
    {
      PrintErrorMessage( 'E', "ReadArrayCommand", "Index Range Error" );
      return (CMDERRORCODE);
    }
    Point[i] = iValue;
  }

  /* read */
  if (ReadArray(theAR,Point,&value))
    return (CMDERRORCODE);
  if (SetStringValue(":ARRAY_VALUE",(double)value))
    return (CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   clar - set all entries of the array to 0.0

   DESCRIPTION:
   Set all entries of the data field contained in the array structure to 0.0.

   'clar $n <name>'

   .  <name> - name of the array structure

   KEYWORDS:
   values, array, manipulate, load, store, data, indexed

   SEE ALSO:
   'array', 'dear', 'wrar', 'rear', 'saar', 'loar', 'clar', 'defaults'
   D*/
/****************************************************************************/

static INT ClearArrayCommand (INT argc, char **argv)
{
  char name[128];
  ARRAY *theAR;

  if (argv[1][0]=='n')
  {
    if (sscanf(argv[1],"n %s",name)!=1)
      return (CMDERRORCODE);
  }
  if (ChangeEnvDir("/Array")==NULL)
  {
    PrintErrorMessage('F',"ClearArrayCommand","could not changedir to /Array");
    return(CMDERRORCODE);
  }
  theAR = (ARRAY *)SearchEnv(name,".",theArrayVarID,SEARCHALL);
  if (theAR==NULL)
    return (CMDERRORCODE);

  if (ClearArray(theAR))
    return (CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/*D
   InitArray - Initialization of the array commands

   SYNOPSIS:
   INT InitArray ();

   PARAMETERS:
   .  void -

   DESCRIPTION:
   This function does initialization of the ug-commands concerning arrays.

   SEE ALSO:
   array, crar, dear, wrar, rear, saar, loar, clar

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    __LINE__ if error occured.
   D*/
/****************************************************************************/

static INT InitArray (void)
{
  /* install the /Array directory */
  if (ChangeEnvDir("/")==NULL)
  {
    PrintErrorMessage('F',"InitArray","could not changedir to root");
    return(__LINE__);
  }
  theArrayDirID = GetNewEnvDirID();
  if (MakeEnvItem("Array",theArrayDirID,sizeof(ENVDIR))==NULL)
  {
    PrintErrorMessage('F',"InitArray","could not install '/Array' dir");
    return(__LINE__);
  }
  theArrayVarID = GetNewEnvVarID();

  /* path to dir for 'array' files */
  arraypathes_set = FALSE;
  if (ReadSearchingPaths(DEFAULTSFILENAME,"arraypathes")==0)
    arraypathes_set = TRUE;

  return (0);
}


/****************************************************************************/
/*D
   dumpalg - dumps the surface algebra to the interface

   DESCRIPTION:
   Dumps the surface algebra

   'dumpalg <name>'

   .  <name> - file name

   KEYWORDS:
   data, io

   SEE ALSO:
   D*/
/****************************************************************************/

static INT DumpAlgCommand(INT argc, char **argv)
{
  INT level, comp;
  VECTOR *v;
  MULTIGRID *theMG;
  GRID *theGrid;
  VECDATA_DESC *v_desc;
  char buffer[1024];

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"dumpalg","no open multigrid");
    return (CMDERRORCODE);
  }

  v_desc = ReadArgvVecDesc(theMG,"v",argc,argv);
  if (v_desc == NULL)
  {
    PrintErrorMessage('E',"dumpalg","wrong vector specification");
    return (CMDERRORCODE);
  }
  UserWriteF(DISPLAY_NP_FORMAT_SS,"vector displayed",ENVITEM_NAME(v_desc));
  DisplayVecDataDesc(v_desc,~0,buffer);

  for (level=0; level<=TOPLEVEL(theMG); level++)
  {
    theGrid = GRID_ON_LEVEL(theMG,level);

#ifdef ModelP
    for( v=PFIRSTVECTOR(theGrid); v!=NULL; v=SUCCVC(v) )
#else
    for( v=FIRSTVECTOR(theGrid); v!=NULL; v=SUCCVC(v) )
#endif
    {
      printf( "Vec key=%d level=%d type=%d pe=%d fine=%d new_def=%d ",
              KeyForObject((KEY_OBJECT*)v),level,VTYPE(v),me,
              FINE_GRID_DOF(v),NEW_DEFECT(v) );
      for( comp=0; comp<VD_NCMPS_IN_TYPE(v_desc,VTYPE(v)); comp++ )
        printf(" %g ",comp,VVALUE(v,VD_CMP_OF_TYPE(v_desc,VTYPE(v),comp)));
      printf("\n");
    }
  }

  /* aus nstools.c
          {
                  MATRIX *m;
              DisplayMatDataDesc(m,buffer);
              fprintf(fptr,"%s",buffer);

              fprintf(fptr,"Matrix:\n");
              for (vec=FIRSTVECTOR(theGrid); vec!=NULL; vec=SUCCVC(vec))
              {
                  for (mat=VSTART(vec); mat!=NULL; mat=MNEXT(mat))
                  {
                      fprintf(fptr,"%d\t%d\n",(int)VINDEX(vec),(int)VINDEX(MDEST(mat)));
                      fprintf(fptr,"f\tt");
                      for (i=0; i<MD_ROWS_IN_RT_CT(m,VTYPE(vec),VTYPE(MDEST(mat))); i++)
                      {
                          for (j=0; j<MD_COLS_IN_RT_CT(m,VTYPE(vec),VTYPE(MDEST(mat))); j+
     +)
                              fprintf(fptr,"\t%.9e",(double)MVALUE(mat,MD_IJ_CMP_OF_RT_CT
          (m,VTYPE(vec),VTYPE(MDEST(mat)),i,j)));
                          fprintf(fptr,"\n\t");
                      }
                      fprintf(fptr,"\n");
                  }
              }
          }
   */

  return(OKCODE);
}

/****************************************************************************/
/* Quick Hack for periodic boundaries                                       */
/****************************************************************************/

#ifdef __PERIODIC_BOUNDARY__
static INT ListPeriodicPosCommand (INT argc, char **argv)
{
  MULTIGRID *theMG = currMG;
  DOUBLE_VECTOR pos;

  if (theMG == NULL)
  {
    UserWrite("ListPeriodicPos: no open multigrid\n");
    return(OKCODE);
  }

#ifdef __THREEDIM__
  if (sscanf(argv[1],"%lf %lf %lf",pos,pos+1,pos+2) != 3)
#else
  if (sscanf(argv[1],"%lf %lf",pos,pos+1) != 2)
#endif
  {
    if (me == master)
      UserWriteF("ListPeriodicPos wrong number of coords\n");
  }

  if (MG_ListPeriodicPos(theMG,0,TOPLEVEL(theMG),pos))
    REP_ERR_RETURN (CMDERRORCODE);

  return (OKCODE);
}

static INT MakePeriodicCommand (INT argc, char **argv)
{
  MULTIGRID *theMG = currMG;

  if (theMG == NULL)
  {
    UserWrite("MakePeriodic: no open multigrid\n");
    return(OKCODE);
  }

  if (MG_GeometricToPeriodic(theMG,0,TOPLEVEL(theMG)))
    REP_ERR_RETURN (CMDERRORCODE);

  return (OKCODE);
}
#endif


/****************************************************************************/
/*
   InitCommands - Initialization of the commands

   SYNOPSIS:
   INT InitCommands ();

   PARAMETERS:
   .  void -

   DESCRIPTION:
   This function does initialization of all ug-commands, using
   'CreateCommand'.
   It initializes 'clock', 'findrange', 'screensize' and 'array'
   commands.

   SEE ALSO:
   commands

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    __LINE__ if error occured.
 */
/****************************************************************************/

INT InitCommands ()
{
  /* quick hack */
#ifdef __PERIODIC_BOUNDARY__
  if (CreateCommand("makeperiodic",       MakePeriodicCommand                             )==NULL) return (__LINE__);
  if (CreateCommand("lppos",                      ListPeriodicPosCommand                  )==NULL) return (__LINE__);
#endif

  /* general commands */
  if (CreateCommand("quit",                       QuitCommand                                     )==NULL) return (__LINE__);
  if (CreateCommand("exitug",                     ExitUgCommand                                   )==NULL) return (__LINE__);
  if (CreateCommand("help",                       HelpCommand                                     )==NULL) return (__LINE__);
  if (CreateCommand("checkhelp",          CheckHelpCommand                                )==NULL) return (__LINE__);
  if (CreateCommand("readclock",          ReadClockCommand                                )==NULL) return (__LINE__);
  if (CreateCommand("resetclock",         ResetClockCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("date",                       DateCommand                                     )==NULL) return (__LINE__);

  /* commands for environement management */
  if (CreateCommand("cd",                         ChangeEnvCommand                                )==NULL) return (__LINE__);
  if (CreateCommand("ls",                         ListEnvCommand                                  )==NULL) return (__LINE__);
  if (CreateCommand("pwd",                        PrintEnvDirCommand                              )==NULL) return (__LINE__);
  if (CreateCommand("envinfo",            EnvInfoCommand                                  )==NULL) return (__LINE__);
  if (CreateCommand("set",                        SetCommand                                              )==NULL) return (__LINE__);
  if (CreateCommand("dv",                         DeleteVariableCommand                   )==NULL) return (__LINE__);
  if (CreateCommand("ms",                         MakeStructCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("cs",                         ChangeStructCommand                             )==NULL) return (__LINE__);
  if (CreateCommand("pws",                        PrintWorkStructCommand                  )==NULL) return (__LINE__);
  if (CreateCommand("ds",                         DeleteStructCommand                     )==NULL) return (__LINE__);


  /* commands for protocol and logfile output */
  if (CreateCommand("protoOn",            ProtoOnCommand                                  )==NULL) return (__LINE__);
  if (CreateCommand("protoOff",           ProtoOffCommand                                 )==NULL) return (__LINE__);
  if (CreateCommand("protocol",           ProtocolCommand                                 )==NULL) return (__LINE__);
  if (CreateCommand("logon",                      LogOnCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("logoff",             LogOffCommand                                   )==NULL) return (__LINE__);
        #ifdef __TWODIM__
  if (CreateCommand("cnom",                       CnomCommand                                             )==NULL) return (__LINE__);
        #endif

  /* commands for grid management */
  if (CreateCommand("configure",          ConfigureCommand                                )==NULL) return (__LINE__);
  if (CreateCommand("setcurrmg",          SetCurrentMultigridCommand              )==NULL) return (__LINE__);
  if (CreateCommand("new",                        NewCommand                                              )==NULL) return (__LINE__);
  if (CreateCommand("open",                       OpenCommand                                     )==NULL) return (__LINE__);
  if (CreateCommand("close",                      CloseCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("save",                       SaveCommand                                     )==NULL) return (__LINE__);
  if (CreateCommand("savedomain",         SaveDomainCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("savedata",           SaveDataCommand                                 )==NULL) return (__LINE__);
  if (CreateCommand("loaddata",           LoadDataCommand                                 )==NULL) return (__LINE__);
  if (CreateCommand("changemc",           ChangeMagicCookieCommand                )==NULL) return (__LINE__);
  if (CreateCommand("level",                      LevelCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("average",        AverageCommand                  )==NULL) return (__LINE__);
  if (CreateCommand("freeaverage",    FreeAverageCommand              )==NULL) return (__LINE__);
  if (CreateCommand("renumber",           RenumberMGCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("smooth",             SmoothMGCommand                                 )==NULL) return (__LINE__);
  if (CreateCommand("smoothgrid",     SmoothGridCommand               )==NULL) return(__LINE__);
  if (CreateCommand("ordernodes",         OrderNodesCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("lexorderv",          LexOrderVectorsCommand                  )==NULL) return (__LINE__);
  if (CreateCommand("orderv",             OrderVectorsCommand                     )==NULL) return (__LINE__);
  if (CreateCommand("lineorderv",         LineOrderVectorsCommand                 )==NULL) return (__LINE__);
  if (CreateCommand("revvecorder",        RevertVecOrderCommand                   )==NULL) return (__LINE__);
  if (CreateCommand("shellorderv",        ShellOrderVectorsCommand                )==NULL) return (__LINE__);
  if (CreateCommand("setindex",           SetIndexCommand                                 )==NULL) return (__LINE__);
  if (CreateCommand("extracon",           ExtraConnectionCommand                  )==NULL) return (__LINE__);
  if (CreateCommand("check",                      CheckCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("in",                         InsertInnerNodeCommand                  )==NULL) return (__LINE__);
  if (CreateCommand("ngin",                       NGInsertInnerNodeCommand                )==NULL) return (__LINE__);
  if (CreateCommand("bn",                         InsertBoundaryNodeCommand               )==NULL) return (__LINE__);
  if (CreateCommand("ngbn",                       NGInsertBoundaryNodeCommand             )==NULL) return (__LINE__);
  if (CreateCommand("gn",                         InsertGlobalNodeCommand                 )==NULL) return (__LINE__);
  if (CreateCommand("deln",                       DeleteNodeCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("move",                       MoveNodeCommand                                 )==NULL) return (__LINE__);
  if (CreateCommand("ie",                         InsertElementCommand                    )==NULL) return (__LINE__);
  if (CreateCommand("ngie",                       NGInsertElementCommand                  )==NULL) return (__LINE__);
  if (CreateCommand("dele",                       DeleteElementCommand                    )==NULL) return (__LINE__);
  if (CreateCommand("refine",             AdaptCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("adapt",                      AdaptCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("fixcoarsegrid",      FixCoarseGridCommand                    )==NULL) return (__LINE__);
  if (CreateCommand("collapse",           CollapseCommand                         )==NULL) return (__LINE__);
  if (CreateCommand("mark",                       MarkCommand                                     )==NULL) return (__LINE__);
  if (CreateCommand("find",                       FindCommand                                     )==NULL) return (__LINE__);
  if (CreateCommand("select",             SelectCommand                                   )==NULL) return (__LINE__);
  if (CreateCommand("wplist",             WindowPictureListCommand                )==NULL) return (__LINE__);
  if (CreateCommand("mglist",             MGListCommand                                   )==NULL) return (__LINE__);
  if (CreateCommand("glist",                      GListCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("nlist",                      NListCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("elist",                      EListCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("slist",                      SelectionListCommand                    )==NULL) return (__LINE__);
  if (CreateCommand("rlist",                      RuleListCommand                                 )==NULL) return (__LINE__);
  if (CreateCommand("printvalue",         PrintValueCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("vmlist",             VMListCommand                                   )==NULL) return (__LINE__);
  if (CreateCommand("convert",        ConvertCommand                  )==NULL) return(__LINE__);
  if (CreateCommand("quality",            QualityCommand                                  )==NULL) return (__LINE__);
  if (CreateCommand("makegrid",           MakeGridCommand                                 )==NULL) return (__LINE__);
  if (CreateCommand("status",                     StatusCommand                                   )==NULL) return (__LINE__);

#if defined(CAD) && defined(__THREEDIM__)
  if (CreateCommand("cadconvert",     CADGridConvertCommand           )==NULL) return (__LINE__);
#endif

  /* commands for grape */
  if (CreateCommand("grape",                      CallGrapeCommand                                )==NULL) return (__LINE__);
#ifdef _COVISE
  /* commands for covise */
  if (CreateCommand("covise",                     CoviseCommand                                   )==NULL) return (__LINE__);
#endif

  /* commands for window and picture management */
  if (CreateCommand("screensize",         ScreenSizeCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("openwindow",         OpenWindowCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("openppic",       OpenPlacedPicturesCommand           )==NULL) return (__LINE__);
  if (CreateCommand("closewindow",        CloseWindowCommand                              )==NULL) return (__LINE__);
  if (CreateCommand("setcurrwindow",      SetCurrentWindowCommand                 )==NULL) return (__LINE__);
  if (CreateCommand("drawtext",           DrawTextCommand                                 )==NULL) return (__LINE__);
  if (CreateCommand("openpicture",        OpenPictureCommand                              )==NULL) return (__LINE__);
  if (CreateCommand("closepicture",       ClosePictureCommand                     )==NULL) return (__LINE__);
  if (CreateCommand("clearpicture",       ClearPictureCommand                     )==NULL) return (__LINE__);
  if (CreateCommand("picframe",           PicFrameCommand                                 )==NULL) return (__LINE__);
  if (CreateCommand("setcurrpicture", SetCurrentPictureCommand            )==NULL) return (__LINE__);
  if (CreateCommand("picwin",                     PictureWindowCommand                    )==NULL) return (__LINE__);
  if (CreateCommand("setview",            SetViewCommand                                  )==NULL) return (__LINE__);
  if (CreateCommand("cpview",                     CopyViewCommand                                 )==NULL) return (__LINE__);
  if (CreateCommand("vdisplay",           DisplayViewCommand                              )==NULL) return (__LINE__);
  if (CreateCommand("walk",                       WalkCommand                                     )==NULL) return (__LINE__);
  if (CreateCommand("walkaround",         WalkAroundCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("zoom",                       ZoomCommand                                     )==NULL) return (__LINE__);
  if (CreateCommand("drag",                       DragCommand                                     )==NULL) return (__LINE__);
  if (CreateCommand("rotate",             RotateCommand                                   )==NULL) return (__LINE__);
  if (CreateCommand("textfac",            TextFacCommand                                  )==NULL) return (__LINE__);
  if (CreateCommand("setplotobject",      SetPlotObjectCommand                    )==NULL) return (__LINE__);
  if (CreateCommand("polist",             PlotObjectListCommand                   )==NULL) return (__LINE__);
  if (CreateCommand("plot",                       PlotCommand                                     )==NULL) return (__LINE__);
  if (CreateCommand("findrange",          FindRangeCommand                                )==NULL) return (__LINE__);
  if (CreateCommand("updateDoc",          UpdateDocumentCommand                   )==NULL) return (__LINE__);
  if (CreateCommand("rotmode",            RotModeCommand                                  )==NULL) return (__LINE__);
  if (CreateCommand("cmfn",                       CreateMetafileNameCommand               )==NULL) return (__LINE__);
  if (CreateCommand("setpalette",         SetPaletteCommand                               )==NULL) return (__LINE__);

  /* commands for problem management */
  if (CreateCommand("reinit",             ReInitCommand                                   )==NULL) return (__LINE__);

  /* commands for NumProc management */
  if (CreateCommand("npexecute",          ExecuteNumProcCommand                   )==NULL) return (__LINE__);
  if (CreateCommand("npdisplay",          NumProcDisplayCommand                   )==NULL) return (__LINE__);
  if (CreateCommand("npcreate",           NumProcCreateCommand                    )==NULL) return (__LINE__);
  if (CreateCommand("npinit",                     NumProcInitCommand                              )==NULL) return (__LINE__);
  if (CreateCommand("scnp",                       SetCurrentNumProcCommand                )==NULL) return (__LINE__);

  /* vectors and matrices */
  if (CreateCommand("clear",                      ClearCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("mflops",         MFLOPSCommand                   )==NULL) return (__LINE__);
  if (CreateCommand("makevdsub",      MakeVDsubCommand                )==NULL) return (__LINE__);

  if (CreateCommand("rand",                       RandCommand                                             )==NULL) return (__LINE__);
  if (CreateCommand("copy",                       CopyCommand                                             )==NULL) return (__LINE__);
  if (CreateCommand("add",                        AddCommand                                              )==NULL) return (__LINE__);
  if (CreateCommand("sub",                        SubCommand                                              )==NULL) return (__LINE__);
  if (CreateCommand("homotopy",       HomotopyCommand                 )==NULL) return(__LINE__);
  if (CreateCommand("interpolate",        InterpolateCommand                              )==NULL) return (__LINE__);

  /* formats */
  if (CreateCommand("newformat",          CreateFormatCommand                             )==NULL) return (__LINE__);
  if (CreateCommand("delformat",          DeleteFormatCommand                             )==NULL) return (__LINE__);
  if (CreateCommand("showpf",             ShowPrintingFormatCommand               )==NULL) return (__LINE__);
  if (CreateCommand("setpf",                      SetPrintingFormatCommand                )==NULL) return (__LINE__);
  if (CreateCommand("createvector",   CreateVecDescCommand            )==NULL) return (__LINE__);
  if (CreateCommand("creatematrix",   CreateMatDescCommand            )==NULL) return (__LINE__);
  if (CreateCommand("freematrix",     FreeMatDescCommand              )==NULL) return (__LINE__);
  if (CreateCommand("symlist",            SymListCommand                      )==NULL) return (__LINE__);

  /* miscellaneous commands */
  if (CreateCommand("setkey",             SetCommandKeyCommand                    )==NULL) return (__LINE__);
  if (CreateCommand("delkey",             DeleteCommandKeyCommand                 )==NULL) return (__LINE__);
  if (CreateCommand("keylist",            ListCommandKeysCommand                  )==NULL) return (__LINE__);
  if (CreateCommand("refreshon",          RefreshOnCommand                                )==NULL) return (__LINE__);
  if (CreateCommand("refreshoff",         RefreshOffCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("machinetest",        MachineTestCommand                              )==NULL) return (__LINE__);
  if (CreateCommand("system",                     SystemCommand                                   )==NULL) return (__LINE__);
  if (CreateCommand("resetCEstat",        ResetCEstatCommand                              )==NULL) return (__LINE__);
  if (CreateCommand("printCEstat",        PrintCEstatCommand                              )==NULL) return (__LINE__);
  if (CreateCommand("heapstat",           HeapStatCommand                             )==NULL) return (__LINE__);
  if (CreateCommand("getheapused",        GetHeapUsedCommand                          )==NULL) return (__LINE__);

  /* commands for debugging */
        #ifdef Debug
  if (CreateCommand("debug",                      DebugCommand                                )==NULL) return (__LINE__);
  if (CreateCommand("trace",                      TraceCommand                                )==NULL) return (__LINE__);
  if (CreateCommand("reperr",             RepErrCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("timing",             TimingCommand                               )==NULL) return (__LINE__);
        #endif
  if (CreateCommand("showconfig",         ShowConfigCommand                           )==NULL) return (__LINE__);

#ifdef ModelP
  /* commands for parallel version */
  if (CreateCommand("lb",                         LBCommand                                               )==NULL) return (__LINE__);
  if (CreateCommand("ptest",                      LBSCommand                                      )==NULL) return (__LINE__);
  if (CreateCommand("lbs",                        LBSCommand                                      )==NULL) return (__LINE__);
  if (CreateCommand("context",            ContextCommand                              )==NULL) return (__LINE__);
  if (CreateCommand("pstat",                      PStatCommand                                )==NULL) return (__LINE__);
#ifdef CHACOT
  if (CreateCommand("lb4",                        LB4Command                                              )==NULL) return (__LINE__);
#endif
        #ifdef __DLB__
  if (CreateCommand("lbmm",           LBMMCommand                     )==NULL) return(__LINE__);
  if (CreateCommand("crcb",           CRCBCommand                     )==NULL) return(__LINE__);
  if (CreateCommand("dlb_config",     DLB_Config_Command                  )==NULL      ) return(__LINE__);
  if (CreateCommand("dlb_eval",           DLB_EvalCommand                             )==NULL) return (__LINE__);
        #endif

#ifdef USE_FAMG
  if (CreateCommand("pamgcheck",      pamgCheckCommand                )==NULL) return(__LINE__);
#endif

#endif /* ModelP */

  /* array commands */
  if (CreateCommand("crar",               CreateArrayCommand                                      )==NULL) return (__LINE__);
  if (CreateCommand("dear",               DeleteArrayCommand                                      )==NULL) return (__LINE__);
  if (CreateCommand("saar",               SaveArrayCommand                                        )==NULL) return (__LINE__);
  if (CreateCommand("loar",               LoadArrayCommand                                        )==NULL) return (__LINE__);
  if (CreateCommand("wrar",               WriteArrayCommand                                       )==NULL) return (__LINE__);
  if (CreateCommand("rear",               ReadArrayCommand                                        )==NULL) return (__LINE__);
  if (CreateCommand("clar",               ClearArrayCommand                                       )==NULL) return (__LINE__);

  if (CreateCommand("dumpalg",            DumpAlgCommand                                  )==NULL) return (__LINE__);

  if (InitClock()                 !=0) return (__LINE__);
  if (InitFindRange()     !=0) return (__LINE__);
  if (InitScreenSize()    !=0) return (__LINE__);
  if (InitArray()                 !=0) return (__LINE__);

  return(0);
}
