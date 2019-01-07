#
# file:        $Source: /usr/adac/Repository/camera/camera/control/src/worklist/libmppsutils/Makefile,v $
#
# purpose:	Makefile for DICOM MPPS shared library routines
#
# revision history:
#	Jiantao Huang		Initial version
#
# inspection history:
#
#
BASEDIR=	..
CAMERA_BASEDIR=	../../../..
include $(CAMERA_BASEDIR)/buildsupport/make.vars

INSTALLDIR_INCLUDE=	$(BASEDIR)/include/mpps

TARGET_STATICLIB_CCC=	libmppsutils.a
TARGET_DLL_CCC=		libmppsutils$(DLL)

C++FILES=	mppsutils.cc

CFILES=		../libworklistutils/qr.c

HFILES=		mppsutils.h mppsexceptions.h

DEFINES+=	-D_POSIX_PTHREAD_SEMANTICS -DBIG_END
INCLUDES=	-I$(MERGEDICOMDIR)/mc3inc -I$(BINNERDIR)/include -I$(BASEDIR)/include

all:		$(TARGET_DLL_CCC) $(TARGET_STATICLIB_CCC)

install:	$(INSTALLDIR_LIB)/$(TARGET_DLL_CCC) \
		$(INSTALLDIR_LIB)/$(TARGET_STATICLIB_CCC)

include $(CAMERA_BASEDIR)/buildsupport/make.targets
