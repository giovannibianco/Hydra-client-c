dnl Usage:
dnl AC_GLITE_WMSUTILS
dnl - GLITE_WMSUTILS_EXCEPTION_LIBS
dnl -
dnl - GLITE_WMSUTILS_CJOBID_LIBS
dnl - GLITE_WMSUTILS_JOBID_LIBS
dnl -
dnl - GLITE_WMSUTILS_TLS_CFLAGS
dnl - GLITE_WMSUTILS_TLS_SOCKET_LIBS
dnl - GLITE_WMSUTILS_TLS_GSI_SOCKET_LIBS
dnl - GLITE_WMSUTILS_TLS_GSISOCKET_LIBS

AC_DEFUN(AC_GLITE_WMSUTILS,
[
    ac_glite_wmsutils_prefix=$GLITE_LOCATION

    if test -n "ac_glite_wmsutils_prefix" ; then
	dnl
	dnl 
	dnl
        ac_glite_wmsutils_lib="-L$ac_glite_wmsutils_prefix/lib"
    fi

    result=no

    if test -n "ac_glite_wmsutils_lib" ; then
	GLITE_WMSUTILS_EXCEPTION_LIBS="$ac_glite_wmsutils_lib -lglite_wmsutils_exception"

	GLITE_WMSUTILS_CJOBID_LIBS="$ac_glite_wmsutils_lib -lglite_wmsutils_cjobid"
        GLITE_WMSUTILS_JOBID_LIBS="$ac_glite_wmsutils_lib -lglite_wmsutils_jobid"

	GLITE_WMSUTILS_TLS_SOCKET_LIBS="$ac_glite_wmsutils_lib -lglite_wmsutils_tls_socket_pp"
	GLITE_WMSUTILS_TLS_GSI_SOCKET_LIBS="$ac_glite_wmsutils_lib -lglite_wmsutils_tls_gsisocket_pp"
	GLITE_WMSUTILS_TLS_GSISOCKET_LIBS="$GLITE_WMSUTILS_TLS_SOCKET_LIBS -lglite_wmsutils_tls_gsisocket_pp"
        result=yes
    fi

dnl
dnl test
dnl

    if test x$result == xyes ; then
	ifelse([$2], , :, [$2])
    else
	GLITE_WMSUTILS_EXCEPTION_LIBS=""

	GLITE_WMSUTILS_CJOBID_LIBS=""
	GLITE_WMSUTILS_JOBID_LIBS=""

	GLITE_WMSUTILS_TLS_SOCKET_LIBS=""
	GLITE_WMSUTILS_TLS_GSI_SOCKET_LIBS=""
	GLITE_WMSUTILS_TLS_GSISOCKET_LIBS=""
	ifelse([$3], , :, [$3])
    fi

    AC_SUBST(GLITE_WMSUTILS_EXCEPTION_LIBS)

    AC_SUBST(GLITE_WMSUTILS_CJOBID_LIBS)
    AC_SUBST(GLITE_WMSUTILS_JOBID_LIBS)

    AC_SUBST(GLITE_WMSUTILS_TLS_SOCKET_LIBS)
    AC_SUBST(GLITE_WMSUTILS_TLS_GSI_SOCKET_LIBS)
    AC_SUBST(GLITE_WMSUTILS_TLS_GSISOCKET_LIBS)
])

