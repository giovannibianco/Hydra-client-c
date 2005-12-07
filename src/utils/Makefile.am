#
#  Copyright (c) Members of the EGEE Collaboration. 2004.
#  See http://eu-egee.org/partners/ for details on the copyright holders.
#  For license conditions see the license file or http://eu-egee.org/license.html
#
#  src/utils/Makefile.am file for the GLite Encrypted Data Storage module
#
#  Authors: Zoltan Farkas <Zoltan.Farkas@cern.ch>
#  

bin_PROGRAMS = glite-eds-get \
               glite-eds-put \
               glite-eds-rm \
               glite-eds-encrypt

glite_data_io_client_ldflags   = \
	-L$(GLITE_LOCATION)/lib -lglite_data_io_base -lglite_data_io_client \
	-lglite_data_catalog_metadata_api_simple_c -lglite_data_catalog_api_simple_c \
	-lglite_data_catalog_fireman_api_simple_c ../c/libglite_data_eds_simple.la \
	-lglite_data_util -lglite-sd-c $(GLOBUS_GSS_THR_LIBS) $(GLOBUS_SSL_THR_LIBS) \
	$(LOG4CPP_LIBS)

glite_eds_get_SOURCES = eds-getfile.cxx

glite_eds_put_SOURCES = eds-putfile.cxx

glite_eds_rm_SOURCES  = eds-unlinkfile.cxx

glite_eds_encrypt_SOURCES  = eds-encrypt.cxx

glite_eds_get_LDADD = $(glite_data_io_client_ldflags)

glite_eds_put_LDADD = $(glite_data_io_client_ldflags)

glite_eds_rm_LDADD  = $(glite_data_io_client_ldflags)

glite_eds_encrypt_LDADD  = $(glite_data_io_client_ldflags)

AM_CPPFLAGS = -I$(top_srcdir)/interface \
	-I$(top_srcdir)/src \
	$(GLITE_CFLAGS) \
	$(LOG4CPP_CFLAGS) \
	-D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE

MAINTAINERCLEANFILES = Makefile.in
