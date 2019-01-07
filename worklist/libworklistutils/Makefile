#
# file:        $Source: /usr/adac/Repository/camera/camera/control/src/worklist/libworklistutils/Makefile,v $
#
# purpose:	Makefile for DICOM worklist shared library routines
#
# revision history:
#	Jiantao Huang		Initial version
#
# inspection history:
#
BASEDIR=	..
CAMERA_BASEDIR=	../../../..
include $(CAMERA_BASEDIR)/buildsupport/make.vars

INSTALLDIR_INCLUDE=	$(BASEDIR)/include/worklist

TARGET_STATICLIB_CCC=	libworklistutils.a
TARGET_DLL_CCC=		libworklistutils$(DLL)

C++FILES=	worklistutils.cc

CFILES=		qr.c

HFILES=		worklistutils.h \
		qr.h

DEFINES+=	-D_POSIX_PTHREAD_SEMANTICS -DBIG_END
INCLUDES=	-I$(MERGEDICOMDIR)/mc3inc -I$(BINNERDIR)/include

all:		$(TARGET_DLL_CCC) $(TARGET_STATICLIB_CCC)

install:	$(INSTALLDIR_LIB)/$(TARGET_DLL_CCC) \
		$(INSTALLDIR_LIB)/$(TARGET_STATICLIB_CCC)

include $(CAMERA_BASEDIR)/buildsupport/make.targets
