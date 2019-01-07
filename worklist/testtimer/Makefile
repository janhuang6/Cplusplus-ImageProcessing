#
# file:		Makefile
# purpose:	build testtimer
#
# inspection history:
#
# revision history:
#   Jiantao Huang		Initial version
#
# $Id: Makefile,v 1.6 Exp $
#

BASEDIR=		..
CAMERA_BASEDIR=		../../../..
include $(CAMERA_BASEDIR)/buildsupport/make.vars

C++FILES=		testtimer.cc \
			../worklistserver/timer.cc

INCLUDES=		-I../worklistserver -I$(BINNERDIR)/include
LIBPATH=		-L$(BINNERDIR)/lib
LIBS=			-lacqbase $(LIBS_REALTIME) \
			$(LIBS_NETWORK) $(LIBS_DLOAD) $(LIBS_POSIX)

TARGET_BINARY_CCC=	testtimer$(EXE)

all:		$(TARGET_BINARY_CCC)

install:	$(INSTALLDIR_BIN)/$(TARGET_BINARY_CCC)

include $(CAMERA_BASEDIR)/buildsupport/make.targets
