#
# file:		Makefile
# purpose:	build worklistserver
# created:	02-Oct-2003
# property of:	PHILIPS Nuclear Medicine, ADAC Laboratories
#
# inspection history:
#
# revision history:
#   02-Oct-2003	Jiantao Huang		Initial version
#   04-Feb-2004	Jiantao Huang		Change the installation base to worklist
#   24-Nov-2004 Jingwen "Gene" Wang Visibroker 6.0 and Forte 6.2 Compiler upgrades
#
# $Id: Makefile,v 1.18 2009/07/10 22:39:15 jhuang Exp $
#

BASEDIR=		..
CAMERA_BASEDIR=		../../../../
include $(CAMERA_BASEDIR)/buildsupport/make.vars

C++FILES=		\
			worklistserver.cc \
			worklistimpl.cc \
			worklistmanager.cc \
			manualworklist.cc \
			automatedworklist.cc \
			timer.cc

INCLUDES=		\
			-I$(MERGEDICOMDIR)/mc3inc \
			$(INCLUDES_ORB) \
			$(INCLUDES_RW) \
			-I$(BASEDIR)/include \
			-I$(CONTROLDIR)/include \
			-I$(BINNERDIR)/include \
			-Wno-deprecated

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
			$(LIBS_POSIX) -lworklistutils $(MERGE_LIBS)

DEFINES+=		$(CORBA_DEFINES)
TARGET_BINARY_CCC=	worklistserver$(EXE)

# Want worklistserver to be rebuilt when anything changes to update embedded build info
worklistserver.o:	DEFINES += $(BUILDERDEFINES)
worklistserver.o:	$(filter-out worklistserver.o,$(TARGET_OBJS_CCC))

all:		mc3adv.so $(TARGET_BINARY_CCC)

install:	all $(INSTALLDIR_BIN)/$(TARGET_BINARY_CCC)

mc3adv.so:
	@$(RM) mc3adv.so
	@$(LNS) $(MERGEDICOMDIR)/mc3lib/mc3adv.so .

clean:
	-$(RM) mc3adv.so

include $(CAMERA_BASEDIR)/buildsupport/make.targets
