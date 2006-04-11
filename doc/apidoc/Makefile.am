#
#  Copyright (c) Members of the EGEE Collaboration. 2004.
#  See http://eu-egee.org/partners/ for details on the copyright holders.
#  For license conditions see the license file or http://eu-egee.org/license.html
#
#  Common doc/html/Makefile.am file for the GLite Data IO Client module
#
#  Authors: Akos Frohner <Akos.Frohner@cern.ch>
#  Version info: $Id: Makefile.am,v 1.1 2006-04-11 13:24:28 szamsu Exp $
#  Release: $Name: not supported by cvs2svn $
#
#
commonutdir = $(datadir)/doc/$(PACKAGE)/apidoc
dist_commonut_DATA = \
	doxygen.css \
	doxygen.png \
	eds-simple_8h-source.html \
	eds-simple_8h.html \
	files.html \
	globals.html \
	globals_func.html \
	index.html

Doxyfile: $(srcdir)/Doxyfile.template
	sed -e "s#\@top_srcdir\@#$(top_srcdir)#; s#\@srcdir\@#$(srcdir)#" $(srcdir)/Doxyfile.template >$@

$(dist_commonut_DATA): Doxyfile
	doxygen Doxyfile

MAINTAINERCLEANFILES = Makefile.in
