[= AutoGen5 template  -*- Mode: text -*-

texi=example.texi

##  Documentation template
##
## Author:            Bruce Korb <bkorb@gnu.org>
##
##  This file is part of Complexity.
##  Complexity Copyright (c) 2011, 2020 by Bruce Korb - all rights reserved
##
##  Complexity is free software: you can redistribute it and/or modify it
##  under the terms of the GNU General Public License as published by the
##  Free Software Foundation, either version 3 of the License, or
##  (at your option) any later version.
##
##  Complexity is distributed in the hope that it will be useful, but
##  WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
##  See the GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License along
##  with this program.  If not, see <http://www.gnu.org/licenses/>.

=][= (out-push-new "set-flags.texi") \=]
@ignore
[=(dne "")=]
@end ignore

@set CRIGHT     2011-[= (shell "date '+%Y'") =]
@set PKG-TITLE  [= package =] - [= prog-title =]
@set PROGRAM    [=prog-name=]
@set TITLE      [=prog-title=]
@set EMAIL      [=

(define texi-escape-encode (lambda (in-str)
   (string-substitute in-str
      '("@"   "{"   "}")
      '("@@"  "@{"  "@}")
)  ))

(texi-escape-encode "bkorb@gnu.org") =]
[= (out-pop) \=]
@page
@node    Example Output
@chapter Example Output
@cindex  Example Output

This is a self-referential example. This output was obtained by
going into the complexity source directory and running the command:
@example
[=`
src=\`cd ${top_srcdir}/src && pwd\`
bld=\`cd ${top_builddir}/src && pwd\`
cmd='complexity --histogram --score --thresh=3'
echo "$cmd *.c"
`=]
@end example

The @code{--threshold} is set to three because all of the functions
score below the default threshold of 30.  It is not zero because
there are too many trivial (0, 1 or 2) functions for a short example.

@noindent
This results in:

@example
[=`
cd ${src}
if test -f opts.c
then optsc=
else optsc=${bld}/opts.c
fi
eval ${bld}/${cmd} *.c $optsc | sed 's/\\(.\\{66\\}\\).*/\\1/'
`=]
@end example
