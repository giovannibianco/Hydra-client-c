dnl Usage:
dnl AC_GLITE_SECURITY
dnl - GLITE_SECURITY_RENEWAL_THR_LIBS
dnl - GLITE_SECURITY_RENEWAL_NOTHR_LIBS

dnl - GLITE_SECURITY_VOMS_LIBS 
dnl - GLITE_SECURITY_VOMS_THR_LIBS
dnl - GLITE_SECURITY_VOMS_NOTHR_LIBS
dnl - GLITE_SECURITY_VOMS_CPP_LIBS
dnl - GLITE_SECURITY_VOMS_CPP_THR_LIBS
dnl - GLITE_SECURITY_VOMS_CPP_NOTHR_LIBS
dnl - GLITE_SECURITY_VOMS_STATIC_LIBS
dnl - GLITE_SECURITY_VOMS_STATIC_THR_LIBS
dnl - GLITE_SECURITY_VOMS_STATIC_NOTHR_LIBS

dnl - SEC_LCAS_LIBS (linker flags, stripping and path for static library)

dnl - SEC_LCMAPS_LIBS (linker flags, stripping and path for static library)
dnl - SEC_LCMAPS_WITHOUT_GSI_LIBS
dnl - SEC_LCMAPS_RETURN_WITHOUT_GSI_LIBS

dnl - SEC_GSOAP_PLUGIN_STATIC_THR_LIBS
dnl - SEC_GSOAP_PLUGIN_STATIC_NOTHR_LIBS
dnl - SEC_GSOAP_PLUGIN_GSS_THR_LIBS
dnl - SEC_GSOAP_PLUGIN_GSS_NOTHR_LIBS

AC_DEFUN(AC_GLITE_SECURITY,
[
    ac_glite_security_prefix=$GLITE_LOCATION

    have_voms=no
    AC_VOMS([],have_voms=yes,have_voms=no)

    have_proxyrenewal=no
    AC_PROXYRENEWAL([],have_proxyrenewal=yes,have_proxyrenewal=no)

    if test "x$have_voms" = "xyes" -a "x$have_proxyrenewal" = "xyes"; then
        GLITE_SECURITY_VOMS_STATIC_LIBS=$VOMS_STATIC_LIBS
        GLITE_SECURITY_VOMS_LIBS=$VOMS_LIBS
        GLITE_SECURITY_VOMS_CPP_LIBS=$VOMS_CPP_LIBS
        GLITE_SECURITY_VOMS_THR_LIBS=$VOMS_THR_LIBS
        GLITE_SECURITY_VOMS_CPP_THR_LIBS=$VOMS_CPP_THR_LIBS
        GLITE_SECURITY_VOMS_NOTHR_LIBS=$VOMS_NOTHR_LIBS
        GLITE_SECURITY_VOMS_CPP_NOTHR_LIBS=$VOMS_CPP_NOTHR_LIBS
        GLITE_SECURITY_VOMS_STATIC_THR_LIBS=$VOMS_STATIC_THR_LIBS
        GLITE_SECURITY_VOMS_STATIC_NOTHR_LIBS=$VOMS_STATIC_NOTHR_LIBS

        GLITE_SECURITY_RENEWAL_THR_LIBS=$RENEWAL_THR_LIBS
        GLITE_SECURITY_RENEWAL_NOTHR_LIBS=$RENEWAL_NOTHR_LIBS
        ifelse([$2], , :, [$2])
    else
        GLITE_SECURITY_VOMS_LIBS=""
        GLITE_SECURITY_VOMS_CPP_LIBS=""
        GLITE_SECURITY_VOMS_STATIC_LIBS=""

        GLITE_SECURITY_RENEWAL_THR_LIBS=""
        GLITE_SECURITY_RENEWAL_NOTHR_LIBS=""
        ifelse([$3], , :, [$3])
    fi

    AC_SUBST(GLITE_SECURITY_VOMS_STATIC_LIBS)
    AC_SUBST(GLITE_SECURITY_VOMS_LIBS)
    AC_SUBST(GLITE_SECURITY_VOMS_CPP_LIBS)
    AC_SUBST(GLITE_SECURITY_VOMS_STATIC_THR_LIBS)
    AC_SUBST(GLITE_SECURITY_VOMS_STATIC_NOTHR_LIBS)
    AC_SUBST(GLITE_SECURITY_VOMS_CPP_THR_LIBS)
    AC_SUBST(GLITE_SECURITY_VOMS_CPP_NOTHR_LIBS)
    AC_SUBST(GLITE_SECURITY_VOMS_THR_LIBS)
    AC_SUBST(GLITE_SECURITY_VOMS_NOTHR_LIBS)
    
    AC_SUBST(GLITE_SECURITY_RENEWAL_THR_LIBS)
    AC_SUBST(GLITE_SECURITY_RENEWAL_NOTHR_LIBS)
])

