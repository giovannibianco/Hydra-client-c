dnl Usage:
dnl AC_GLITE_WMS_COMMON
dnl - GLITE_WMS_COMMON_CONF_LIBS
dnl - GLITE_WMS_COMMON_CONF_WRAPPER_LIBS
dnl - GLITE_WMS_COMMON_CONFIG_LIBS
dnl - GLITE_WMS_COMMON_LDIF2CLASSADS_LIBS
dnl - GLITE_WMS_COMMON_LOGGER_LIBS
dnl - GLITE_WMS_COMMON_PROCESS_LIBS
dnl - GLITE_WMS_COMMON_UT_UTIL_LIBS
dnl - GLITE_WMS_COMMON_UT_FTP_LIBS
dnl - GLITE_WMS_COMMON_UT_II_LIBS
dnl
dnl
dnl AC_GLITE_WMS_JDL
dnl - GLITE_WMS_JDL_CFLAGS
dnl - GLITE_WMS_JDL_LIBS
dnl
dnl
dnl AC_GLITE_WMS_CHKPT
dnl - GLITE_WMS_CHKPT_CFLAGS
dnl - GLITE_WMS_CHKPT_LIBS
dnl
dnl
dnl AC_GLITE_WMS_PARTITIONER
dnl - GLITE_WMS_PARTITIONER_LIBS

AC_DEFUN(AC_GLITE_WMS_CHKPT,
[
    ac_glite_wms_chkpt_prefix=$GLITE_LOCATION

    if test -n "ac_glite_wms_chkpt_prefix" ; then
        GLITE_WMS_CHKPT_CFLAGS="-I$ac_glite_wms_chkpt_prefix/include/glite/wms/checkpointing"
        dnl
        dnl
        dnl
        ac_glite_wms_chkpt_lib="-L$ac_glite_wms_chkpt_prefix/lib"
        GLITE_WMS_CHKPT_LIBS="$ac_glite_wms_chkpt_lib -lglite_wms_checkpointing"
        ifelse([$2], , :, [$2])
    else
        GLITE_WMS_CHKPT_CFLAGS=""
        GLITE_WMS_CHKPT_LIBS=""
        ifelse([$3], , :, [$3])
    fi

    AC_SUBST(GLITE_WMS_CHKPT_CFLAGS)
    AC_SUBST(GLITE_WMS_CHKPT_LIBS)
])

AC_DEFUN(AC_GLITE_WMS_JDL,
[
    ac_glite_wms_jdl_prefix=$GLITE_LOCATION

    if test -n "ac_glite_wms_jdl_prefix" ; then
        GLITE_WMS_JDL_CFLAGS="-I$ac_glite_wms_jdl_prefix/include/glite/wms/jdl"
        dnl
        dnl
        dnl
        ac_glite_wms_jdl_lib="-L$ac_glite_wms_jdl_prefix/lib"
        GLITE_WMS_JDL_LIBS="$ac_glite_wms_jdl_lib -lglite_wms_jdl"
        ifelse([$2], , :, [$2])
    else
        GLITE_WMS_JDL_CFLAGS=""
        GLITE_WMS_JDL_LIBS=""
        ifelse([$3], , :, [$3])
    fi

    AC_SUBST(GLITE_WMS_JDL_CFLAGS)
    AC_SUBST(GLITE_WMS_JDL_LIBS)
])

AC_DEFUN(AC_GLITE_WMS_PARTITIONER,
[
    ac_glite_wms_partitioner_prefix=$GLITE_LOCATION

    if test -n "ac_glite_wms_partioner_prefix" ; then
        dnl
        dnl
        dnl
        ac_glite_wms_partitioner_lib="-L$ac_glite_wms_partitioner_prefix/lib"
        GLITE_WMS_PARTITIONER_LIBS="$ac_glite_wms_partitioner_lib -lglite_wms_partitioner"
        ifelse([$2], , :, [$2])
    else
        GLITE_WMS_PARTITIONER_LIBS=""
        ifelse([$3], , :, [$3])
    fi

    AC_SUBST(GLITE_WMS_PARTITIONER_LIBS)
])

AC_DEFUN(AC_GLITE_WMS_COMMON,
[
    ac_glite_wms_common_prefix=$GLITE_LOCATION

    if test -n "ac_glite_wms_common_prefix" ; then
	dnl
	dnl 
	dnl
        ac_glite_wms_common_lib="-L$ac_glite_wms_common_prefix/lib"
	GLITE_WMS_COMMON_CONF_LIBS="$ac_glite_wms_common_lib -lglite_wms_conf"
	GLITE_WMS_COMMON_CONF_WRAPPER_LIBS="$ac_glite_wms_common_lib -lglite_wms_conf_wrapper"
	GLITE_WMS_COMMON_CONFIG_LIBS="$GLITE_WMS_COMMON_CONF_LIBS -lglite_wms_conf_wrapper"
	GLITE_WMS_COMMON_LDIF2CLASSADS_LIBS="$ac_glite_wms_common_lib -lglite_wms_LDIF2ClassAd"
	GLITE_WMS_COMMON_LOGGER_LIBS="$ac_glite_wms_common_lib -lglite_wms_logger"
	GLITE_WMS_COMMON_PROCESS_LIBS="$ac_glite_wms_common_lib -lglite_wms_process"
	GLITE_WMS_COMMON_UT_UTIL_LIBS="$ac_glite_wms_common_lib -lglite_wms_util"
	GLITE_WMS_COMMON_UT_FTP_LIBS="$ac_glite_wms_common_lib -lglite_wms_globus_ftp_util"
	GLITE_WMS_COMMON_UT_II_LIBS="$ac_glite_wms_common_lib -lglite_wms_iiattrutil"
	ifelse([$2], , :, [$2])
    else
	GLITE_WMS_COMMON_CONF_LIBS=""
	GLITE_WMS_COMMON_CONF_WRAPPER_LIBS=""
	GLITE_WMS_COMMON_CONFIG_LIBS=""
	GLITE_WMS_COMMON_LDIF2CLASSADS_LIBS=""
	GLITE_WMS_COMMON_LOGGER_LIBS=""
	GLITE_WMS_COMMON_PROCESS_LIBS=""
	GLITE_WMS_COMMON_UT_UTIL_LIBS=""
	GLITE_WMS_COMMON_UT_FTP_LIBS=""
	GLITE_WMS_COMMON_UT_II_LIBS=""
	ifelse([$3], , :, [$3])
    fi

    AC_SUBST(GLITE_WMS_COMMON_CONF_LIBS)
    AC_SUBST(GLITE_WMS_COMMON_CONF_WRAPPER_LIBS)
    AC_SUBST(GLITE_WMS_COMMON_CONFIG_LIBS)
    AC_SUBST(GLITE_WMS_COMMON_LDIF2CLASSADS_LIBS)
    AC_SUBST(GLITE_WMS_COMMON_LOGGER_LIBS)
    AC_SUBST(GLITE_WMS_COMMON_PROCESS_LIBS)
    AC_SUBST(GLITE_WMS_COMMON_UT_UTIL_LIBS)
    AC_SUBST(GLITE_WMS_COMMON_UT_FTP_LIBS)
    AC_SUBST(GLITE_WMS_COMMON_UT_II_LIBS)	
])

