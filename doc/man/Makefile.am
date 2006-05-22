man1_MANS = \
		glite-eds-put.1 \
		glite-eds-get.1 \
		glite-eds-rm.1 \
		glite-eds-encrypt.1 \
		glite-eds-decrypt.1 \
		glite-eds-key-register.1 \
		glite-eds-key-unregister.1 \
		glite-eds-getacl.1 \
		glite-eds-chmod.1 \
		glite-eds-setacl.1

COMMON_XMLFILES = common-gliteio-env.xml

EXTRA_DIST = $(addsuffix .xml,$(man_MANS)) $(COMMON_XMLFILES)
CLEANFILES = $(man1_MANS) $(addsuffix .html,$(man1_MANS))

MAN_STYLESHEET = http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl
HTML_STYLESHEET = $(top_srcdir)/doc/html-output.xsl
LATEX_STYLESHEET = $(top_srcdir)/doc/man/latex-output.xsl

if HAVE_DOCBOOK_MAN
html_DATA = $(addsuffix .html,$(man1_MANS))

# this target is not required in the normal builds
# so one has to run it by hand to generate LaTeX
# formatted manual pages (you need the db2latex-xsl
# extension package for DocBook to use it!)
#
#       make texdoc
#
texdoc: $(addsuffix .tex,$(man1_MANS))

$(man1_MANS): %.1: %.1.xml $(COMMON_XMLFILES)
	$(XSLTPROC) $(XSLTPROCFLAGS) $(MAN_STYLESHEET) $<

$(addsuffix .html,$(man1_MANS)): %.1.html: %.1.xml $(COMMON_XMLFILES) $(HTML_STYLESHEET)
	$(XSLTPROC) $(XSLTPROCFLAGS) -o $@ $(HTML_STYLESHEET) $<

$(addsuffix .tex,$(man1_MANS)): %.1.tex: %.1.xml $(COMMON_XMLFILES) $(LATEX_STYLESHEET)
	$(XSLTPROC) $(XSLTPROCFLAGS) -o $@ $(LATEX_STYLESHEET) $<
endif