AC_DEFUN(AC_PROXYRENEWAL,
[ 
  with_proxyrenewal_prefix=$GLITE_LOCATION

  AC_MSG_CHECKING([for PROXYRENEWAL installation at ${with_proxyrenewal_prefix}])
    
  ac_save_CFLAGS=$CFLAGS
  ac_save_CPPFLAGS=$CPPFLAGS
  ac_save_LIBS=$LIBS
  if test -n "$with_proxyrenewal_prefix" ; then
     RENEWAL_CFLAGS="-I$with_proxyrenewal_prefix/include"
     RENEWAL_LIBS="-L$with_proxyrenewal_prefix/lib"
  else
     RENEWAL_CFLAGS=""
     RENEWAL_LIBS=""
  fi

  RENEWAL_THR_LIBS="$RENEWAL_LIBS -lglite_security_proxyrenewal_$GLOBUS_THR_FLAVOR"
  RENEWAL_NOTHR_LIBS="$RENEWAL_LIBS -lglite_security_proxyrenewal_$GLOBUS_NOTHR_FLAVOR"

  AC_LANG_SAVE
  AC_LANG_C
  CFLAGS="$RENEWAL_CFLAGS $CFLAGS"
  LIBS="$RENEWAL_THR_LIBS $LIBS"

  AC_TRY_COMPILE([ #include <glite/security/proxyrenewal/renewal.h> ],
                 [ edg_wlpr_ErrorCode error ],
                 [ ac_cv_renewal_thr_valid=yes ], [ac_cv_renewal_thr_valid=no ])
  CFLAGS=$ac_save_CFLAGS
  LIBS=$ac_save_LIBS
  AC_MSG_RESULT([$ac_cv_renewal_thr_valid for thr api])

  CPPFLAGS="$RENEWAL_CFLAGS $CPPFLAGS"
  LIBS="$RENEWAL_NOTHR_LIBS $LIBS"

  AC_TRY_COMPILE([ #include <glite/security/proxyrenewal/renewal.h> ],
                 [ edg_wlpr_ErrorCode error ],
                 [ ac_cv_renewal_nothr_valid=yes ], [ac_cv_renewal_nothr_valid=no ])
  CPPFLAGS=$ac_save_CPPFLAGS
  LIBS=$ac_save_LIBS
  AC_LANG_RESTORE
  AC_MSG_RESULT([$ac_cv_renewal_nothr_valid for nothr api])

  if test "x$ac_cv_renewal_nothr_valid" = "xyes" -a "x$ac_cv_renewal_thr_valid" = "xyes" ; then
     ifelse([$2], , :, [$2])
  else
     RENEWAL_THR_LIBS=""
     RENEWAL_NOTHR_LIBS=""
     ifelse([$3], , :, [$3])
  fi

  AC_SUBST(RENEWAL_THR_LIBS)
  AC_SUBST(RENEWAL_NOTHR_LIBS)

]
)


AC_DEFUN(AC_VOMS,
[

  with_voms_prefix=$GLITE_LOCATION

  AC_MSG_CHECKING([for VOMS installation at ${with_voms_prefix}])

  ac_save_CFLAGS=$CFLAGS
  ac_save_CPPFLAGS=$CPPFLAGS
  ac_save_LIBS=$LIBS
  if test -n "$with_voms_prefix" ; then
     VOMS_CFLAGS="-I$with_voms_prefix/include"
     VOMS_PATH_LIBS="-L$with_voms_prefix/lib"
  else
     VOMS_CFLAGS=""
     VOMS_PATH_LIBS=""
     VOMS_LIBS=""
     VOMS_THR_LIBS=""
     VOMS_NOTHR_LIBS=""
     VOMS_CPP_LIBS=""
     VOMS_CPP_THR_LIBS=""
     VOMS_CPP_NOTHR_LIBS=""
  fi

  VOMS_THR_LIBS="$VOMS_PATH_LIBS -lvomsc_$GLOBUS_THR_FLAVOR"
  VOMS_NOTHR_LIBS="$VOMS_PATH_LIBS -lvomsc_$GLOBUS_NOTHR_FLAVOR"

  VOMS_CPP_THR_LIBS="$VOMS_PATH_LIBS -lvomsapi_$GLOBUS_THR_FLAVOR"
  VOMS_CPP_NOTHR_LIBS="$VOMS_PATH_LIBS -lvomsapi_$GLOBUS_NOTHR_FLAVOR"

  VOMS_LIBS="$VOMS_PATH_LIBS -lvomsc"
  VOMS_CPP_LIBS="$VOMS_PATH_LIBS -lvomsapi"

  AC_LANG_SAVE
  AC_LANG_C
  CFLAGS="$GLOBUS_THR_CFLAGS $VOMS_CFLAGS $CFLAGS"
  LIBS="$VOMS_THR_LIBS $LIBS"

  AC_TRY_COMPILE([ #include <glite/security/voms/voms_apic.h> ],
                 [ struct vomsdata *voms_info = VOMS_Init("/tmp", "/tmp") ],
                 [ ac_cv_vomsc_valid=yes ], [ac_cv_vomsc_valid=no ])
  CFLAGS=$ac_save_CFLAGS
  LIBS=$ac_save_LIBS
  AC_MSG_RESULT([$ac_cv_vomsc_valid for c api])

  AC_LANG_CPLUSPLUS
  CPPFLAGS="$GLOBUS_THR_CFLAGS $VOMS_CFLAGS $CPPFLAGS"
  LIBS="$VOMS_CPP_THR_LIBS $LIBS"

  AC_TRY_COMPILE([ #include <glite/security/voms/voms_api.h> ],
                 [ vomsdata vo_data("","") ],
                 [ ac_cv_vomscpp_valid=yes ], [ac_cv_vomscpp_valid=no ])
  CPPFLAGS=$ac_save_CPPFLAGS
  LIBS=$ac_save_LIBS
  AC_LANG_RESTORE
  AC_MSG_RESULT([$ac_cv_vomscpp_valid for cpp api])

  if test "x$ac_cv_vomsc_valid" = "xyes" -a "x$ac_cv_vomscpp_valid" = "xyes" ; then
     VOMS_STATIC_LIBS="$with_voms_prefix/lib/libvomsc.a"
     VOMS_STATIC_THR_LIBS="$with_voms_prefix/lib/libvomsc_$GLOBUS_THR_FLAVOR.a"
     VOMS_STATIC_NOTHR_LIBS="$with_voms_prefix/lib/libvomsc_$GLOBUS_NOTHR_FLAVOR.a"
     ifelse([$2], , :, [$2])
  else
     VOMS_LIBS=""
     VOMS_THR_LIBS=""
     VOMS_NOTHR_LIBS=""
     VOMS_CPP_LIBS=""
     VOMS_CPP_THR_LIBS=""
     VOMS_CPP_NOTHR_LIBS=""
     VOMS_STATIC_LIBS=""
     VOMS_STATIC_THR_LIBS=""
     VOMS_STATIC_NOTHR_LIBS=""
     ifelse([$3], , :, [$3])
  fi

  AC_SUBST(VOMS_LIBS)
  AC_SUBST(VOMS_CPP_LIBS)
  AC_SUBST(VOMS_STATIC_LIBS)
  AC_SUBST(VOMS_THR_LIBS)
  AC_SUBST(VOMS_NOTHR_LIBS)
  AC_SUBST(VOMS_CPP_THR_LIBS)
  AC_SUBST(VOMS_CPP_NOTHR_LIBS)
  AC_SUBST(VOMS_STATIC_THR_LIBS)
  AC_SUBST(VOMS_STATIC_NOTHR_LIBS)
])


AC_DEFUN(AC_SEC_LCAS,
[
    with_lcas_prefix=$GLITE_LOCATION

    if test -n "with_lcas_prefix" ; then
        dnl
        dnl
        dnl
        with_lcas_prefix_lib="-L$with_lcas_prefix/lib"
        SEC_LCAS_LIBS="$with_lcas_prefix_lib -llcas"

        ifelse([$2], , :, [$2])
    else
        SEC_LCAS_LIBS=""
        ifelse([$3], , :, [$3])
    fi

    AC_SUBST(SEC_LCAS_LIBS)
])

AC_DEFUN(AC_SEC_LCMAPS,
[
    with_lcmaps_prefix=$GLITE_LOCATION

    if test -n "with_lcmaps_prefix" ; then
        dnl
        dnl
        dnl
        with_lcmaps_prefix_lib="-L$with_lcmaps_prefix/lib"
        SEC_LCMAPS_LIBS="$with_lcmaps_prefix_lib -llcmaps"

        ifelse([$2], , :, [$2])
    else
        SEC_LCMAPS_LIBS=""
        ifelse([$3], , :, [$3])
    fi

    AC_SUBST(SEC_LCMAPS_LIBS)
])

AC_DEFUN(AC_SEC_LCMAPS_WITHOUT_GSI,
[
    with_lcmaps_prefix=$GLITE_LOCATION

    if test -n "with_lcmaps_prefix" ; then
        dnl
        dnl
        dnl
        with_lcmaps_prefix_lib="-L$with_lcmaps_prefix/lib"
        SEC_LCMAPS_WITHOUT_GSI_LIBS="$with_lcmaps_prefix_lib -llcmaps_without_gsi"
        SEC_LCMAPS_RETURN_WITHOUT_GSI_LIBS="$with_lcmaps_prefix_lib -llcmaps_return_poolindex_without_gsi"

        ifelse([$2], , :, [$2])
    else
        SEC_LCMAPS_WITHOUT_GSI_LIBS=""
        SEC_LCMAPS_RETURN_WITHOUT_GSI_LIBS=""
        ifelse([$3], , :, [$3])
    fi

    AC_SUBST(SEC_LCMAPS_WITHOUT_GSI_LIBS)
    AC_SUBST(SEC_LCMAPS_RETURN_WITHOUT_GSI_LIBS)
])

AC_DEFUN(AC_SEC_GSOAP_PLUGIN,
[
    with_sec_gsoap_plugin_prefix=$GLITE_LOCATION

    if test -n "with_sec_gsoap_plugin_prefix" ; then
        dnl
        dnl
        dnl
        with_sec_gsoap_plugin_lib="-L$with_sec_gsoap_plugin_prefix/lib"
        SEC_GSOAP_PLUGIN_STATIC_THR_LIBS="$with_sec_gsoap_plugin_prefix/lib/libglite_security_gsoap_plugin_$GLOBUS_THR_FLAVOR.a"
	SEC_GSOAP_PLUGIN_STATIC_NOTHR_LIBS="$with_sec_gsoap_plugin_prefix/lib/libglite_security_gsoap_plugin_$GLOBUS_NOTHR_FLAVOR.a"

	SEC_GSOAP_PLUGIN_GSS_THR_LIBS="$with_sec_gsoap_plugin_lib -lglite_security_gss_$GLOBUS_THR_FLAVOR"
        SEC_GSOAP_PLUGIN_GSS_NOTHR_LIBS="$with_sec_gsoap_plugin_lib -lglite_security_gss_$GLOBUS_NOTHR_FLAVOR"

        ifelse([$2], , :, [$2])
    else
        SEC_GSOAP_PLUGIN_STATIC_THR_LIBS=""
        SEC_GSOAP_PLUGIN_STATIC_NOTHR_LIBS=""
        SEC_GSOAP_PLUGIN_GSS_THR_LIBS=""
        SEC_GSOAP_PLUGIN_GSS_NOTHR_LIBS=""
        ifelse([$3], , :, [$3])
    fi

    AC_SUBST(SEC_GSOAP_PLUGIN_STATIC_THR_LIBS)
    AC_SUBST(SEC_GSOAP_PLUGIN_STATIC_NOTHR_LIBS)
    AC_SUBST(SEC_GSOAP_PLUGIN_GSS_THR_LIBS)
    AC_SUBST(SEC_GSOAP_PLUGIN_GSS_NOTHR_LIBS)
])

