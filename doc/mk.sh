#! /bin/sh

ag=`command -v autogen`
test -x "$ag" || {
    touch $1
    exit 0
}

test -x ${top_builddir}/src/${exe} || {
    cd ${top_builddir}/src || exit 1
    ${MAKE} ${exe} || exit 1
    cd ${top_builddir}/doc
}

set -e
touch ${1}-TMP
$ag -T${texi_tpl} -b ${exe} ${optdefs}
$ag ${TXI_DEP} -Tagtexi-cmd -DLEVEL=chapter ${optdefs}
mv ${1}-TMP ${1}
