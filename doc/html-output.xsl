<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:xlink='http://www.w3.org/1999/xlink'
    version='1.0'>

<!-- Import the HTML stylesheet -->
<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl"/>

<!-- Turn on link generation for <citerefentry> -->
<xsl:param name="citerefentry.link">1</xsl:param>

<xsl:param name="html.cleanup">1</xsl:param>

<!-- These are mutually exclusive, select only one -->
<xsl:param name="refentry.generate.name">0</xsl:param>
<xsl:param name="refentry.generate.title">1</xsl:param>

<!-- TOC generation -->
<xsl:param name="generate.toc">
appendix  toc,title
article/appendix  nop
article   title
book      toc,title,figure,table,example,equation
chapter   toc,title
part      toc,title
preface   toc,title
qandadiv  toc
qandaset  toc
reference toc,title
sect1     toc
sect2     toc
sect3     toc
sect4     toc
sect5     toc
section   toc
set       toc,title
</xsl:param>

<!-- Template for generating <citerefentry> link names -->
<xsl:template name="generate.citerefentry.link">
    <xsl:value-of select="refentrytitle"/>
    <xsl:text>.</xsl:text>
    <xsl:value-of select="manvolnum"/>
    <xsl:text>.html</xsl:text>
</xsl:template>

</xsl:stylesheet>
<!-- vim: set ai sw=4: -->
