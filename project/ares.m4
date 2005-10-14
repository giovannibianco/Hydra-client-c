dnl Usage:
dnl AC_ARES(MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for ares, and defines
dnl - ARES_CFLAGS (compiler flags)
dnl - ARES_LIBS (linker flags, stripping and path)
dnl - ARES_INSTALL_PATH
dnl prerequisites:

AC_DEFUN(AC_ARES,
[
    AC_ARG_WITH(ares_prefix,
	[  --with-ares-prefix=PFX       prefix where 'ares' is installed.],
	[], 
        with_ares_prefix=${ARES_INSTALL_PATH:-/usr})

    AC_MSG_CHECKING([for ARES installation at ${with_ares_prefix}])

    ac_save_CFLAGS=$CFLAGS
    ac_save_LIBS=$LIBS
    if test -n "$with_ares_prefix" -a "$with_ares_prefix" != "/usr" ; then
	ARES_CFLAGS="-I$with_ares_prefix/include"
	ARES_LIBS="-L$with_ares_prefix/lib"
    else
	ARES_CFLAGS=""
	ARES_LIBS=""
    fi

    ARES_LIBS="$ARES_LIBS -lares"
    CFLAGS="$ARES_CFLAGS $CFLAGS"
    LIBS="$ARES_LIBS $LIBS"
    AC_TRY_COMPILE([ #include "ares.h" ],
    		   [ ],
    		   [ ac_cv_ares_valid=yes ], [ ac_cv_ares_valid=no ])
    CFLAGS=$ac_save_CFLAGS
    LIBS=$ac_save_LIBS
    AC_MSG_RESULT([$ac_cv_ares_valid])

    if test x$ac_cv_ares_valid = xyes ; then
        ARES_INSTALL_PATH=$with_ares_prefix
	ifelse([$2], , :, [$2])
    else
	ARES_CFLAGS=""
	ARES_LIBS=""
	ifelse([$3], , :, [$3])
    fi

    AC_SUBST(ARES_INSTALL_PATH)
    AC_SUBST(ARES_CFLAGS)
    AC_SUBST(ARES_LIBS)
])

