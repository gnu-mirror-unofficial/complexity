##  -*- Mode: Makefile -*-
##
## Makefile.am -- process this file with automake to produce Makefile.in
##
##  This file is part of Complexity.
##  Complexity Copyright (c) 1992-2011, 2014 by Bruce Korb - all rights reserved
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

bin_PROGRAMS        = complexity
bin_SCRIPTS         = cx-vs-mc
option_def          = opts.def
option_src          = $(option_def:.def=.c) $(option_def:.def=.h)
charmap_map         = char-types.map
charmap_src         = $(charmap_map:.map=.h)
gnulib              = $(top_builddir)/lib/libgnu.a

complexity_SOURCES  = \
	complexity.h complexity.c score.c tokenize.c \
	$(charmap_src)
nodist_complexity_SOURCES = $(option_src)
BUILT_SOURCE        = $(option_src)
$(complexity_OBJECTS) : $(BUILT_SOURCE)

complexity_CFLAGS   = $(ao_CFLAGS)
complexity_LDADD    = $(ao_LIBS) $(gnulib) -lm

CLEANFILES          = *-stamp $(DEP_FILES) $(bin_SCRIPTS)
EXTRA_DIST          = $(option_def) $(charmap_map) cx-vs-mc.sh
DISTCLEANFILES      = $(option_src)

if AMDEP
if  AG_MF
    OPT_DEP         = -MF$(DEPDIR)/opts-dep -MT$@
    DEP_FILES       = $(DEPDIR)/opts-dep
-include $(DEPDIR)/opts-dep

else
    OPT_DEP         =
endif
else
  OPT_DEP           =
endif

$(option_src)       : option-stamp
option-stamp        : $(srcdir)/$(option_def)
	top_srcdir=$(top_srcdir) \
	    autogen $(OPT_DEP) $(srcdir)/$(option_def)

$(charmap_src)      : $(charmap_map)
	char-mapper $(srcdir)/$(charmap_map)

cx-vs-mc            : cx-vs-mc.sh
	@-rm -f $@
	cp $(srcdir)/cx-vs-mc.sh $@ && chmod 555 $@

# end of Makefile.am
