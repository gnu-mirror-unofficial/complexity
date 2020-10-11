
## Makefile.am -- process this file with automake to produce Makefile.in
##
##  This file is part of Complexity.
##  Complexity Copyright (c) 2011, 2014 by Bruce Korb - all rights reserved
##
## Complexity is free software: you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by the
## Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## Complexity is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
## See the GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along
## with this program.  If not, see <http://www.gnu.org/licenses/>.

exe                 = complexity
man_MANS            = $(exe).1
optdefs             = $(top_srcdir)/src/opts.def
texi_tpl            = $(srcdir)/$(exe)-tpl.texi

if AMDEP
TXI_DEP_FILE        = $(DEPDIR)/info-dep
TXI_DEP             = -MF$(TXI_DEP_FILE) -MT$@
$(TXI_DEP_FILE)     : texi-stamp

MAN_DEP_FILE        = $(DEPDIR)/man-dep
MAN_DEP             = -MF$(MAN_DEP_FILE)  -MT$@
$(MAN_DEP_FILE)     : $(man_MANS)

include $(DEPDIR)/info-dep
include $(DEPDIR)/man-dep
DEP_FILES           = $(DEPDIR)/info-dep $(DEPDIR)/man-dep
$(DEP_FILES)        : $(DEPDIR)
$(DEPDIR)           :
	mkdir $@

else
TXI_DEP             =
MAN_DEP             =
DEP_FILES           =
DEPDIR              =
endif

CLEANFILES          = *-stamp $(DEP_FILES) invoke-complexity.*
TEXI2DVI_FLAGS      = --texinfo='@pagesizes 9.5in,7.0in'
info_TEXINFOS       = $(exe).texi
EXTRA_DIST          = $(exe)-tpl.texi fdl.texi intro.texi compute.texi \
	gendocs_template invoke-$(exe).texi texi-stamp mk.sh
DISTCLEANFILES      = $(BUILT_SOURCES) $(man_MANS)

$(man_MANS)         : $(optdefs) $(DEPDIR)
	autogen $(MAN_DEP) -Tagman-cmd $(optdefs)

$(exe).texi         : texi-stamp
$(top_builddir)/src/complexity :

texi-stamp          : $(optdefs) $(DEPDIR)
	exe="$(exe)" texi_tpl="$(texi_tpl)" optdefs="$(optdefs)" \
	TXI_DEP="$(TXI_DEP)" \
	top_srcdir="$(top_srcdir)" \
	top_builddir="$(top_builddir)" \
	MAKE="$(MAKE)" $(SHELL) $(srcdir)/mk.sh $@

gnudocs             : $(srcdir)/gendocs_template texi-stamp
	title=`sed -n 's/^@title  *//p' $(exe).texi` ; \
	opts='--texi2html' ; \
	$(SHELL) $(top_srcdir)/build-aux/gendocs.sh \
	    $$opts complexity "$$title"
