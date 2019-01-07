#
# file:		Makefile
# purpose:	main Makefile for worklist software
# reference:	Software Development Plan, Document #xxxx-xxxx-Rev. x.
#
# revision history:
#   Jiantao Huang		Initial version
#

#
# $Header: /usr/adac/Repository/camera/camera/control/src/worklist/Makefile,v 1.19 Exp $
#

#
# All include (.h) files are contained within subdirectories.
# Those include files which must be shared with other
# subdirectories are copied (by their Makefiles) into the
# include/worklist directory.  The include/worklist directory
# (actually (../include/worklist) can be removed, and a
# make install will repopulate it.  Therefore, the order in
# which make runs through these subdirectories is important.
# Specifically, lib should always be done early on.
#

BASEDIR=	./
CAMERA_BASEDIR=	../../..
include $(CAMERA_BASEDIR)/buildsupport/make.toplevel.vars

SUBDIRECTORIES=	libworklistutils testtimer worklistscu worklistserver \
		work_scus libmppsutils mppsscu 

PWDDIR=		$(shell pwd)
PKGDIR=		pkg
PKGNAME=	ADACjcwl
PKGDESC=	Worklist software
PKGTAG=		$$Name: Atlantis710R01_branch $$
PKGDEP=		ADACserv ADACutil ADACJetC
PKGPROVIDES=	
RELEASEDIRS=	./bin ./etc/ris.sed ./lib ./DicomJetConnect
PKGSKIP+=	testtimer worklistscu

update:	install
	@echo Done. Worklist Server and MPPS SCU software are ready to test.

all:	install
	@echo Done. Worklist Server and MPPS SCU software are ready to test.

release:	clean all
	@echo Done.  Worklist Server and MPPS SCU software have been built.

_package:
	($(CD) $(PKGDIR); $(CAT) infofile > prototype)
	(pkgproto -c appdir $(RELEASEDIRS) | $(AWK) '{printf("%s %s %s %s atlas users\n", $$1, $$2, $$3, $$4)}' >> $(PKGDIR)/prototype)
	($(CD) $(PKGDIR); $(SED) $(PKGSKIP_SED) prototype > prototype.tmp; cp -p prototype.tmp prototype; pkgmk -d $(PKGTMPDIR) -o -b $(BASEDIR))
	pkgtrans -o -s $(PKGTMPDIR) $(PWDDIR)/$(PKGDIR)/$(PKGNAME).pkg $(PKGNAME)
	$(RMDIR) $(PKGTMPDIR)/$(PKGNAME)
	$(RM) $(PKGDIR)/prototype.tmp
	@echo Done.  Worklist Server software has been built and packaged.

distclean:
	$(RM) $(PKGDIR)/$(PKGNAME).pkg

include $(CAMERA_BASEDIR)/buildsupport/make.targets
