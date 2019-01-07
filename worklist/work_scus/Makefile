#
# file:		Makefile
# purpose:	build a test SCU
#
# inspection history:
#
# revision history:
#   Jiantao Huang		Initial version
#
# $Id: Makefile,v 1.2 Exp $
#

BASEDIR=		..
CAMERA_BASEDIR=		../../../..
include $(CAMERA_BASEDIR)/buildsupport/make.vars

CFILES=		work_scu.c qr_util.c

CFLAGS+=	-w

INCLUDES=		-I$(MERGEDICOMDIR)/mc3inc

LIBPATH=

LIBS=			$(LIBS_NETWORK) \
			$(LIBS_DLOAD) $(LIBS_THREAD) $(LIBS_POSIX) \
			$(LIBS_REALTIME) $(MERGE_LIBS)


TARGET_BINARY_CCC=	work_scus$(EXE)

all:		mc3adv.so $(TARGET_BINARY_CCC)

install:	all $(INSTALLDIR_BIN)/$(TARGET_BINARY_CCC) $(INSTALLDIR_BIN)/workreqs

mc3adv.so:
	@$(RM) mc3adv.so
	@$(LNS) $(MERGEDICOMDIR)/mc3lib/mc3adv.so .

clean:
	-$(RM) mc3adv.so

include $(CAMERA_BASEDIR)/buildsupport/make.targets
