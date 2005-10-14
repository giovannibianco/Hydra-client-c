#
#  Copyright (c) Members of the EGEE Collaboration. 2005.
#  See http://eu-egee.org/partners/ for details on the copyright holders.
#  For license conditions see the license file or http://eu-egee.org/license.html
#
#  Simple interface wrapper for the encrypted data storage
#
#  Authors:
#	Zoltan Farkas <Zoltan.Farkas@cern.ch>
#

AM_CPPFLAGS= -I$(top_srcdir)/interface $(GLOBUS_THR_CFLAGS) $(GLITE_CFLAGS) $(GLIB_CFLAGS)

lib_LTLIBRARIES = libglite_data_eds_simple.la

libglite_data_eds_simple_la_SOURCES  = eds-simple.c

libglite_data_eds_simple_la_LIBADD = \
	-L$(GLITE_LOCATION)/lib -lglite_data_util -lglite-sd-c \
	-lglite_data_catalog_metadata_api_simple_c \
	-lglite_data_catalog_fireman_api_simple_c \
	-lglite_data_catalog_api_simple_c $(GLIB_LIBS)

libglite_data_eds_simple_la_LDFLAGS = \
	-version-info $(INTERFACE_LIBTOOL_CURRENT):$(INTERFACE_LIBTOOL_REVISION):$(INTERFACE_LIBTOOL_AGE)
