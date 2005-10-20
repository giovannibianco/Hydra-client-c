#
#  Copyright (c) Members of the EGEE Collaboration. 2004.
#  See http://eu-egee.org/partners/ for details on the copyright holders.
#  For license conditions see the license file or http://eu-egee.org/license.html
#
#  configure.ac file for the EDS client module
#
#  Authors: Zoltan Farkas <Zoltan.Farkas@cern.ch>
#

AC_PREREQ(2.57)
AC_INIT([GLite Data Hydra CLI], [0.0.0])
AC_CONFIG_AUX_DIR([./project])
AM_INIT_AUTOMAKE([1.6.3 subdir-objects])

# Notices.
AC_COPYRIGHT([Copyright (c) 2004 The EU EGEE Project
See LICENCE file for details
])
AC_REVISION([$Revision: 1.3 $])

#Environment.
WORKDIR=`pwd`
AC_SUBST(WORKDIR)

# Check for the basic compile environment.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_LN_S
# Make libtool less verbose
LIBTOOL="$LIBTOOL --silent"

# Turn on warnings
if test "$ac_cv_c_compiler_gnu" = yes; then
	CFLAGS="$CFLAGS -Wall -W -Wmissing-prototypes"
fi

# Check for the basic compile environment.
AC_PROG_INSTALL
AC_PROG_MAKE_SET


dnl Standard warnings.
WARNINGS=""
#WARNINGS="-Wall -Wpointer-arith -Wmissing-prototypes -Woverloaded-virtual -Wmissing-declarations 
#-Wcast-align -Wwrite-strings -Wformat"

# default compiler flags
am_cxx_flags="$WARNINGS $am_cxx_flags"

# default linker flags
am_ld_flags="$am_ld_flags"

# Arguments.

AC_ARG_ENABLE(
	[debug],
	[  --enable-debug     Produce an executable with debugging symbols],
	[CXXFLAGS="-g -D_DEBUG $CXXFLAGS"])

################################################################################
# Check for available functions.

# Check for header files.
AC_CHECK_HEADERS([\
	fcntl.h\
	sys/types.h\
	])

# Set Globus Properties
have_globus=no
AC_GLOBUS([], have_globus=yes, have_globus=no)
AC_MSG_RESULT(["GLOBUS found $have_globus"])

# Set GLite Properties
AC_GLITE
AC_GLITE_VERSION

MAKEDEPEND="\$(CXX) -M"

am_cxx_flags="$am_cxx_flags -I $prefix/include"

AM_PATH_LOG4CPP

# Look for xsltproc & manpage stylesheets
GLITE_DOCBOOK_MAN

# Check that the catalog-fireman-api-c package is available
save_CPPFLAGS="$CPPFLAGS"
save_LDFLAGS="$LDFLAGS"
save_LIBS="$LIBS"
CPPFLAGS="$CPPFLAGS $GLITE_CFLAGS $GSOAP_CFLAGS"
LDFLAGS="$LDFLAGS -L$GLITE_LOCATION/lib"
AC_CHECK_HEADER([glite/data/catalog/fireman/c/firemanStub.h],,
	[AC_MSG_ERROR([Headers from the org.glite.data.catalog-fireman-api-c package are not available])])
dnl AC_CHECK_LIB([glite_data_catalog_fireman_api_c], [soap_call_fireman__getVersion],,
dnl 	[AC_MSG_ERROR([Libraries from the org.glite.data.catalog-fireman-api-c package are not available])])
LIBS="$save_LIBS"
LDFLAGS="$save_LDFLAGS"
CPPFLAGS="$save_CPPFLAGS"

# Check that the catalog-meta-api-c package is available
save_CPPFLAGS="$CPPFLAGS"
save_LDFLAGS="$LDFLAGS"
save_LIBS="$LIBS"
CPPFLAGS="$CPPFLAGS $GLITE_CFLAGS $GSOAP_CFLAGS"
LDFLAGS="$LDFLAGS -L$GLITE_LOCATION/lib"
AC_CHECK_HEADER([glite/data/catalog/metadata/c/metadataStub.h],,
	[AC_MSG_ERROR([Headers from the org.glite.data.catalog-meta-api-c package are not available])])
dnl AC_CHECK_LIB([glite_data_catalog_metadata_api_c], [soap_call_metadata__getVersion],,
dnl 	[AC_MSG_ERROR([Libraries from the org.glite.data.catalog-meta-api-c package are not available])])
LIBS="$save_LIBS"
LDFLAGS="$save_LDFLAGS"
CPPFLAGS="$save_CPPFLAGS"

# Location of the HTML documentation
AC_ARG_WITH([htmldir],
	AC_HELP_STRING([--with-htmldir],
		[Where to put the HTML documentation @<:@${datadir}/doc/${PACKAGE}/html@:>@]),
	[htmldir=$withval],
	[htmldir='${datadir}/doc/${PACKAGE}/html'])
AC_SUBST([htmldir])

# Configuration items
AC_PREFIX_DEFAULT([/opt/glite])
AM_CONFIG_HEADER([src/autogen/config.h])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([interface/Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/c/Makefile])
AC_CONFIG_FILES([src/utils/Makefile])
AC_CONFIG_FILES([doc/Makefile])
AC_CONFIG_FILES([doc/html/Makefile])
AC_CONFIG_FILES([doc/man/Makefile])

AC_OUTPUT
