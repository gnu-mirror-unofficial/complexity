#! /bin/bash

## This file is part of Complexity.
## Complexity Copyright (c) 2011-2016 by Bruce Korb - all rights reserved
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

usage()
{
    test $# -gt 0 && {
        exec >&2
        echo "${prog} usage error:  $*"
    }
    cat <<- _EOF_
	USAGE:  ${prog} [--percent N] [--keep D] \
	        { file-list-file | file-1 file-2 ... }
	WHERE:  'N' indicates by what percent of the function count the results
	must differ for a result to be printed.  '0' prints everything, and
	'100' prints nothing.  The default is '25'.
	AND:    'D' is a directory for keeping the complexity/pmccabe output.
	The files will be named:  $currentdir.{complexity,pmccabe}

	Both pmccabe and complexity will be run against the list of files.
	The pmccabe results will be numerically sorted.  Each function will
	be scored as its position within the output.  Functions that appear
	in substantially different places in the output will be printed out.
	The results are numerically sorted so that the largest ranking
	difference will be listed last.

	Functions that appear in one output but not the other will be printed
	to stderr and appear first.
	_EOF_

    exit $#
}

run_tools()
{
    local sedcmd=$'s/^[ \t0-9]*[ \t]//;s/([0-9][0-9]*):/:/p'

    test ${#keepdir} -gt 0 || keepdir=${tmpdir}

    complexity --thresh=0 --no-header $files > ${keepdir}/${base}.complexity
    {
        pmccabe -v /dev/null
        pmccabe $files | sort -n -k1,1 -k3,3 -k5,5
    } > ${keepdir}/${base}.pmccabe

    sed -n "$sedcmd" ${keepdir}/${base}.complexity > ${tmpdir}/${base}.complex
    sed -n "$sedcmd" ${keepdir}/${base}.pmccabe    > ${tmpdir}/${base}.mccabe
}

compare_results()
{
    local proc_ct=$(wc -l < ${tmpdir}/${base}.complex)
    local large=$(( (proc_ct * percent) / 100  ))
    local mc_line= cx_line= delta=

    exec 3< ${tmpdir}/${base}.mccabe
    while read -u 3 line
    do
        egrep "^${line}\$" ${tmpdir}/${base}.complex || \
            printf 'missing from complexity:  %s\n' "$line" >&2
    done > /dev/null

    exec 3< ${tmpdir}/${base}.complex
    cx_line=0
    while read -u3 line
    do
        (( cx_line += 1 ))
        mc_line=$(egrep -n "^${line}\$" ${tmpdir}/${base}.mccabe)
        test ${#mc_line} -le 1 && {
            printf 'missing from pmccabe:  %s\n' "$line" >&2
            continue
        }

        mc_line=${mc_line%%:*}
        delta=$(( mc_line - cx_line ))
        (( delta < 0 )) && (( delta = 0 - delta ))
        (( delta < large )) && continue

        line=${line##*/}
        printf '%5s %7s %7s %s\n' \
            $(( cx_line - mc_line )) $mc_line $cx_line "$line"
    done | {
        out=$(sort -n)
        printf 'delta mc-rank cx-rank procs w/ rankings differ over %4s\n' $large
        printf '===== ======= ======= ==================================\n'
        echo "$out"
    }
}

init()
{
    percent=25
    keepdir=''
    local do_opts=true opt=''
    files=''

    while :
    do
        opt=${1}
        case "$opt" in
        ( -[h?] | --h* ) usage ;;

        ( --per*=* ) percent=${1##*=} ;;
        ( --per*   ) shift
                     percent=$1
                     ;;

        ( --keep=* ) keepdir=${1##*=} ;;
        ( --keep   ) shift
                     keepdir=$1
                     ;;

        ( - )        files=$(egrep -v $'^[ \t]*(#|\$)') # read from stdin
                     ;;

        ( '' )       test $# -eq 0 && test ${#files} -le 1 && \
                         usage "no files specified"
                     do_opts=false
                     ;;

        ( -- )       shift ;&
        ( * )        do_opt=false ;;
        esac

        $do_opts || break;
        shift    || usage "no argument for $opt option"
    done

    # if all arguments have been consumed, then the empty argument was
    # seen and it was verified that the file list was obtained from stdin.
    #
    if test $# -gt 0
    then
        test ${#files} -gt 1 && \
            usage "input files read from stdin and provided on command line"

        # If there is more than one argument, they must be a list of files.
        # Otherwise, see if it is a file of file names or an input file.
        #
        if test $# -gt 1
        then files="$*"
        else
            files=$(egrep -v $'^[ \t]*($|#)' $1)
            test ${#files} -le 1 && files=$1
            for f in ${files}
            do
                test -f "$f" || files=$1
                break
            done
        fi
    fi

    tmpdir=$(mktemp -d ${TMPDIR:-/tmp}/${prog}-XXXXXX)
    test -d "$tmpdir" || \
        die "cannot make tmpdir of ${TMPDIR:-/tmp}/${prog}-XXXXXX"
    trap "rm -rf ${tmpdir}" 0
    base=$(basename $PWD)
}

init ${1+"$@"}
run_tools
compare_results
