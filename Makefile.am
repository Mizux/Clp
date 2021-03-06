# Copyright (C) 2006 International Business Machines and others.
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.

## $Id$

# Author:  Andreas Waechter           IBM    2006-04-13

AUTOMAKE_OPTIONS = foreign

include BuildTools/Makemain.inc

########################################################################
#            Subdirectories and installation of .pc files              #
########################################################################

SUBDIRS = src

pkgconfiglibdir = $(libdir)/pkgconfig
pkgconfiglib_DATA = clp.pc

# Build OsiClp only if Osi is available.

if COIN_HAS_OSI
  SUBDIRS += src/OsiClp
  pkgconfiglib_DATA += osi-clp.pc
endif

# We don't want to compile the test subdirectory unless the test target
# is specified.  But we need to list it as subdirectory to make sure that
# it's included in the tarball.

if ALWAYS_FALSE
  SUBDIRS += test
endif

########################################################################
#             Additional files to be included in tarball               #
########################################################################

# Here we need include all files that are not mentioned in other Makefiles

EXTRA_DIST = \
	doc/authors.xml \
	doc/basicmodelclasses.xml \
	doc/clpexe.xml \
	doc/clpuserguide.xml \
	doc/doxygen.xml \
	doc/faqcontent.xml \
	doc/faq.xml \
	doc/intro.xml \
	doc/legal.xml \
	doc/messages.xml \
	doc/moresamples.xml \
	doc/notsobasic.xml \
	doc/revhist.xml \
	doc/Howto/docbook4clp.xml \
	examples/addBits.cpp \
	examples/addColumns.cpp \
	examples/addRows.cpp \
	examples/decompose.cpp \
	examples/defaults.cpp \
	examples/driver2.cpp \
	examples/driverC.c \
	examples/driver.cpp \
	examples/dualCuts.cpp \
	examples/ekk.cpp \
	examples/ekk_interface.cpp \
	examples/hello.cpp \
	examples/hello.mps \
	examples/input.130 \
	examples/INSTALL \
	examples/makeDual.cpp \
	examples/Makefile.in \
	examples/minimum.cpp \
	examples/network.cpp \
	examples/piece.cpp \
	examples/rowColumn.cpp \
	examples/sprint2.cpp \
	examples/sprint.cpp \
	examples/testBarrier.cpp \
	examples/testBasis.cpp \
	examples/testGub2.cpp \
	examples/testGub.cpp \
	examples/testQP.cpp \
	examples/useVolume.cpp

########################################################################
#                           Extra Targets                              #
########################################################################

test: all
	cd test; $(MAKE) test

unitTest: test

clean-local: clean-doxygen-docs
	if test -r test/Makefile; then cd test; $(MAKE) clean; fi

distclean-local:
	if test -r test/Makefile; then cd test; $(MAKE) distclean; fi

install-exec-local: install-doc

uninstall-local: uninstall-doc uninstall-doxygen-docs

.PHONY: test unitTest


########################################################################
#                         Maintainer Stuff                             #
########################################################################

CLEANFILES =

# Files that are generated and should be cleaned with make distclean

DISTCLEANFILES =

