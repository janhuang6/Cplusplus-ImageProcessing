#
# file:		Makefile
# purpose:	build mpps
#
# inspection history:
#
# revision history:
#   Jiantao Huang		Initial version
#
# $Id: Makefile,v 1.9 Exp $
#

BASEDIR=		..
CAMERA_BASEDIR=		../../../../
include $(CAMERA_BASEDIR)/buildsupport/make.vars

C++FILES=		\
			mppsscu.cc \
			mppsimpl.cc \
			mppsmanager.cc

INCLUDES=		\
			-I$(MERGEDICOMDIR)/mc3inc \
			$(INCLUDES_ORB) \
			$(INCLUDES_RW) \
			-I$(BASEDIR)/include \
			-I$(CONTROLDIR)/include \
			-I$(BINNERDIR)/include

LIBPATH=		\
			-L$(CONTROLDIR)/lib \
			-L$(BINNERDIR)/lib \
			-L$(BASEDIR)/lib \
			-L$(UTILSDIR)/lib

LIBS=			 -lasf -lacqserver -lacqbase \
			-lxmlio -lpngio $(LIBS_PNG) $(LIBS_Z) \
			$(LIBS_LICENSING) \
			$(LIBS_XERCES) $(LIBS_ORB) $(LIBS_RW) \
			$(LIBS_REALTIME) \
			$(LIBS_NETWORK) $(LIBS_DLOAD) $(LIBS_THREAD) \
			$(LIBS_POSIX) -lmppsutils $(MERGE_LIBS)

DEFINES+=		$(CORBA_DEFINES) -Wno-deprecated
TARGET_BINARY_CCC=	mpps$(EXE)

# Want mpps to be rebuilt when anything changes to update embedded build info
mppsscu.o:	DEFINES += $(BUILDERDEFINES)
mppsscu.o:	$(filter-out mppsscu.o,$(TARGET_OBJS_CCC))

all:		mc3adv.so $(TARGET_BINARY_CCC)

install:	all $(INSTALLDIR_BIN)/$(TARGET_BINARY_CCC)

mc3adv.so:
	@$(RM) mc3adv.so
	@$(LNS) $(MERGEDICOMDIR)/mc3lib/mc3adv.so .

clean:
	-$(RM) mc3adv.so

include $(CAMERA_BASEDIR)/buildsupport/make.targets
