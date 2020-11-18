#! /bin/sh

build_exe() {
    test -x ${top_builddir}/src/${exe} || {
	cd ${top_builddir}/src || exit 1
	${MAKE} ${exe} || exit 1
	cd ${top_builddir}/doc
    }
}

build_texi() {
    ag=`command -v autogen`
    test -x "$ag" || {
	touch $1
	exit 0
    }

    example_texi=${top_srcdir}/doc/example-texi.tpl
    if $VERBOSE
    then
	test -z "$TMPDIR" && TMPDIR=${TEMP_DIR}
	test -z "$TMPDIR" && TMPDIR=/tmp/run-ag-$$
	test -d "$TMPDIR" || mkdir "$TMPDIR" || exit 1
	export AUTOGEN_TRACE=every
	export AUTOGEN_TRACE_OUT=">>${TMPDIR}/AG-man.txt"
    fi

    $ag -T$example_texi ${optdefs}
    $VERBOSE && \
	AUTOGEN_TRACE_OUT=">>${TMPDIR}/AG-info.txt"
    $ag ${TXI_DEP} -Tagtexi-cmd -DLEVEL=chapter ${optdefs}
}

build_info() {
    local write_perms=false
    test -w ${top_srcdir}/doc || {
	write_perms=true
	chmod u+w ${top_srcdir}/doc ${top_srcdir}/doc/*.*
    }

    title=`sed -n 's/^@title  *//p' complexity.texi`
    . ${top_srcdir}/build-aux/gendocs.sh --texi2html complexity "$title"
    $write_perms && \
	chmod u-w ${top_srcdir}/doc ${top_srcdir}/doc/*.*
}

case "$(set -o|grep xtrace)" in
    *on ) VERBOSE=true ;;
esac
if test "X$VERBOSE" = Xtrue
then
    set -x
    PS4='>DOX-${FUNCNAME:-=}=$LINENO> '
else
    VERBOSE=false
fi

top_builddir=`cd $top_builddir && pwd`
top_srcdir=`cd $top_srcdir && pwd`

set -e
case "$1" in
    example.texi )  build_texi  ;;
    gnudocs )       build_info  ;;
esac
