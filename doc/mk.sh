#! /bin/sh

if test "X$VERBOSE" = Xtrue
then
    exec 2> /tmp/MK-SH.txt
    set -x
    PS4='+DOC-${FUNCNAME:-=}=$LINENO> '
else
    VERBOSE=false
fi

ag=`command -v autogen`
test -x "$ag" || {
    touch $1
    exit 0
}
top_builddir=`cd $top_builddir && pwd`
top_srcdir=`cd $top_srcdir && pwd`

test -x ${top_builddir}/src/${exe} || {
    cd ${top_builddir}/src || exit 1
    ${MAKE} ${exe} || exit 1
    cd ${top_builddir}/doc
}

set -e
touch ${1}-TMP

if $VERBOSE
then
    export AUTOGEN_TRACE=every
    export AUTOGEN_TRACE_OUT=/tmp/AG-man.txt
    $ag -T${texi_tpl} -b ${exe} ${optdefs}
    AUTOGEN_TRACE_OUT=/tmp/AG-info.txt
    $ag ${TXI_DEP} -Tagtexi-cmd -DLEVEL=chapter ${optdefs}
else
    $ag -T${texi_tpl} -b ${exe} ${optdefs}
    $ag ${TXI_DEP} -Tagtexi-cmd -DLEVEL=chapter ${optdefs}
fi

mv ${1}-TMP ${1}
