#****************************************************************************#
#*																			*#
#* File:	  ui.make														*#
#*																			*#
#* Purpose:   build ug user interface										*#
#*																			*#
#* Author:	  Peter Bastian 												*#
#*			  Interdisziplinaeres Zentrum fuer Wissenschaftliches Rechnen	*#
#*			  Universitaet Heidelberg										*#
#*			  Im Neuenheimer Feld 368										*#
#*			  6900 Heidelberg												*#
#*			  internet: ug@ica3.uni-stuttgart.de					*#
#*																			*#
#* History:   24.05.92 begin, ug version 2.0								*#
#*			  13.08.92 update for ug 2.0.2									*#
#*			  05 Sep 92 update for ug 2.1(.0)								*#
#*			  05 Sep 94 update for ug 2.3									*#
#*			  01 Jan 95 update for ug 3.0									*#
#*																			*#
#* Remarks:   MPW 3.2 version												*#
#*																			*#
#****************************************************************************#

# list of source files
OBJECTS = initui.c.o helpmsg.c.o cmdline.c.o cmdint.c.o uginterface.c.o commands.c.o

# local C compiler flags
LCFLAGS = -i "::low" -i "::dev" -i "::gm" -i "::numerics" -i "::graph"

# the main rule
all � {OBJECTS} ui.make
	Lib -o "::lib:libui{LIBSUFFIX}.a" {OBJECTS}

# compile all source files
initui.c.o �  initui.c
	 C {COPTS} {LCFLAGS} {UGDEFINES} initui.c

# compile all source files
helpmsg.c.o �  helpmsg.c
	 C {COPTS} {LCFLAGS} {UGDEFINES} helpmsg.c

# compile all source files
cmdline.c.o �  cmdline.c
	 C {COPTS} {LCFLAGS} {UGDEFINES} cmdline.c

# compile all source files
cmdint.c.o �  cmdint.c
	 C {COPTS} {LCFLAGS} {UGDEFINES} cmdint.c

# compile all source files
uginterface.c.o �  uginterface.c
	 C {COPTS} {LCFLAGS} {UGDEFINES} uginterface.c

# compile all source files
commands.c.o �	commands.c
	 C {COPTS} {LCFLAGS} {UGDEFINES} commands.c

# clean up
clean �
	Set Exit 0
	delete -i �.o
