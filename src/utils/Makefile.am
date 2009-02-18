#
#  Copyright (c) Members of the EGEE Collaboration. 2004.
#  See http://eu-egee.org/partners/ for details on the copyright holders.
#  For license conditions see the license file or http://eu-egee.org/license.html
#
#  src/utils/Makefile.am file for the GLite Encrypted Data Storage module
#
#  Authors: 
#  		Akos Frohner <Akos.Frohner@cern.ch>
#  		Zoltan Farkas <Zoltan.Farkas@cern.ch>
#  

bin_PROGRAMS = glite-eds-get \
               glite-eds-put \
               glite-eds-rm \
               glite-eds-encrypt \
               glite-eds-decrypt \
			   glite-eds-key-register \
			   glite-eds-key-unregister \
			   glite-eds-setacl \
			   glite-eds-chmod \
			   glite-eds-getacl

glite_data_io_ldflags   = \
	$(GLITE_LDFLAGS) -lgfal -lcgsi_plugin_gsoap_2.7 -lvomsc \
	-lglite_data_util -L$(GLITE_LOCATION)/lib -lgridsite_globus -lglite-sd-c \
	-lglite_security_ssss \
	$(GLOBUS_GSS_THR_LIBS) $(GLOBUS_SSL_THR_LIBS) \
	../c/libglite_data_eds_simple.la

glite_eds_get_SOURCES = eds-getfile.c

glite_eds_put_SOURCES = eds-putfile.c

glite_eds_rm_SOURCES  = eds-unlinkfile.c

glite_eds_get_LDADD = $(glite_data_io_ldflags)

glite_eds_put_LDADD = $(glite_data_io_ldflags)

glite_eds_rm_LDADD  = $(glite_data_io_ldflags)

glite_data_eds_client_ldflags   = \
	$(GLITE_LDFLAGS) ../c/libglite_data_eds_simple.la \
	-lglite_data_util -L$(GLITE_LOCATION)/lib -lgridsite_globus -lglite-sd-c \
	$(GLOBUS_GSS_THR_LIBS) $(GLOBUS_SSL_THR_LIBS)


glite_eds_encrypt_SOURCES  = eds-encrypt.c

glite_eds_decrypt_SOURCES  = eds-decrypt.c

glite_eds_key_register_SOURCES  = eds-register.c

glite_eds_key_unregister_SOURCES  = eds-unregister.c

glite_eds_encrypt_LDADD  = $(glite_data_eds_client_ldflags)

glite_eds_decrypt_LDADD  = $(glite_data_eds_client_ldflags)

glite_eds_key_register_LDADD  = $(glite_data_eds_client_ldflags)

glite_eds_key_unregister_LDADD  = $(glite_data_eds_client_ldflags)

glite_data_hydra_client_ldflags   = \
	$(GLITE_LDFLAGS) ../c/libglite_data_eds_simple.la \
	-lglite_data_util -L$(GLITE_LOCATION)/lib -lgridsite_globus -lglite-sd-c \
	$(GLOBUS_GSS_THR_LIBS) $(GLOBUS_SSL_THR_LIBS) \
	$(GLIB_LIBS)

glite_eds_setacl_SOURCES  = eds-setacl.c meta-tool-main.c util.c

glite_eds_chmod_SOURCES  = eds-chmod.c meta-tool-main.c util.c

glite_eds_getacl_SOURCES  = eds-getacl.c meta-tool-main.c util.c

glite_eds_setacl_LDADD  = $(glite_data_hydra_client_ldflags)

glite_eds_chmod_LDADD  = $(glite_data_hydra_client_ldflags)

glite_eds_getacl_LDADD  = $(glite_data_hydra_client_ldflags)

AM_CPPFLAGS = -I$(top_srcdir)/interface \
	-I$(top_srcdir)/src \
	$(GLITE_CFLAGS) \
	$(GLIB_CFLAGS) \
	-D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE

MAINTAINERCLEANFILES = Makefile.in
