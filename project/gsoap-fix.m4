dnl
dnl Define GSOAP Fix scripts
dnl
AC_DEFUN(AC_GSOAP_FIX,
[
	GSOAP_FIX_CPP_COMMON='perl $(top_builddir)/script/fixCppCommon.pl'
	GSOAP_FIX_ADD_CPP_NAMESPACE='perl $(top_builddir)/script/addCppNamespace.pl'
	GSOAP_FIX_GENERATE_C_COMMON='perl $(top_builddir)/script/generateCommon.pl env.c soapdefs.h'
	GSOAP_FIX_GENERATE_CPP_COMMON='perl $(top_builddir)/script/generateCommon.pl env.cpp soapdefs.h'
	
	dnl
	dnl Check If Fault Details Should be Fixed
	dnl 
	if test [ "$GSOAP_WSDL2H_VERSION_NUM" -ge "020600"  -a  "$GSOAP_VERSION_NUM" -lt "020600" ] ; then
		GSOAP_FIX_FAULT_DETAIL='perl $(top_builddir)/script/fixFaultDetail.pl'
	else
		GSOAP_FIX_FAULT_DETAIL='@echo No Patch Needed for '
	fi
	
	AC_MSG_RESULT([GSOAP_FIX_CPP_COMMON is $GSOAP_FIX_CPP_COMMON])
	AC_MSG_RESULT([GSOAP_FIX_ADD_CPP_NAMESPACE  is $GSOAP_FIX_ADD_CPP_NAMESPACE])
	AC_MSG_RESULT([GSOAP_FIX_GENERATE_C_COMMON is $GSOAP_FIX_GENERATE_C_COMMON])
	AC_MSG_RESULT([GSOAP_FIX_GENERATE_CPP_COMMON is $GSOAP_FIX_GENERATE_CPP_COMMON])
	AC_MSG_RESULT([GSOAP_FIX_FAULT_DETAIL is $GSOAP_FIX_FAULT_DETAIL])
	
	AC_SUBST(GSOAP_FIX_CPP_COMMON)
	AC_SUBST(GSOAP_FIX_ADD_CPP_NAMESPACE)
	AC_SUBST(GSOAP_FIX_GENERATE_C_COMMON)
	AC_SUBST(GSOAP_FIX_GENERATE_CPP_COMMON)
	AC_SUBST(GSOAP_FIX_FAULT_DETAIL)
])