AC_DEFUN(AC_OPTIMIZE,
[
    AC_MSG_CHECKING([for OPTIMIZATION parameters])

    AC_ARG_ENABLE(optimization,
	[  --enable-optimization=<option> Default is yes],
	optimizopt="$enableval", 
	optimizopt="yes"
    )
  
    if test "x$optimizopt" = "xyes" ; then
        AC_MSG_RESULT([$optimizopt])
    else
        AC_MSG_RESULT(no)
    fi
  
    if test "x$optimizopt" = "xyes" ; then
        CXXFLAGS="-g -Wall -O2 -mcpu=i486"
        CFLAGS="-g -Wall -O2 -mcpu=i486"
    else
        CXXFLAGS="-g -Wall -O0 -mcpu=i486"
        CFLAGS="-g -Wall -O0 -mcpu=i486"
    fi

    AC_SUBST(CXXFLAGS)
    AC_SUBST(CFLAGS)
])

