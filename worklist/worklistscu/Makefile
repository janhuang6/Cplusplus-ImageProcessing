#
# file:		Makefile
# purpose:	build a test SCU
#
# inspection history:
#
# revision history:
#   Jiantao Huang		Initial version
#
# $Id: Makefile,v 1.12 Exp $
#

BASEDIR=		..
CAMERA_BASEDIR=		../../../..
include $(CAMERA_BASEDIR)/buildsupport/make.vars

C++FILES=		worklistscu.cc

INCLUDES=		\
			-I$(BASEDIR)/include/worklist \
			-I$(BINNERDIR)/include \
			-I$(MERGEDICOMDIR)/mc3inc

LIBPATH=		\
			-L$(BASEDIR)/lib \
			-L$(BINNERDIR)/lib \
			-L$(UTILSDIR)/lib

LIBS=			-lacqbase \
			$(LIBS_Z) $(LIBS_XERCES) $(LIBS_NETWORK) \
			$(LIBS_DLOAD) $(LIBS_THREAD) $(LIBS_POSIX) \
			$(LIBS_REALTIME) -lworklistutils $(MERGE_LIBS)


TARGET_BINARY_CCC=	worklistscu$(EXE)

all:		mc3adv.so $(TARGET_BINARY_CCC)

install:	all $(INSTALLDIR_BIN)/$(TARGET_BINARY_CCC)

mc3adv.so:
	@$(RM) mc3adv.so
	@$(LNS) $(MERGEDICOMDIR)/mc3lib/mc3adv.so .

clean:
	-$(RM) mc3adv.so

include $(CAMERA_BASEDIR)/buildsupport/make.targets
