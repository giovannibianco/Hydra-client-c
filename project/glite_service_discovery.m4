dnl Usage:
dnl AC_GLITE_SERVICE_DISCOVERY
dnl - GLITE_SERVICE_DISCOVERY_API_C_LIBS

AC_DEFUN(AC_GLITE_SERVICE_DISCOVERY,
[
    ac_glite_sd_prefix=$GLITE_LOCATION

    if test -n "ac_glite_sd_prefix" ; then
	dnl
	dnl 
	dnl
        ac_glite_sd_lib="-L$ac_glite_sd_prefix/lib"
	GLITE_SERVICE_DISCOVERY_API_C_LIBS="$ac_glite_sd_lib -lglite_sd_c"
	ifelse([$2], , :, [$2])
    else
	GLITE_SERVICE_DISCOVERY_API_C_LIBS=""
	ifelse([$3], , :, [$3])
    fi

    AC_SUBST(GLITE_SERVICE_DISCOVERY_API_C_LIBS)
])

