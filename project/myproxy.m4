dnl AC_MYPROXY([ ])
dnl define MYPROXY_NOTHR_CFLAGS, MYPROXY_THR_CFLAGS, MYPROXY_NOTHR_LIBS and MYPROXY_THR_LIBS
dnl
AC_DEFUN(AC_MYPROXY,
[
	# Set MyProxy Properties
	AC_ARG_WITH(myproxy_prefix,
		[  --with-myproxy-prefix=PFX  prefix where MyProxy is installed. (/opt/globus)],
		[],
	        with_myproxy_prefix=${MYPROXY_PATH:-/opt/globus})

    AC_MSG_RESULT(["MyProxy prefix is $with_myproxy_prefix"])

    AC_ARG_WITH(myproxy_nothr_flavor,
	[  --with-myproxy-nothr-flavor=flavor [default=gcc32dbg]],
	[],
        with_myproxy_nothr_flavor=${MYPROXY_FLAVOR:-gcc32dbg})

    AC_MSG_RESULT(["MyProxy nothread flavor is $with_myproxy_nothr_flavor"])

    AC_ARG_WITH(myproxy_thr_flavor,
        [  --with-myproxy-thr-flavor=flavor [default=gcc32dbgpthr]],
        [],
        with_myproxy_thr_flavor=${MYPROXY_FLAVOR:-gcc32dbgpthr})

    AC_MSG_RESULT(["MyProxy thread flavor is $with_myproxy_thr_flavor"])

    MYPROXY_PATH="$with_myproxy_prefix"
	MYPROXY_NOTHR_CFLAGS="-I $with_myproxy_prefix/include/$with_myproxy_nothr_flavor"
    MYPROXY_THR_CFLAGS="-I $with_myproxy_prefix/include/$with_myproxy_thr_flavor"

	ac_myproxy_ldlib="-L$with_myproxy_prefix/lib"

    MYPROXY_NOTHR_LIBS="$ac_myproxy_ldlib -lmyproxy_$with_myproxy_nothr_flavor"
    MYPROXY_THR_LIBS="$ac_myproxy_ldlib -lmyproxy_$with_myproxy_thr_flavor"

	AC_SUBST(MYPROXY_PATH)
	AC_SUBST(MYPROXY_NOTHR_CFLAGS)
	AC_SUBST(MYPROXY_THR_CFLAGS)
    AC_SUBST(MYPROXY_NOTHR_LIBS)
	AC_SUBST(MYPROXY_THR_LIBS)
])
