<?xml version="1.0" encoding="utf-8"?>

<!--

Chapters to shnsplit
====================

This is a XSLT 2.0 stylesheet for turning the chapter format that mkvmerge
understands into a list of timecodes for the "shnsplit" tool from the
"shntool" package (see http://etree.org/shnutils/shntool/). It allows you to
split a large audio file into smaller ones, optionally encoding/decoding in
the process.

You need a XSLT 2.0 compatible XSLT processor, e.g. Saxon-HE (see
http://saxon.sourceforge.net/).

Usage depends on your processor. For Saxon-HE on Linux this should work:

  $ java -classpath saxon9he.jar net.sf.saxon.Transform \
      -o:splitpoints.txt \
      -xsl:chapters-to-shnsplit.xsl \
      input-filename.xml \
      artist="Name of the artist" \
      album="Name of the album"

You have to extract the chapters from a Matroska file with mkvextract first if
you want to turn the chapters included in one into split points.

This can be useful if you want to turn an audio file contain in a Matroska
file into one file per chapter (e.g. for music videos). For this you need both
"chapters-to-cuesheet.xsl" and "chapters-to-shnsplit.xsl" as well as the
"cuetools" (see https://github.com/svend/cuetools) and "shntool" (see
http://etree.org/shnutils/shntool/) packages.

Assumptions are (meaning: adjust these values in the commands):

- the source file contains an audio track with track ID 2 of type FLAC,
- the band is "Perpetuum Jazzile",
- the album is "Live 2009",
- the source file name is "source.mkv".

Here we go:

  # 1. Extract audio track from Matroska file:
  $ mkvextract source.mkv tracks 2:source.flac

  # 2. Extract chapters from same file:
  $ mkvextract source.mkv chapters > chapters.xml

  # 3. Generate the split points used in step 4:
  $ java -classpath saxon9he.jar net.sf.saxon.Transform \
      -o:splitpoints.txt \
      -xsl:chapters-to-cuesheet.xsl \
      chapters.xml

  # 4. Split the audio file into multiple files:
  $ shnsplit -o flac source.flac < splitpoints.txt

  # 5. Generate the cue sheet used in step 6:
  $ java -classpath saxon9he.jar net.sf.saxon.Transform \
      -o:source.cue \
      -xsl:chapters-to-cuesheet.xsl \
      chapters.xml \
      artist="Perpetuum Jazzile" \
      album="Live 2009"

  # 6: Assign tags (artist and title):
  $ cuetag source.cue split-*.flac

Written by Moritz Bunkus <mo@bunkus.online>.

-->

<xsl:stylesheet version="2.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:xs="http://www.w3.org/2001/XMLSchema">
 <xsl:output method="text"/>
 <xsl:strip-space elements="*"/>

 <xsl:template match="/Chapters">
  <xsl:apply-templates select="EditionEntry"/>
 </xsl:template>

 <xsl:template name="timecode-to-ns" as="xs:integer">
  <xsl:param name="timecode" select="'0'"/>
  <xsl:choose>
   <xsl:when test="$timecode eq ''">
    <xsl:value-of select="0"/>
   </xsl:when>
   <xsl:when test="matches($timecode, '^\d+:\d+:\d+\.\d{9}$')">
    <xsl:analyze-string select="$timecode"
                        regex="^ (\d+) : (\d+) : (\d+) \. (\d+) $"
                        flags="x">
     <xsl:matching-substring>
      <xsl:value-of select="  (regex-group(1) cast as xs:integer) * 3600000000000
                            + (regex-group(2) cast as xs:integer) *   60000000000
                            + (regex-group(3) cast as xs:integer) *    1000000000
                            + (regex-group(4) cast as xs:integer)"/>
     </xsl:matching-substring>
    </xsl:analyze-string>
   </xsl:when>
   <xsl:when test="matches($timecode, '^\d+:\d+:\d+\.\d*$')">
    <xsl:call-template name="timecode-to-ns">
     <xsl:with-param name="timecode" select="concat($timecode, '0')"/>
    </xsl:call-template>
   </xsl:when>
   <xsl:when test="matches($timecode, '^\d+:\d+:\d+$')">
    <xsl:call-template name="timecode-to-ns">
     <xsl:with-param name="timecode" select="concat($timecode, '.')"/>
    </xsl:call-template>
   </xsl:when>
   <xsl:otherwise>
    <xsl:call-template name="timecode-to-ns">
     <xsl:with-param name="timecode" select="concat('0:', $timecode)"/>
    </xsl:call-template>
   </xsl:otherwise>
  </xsl:choose>
 </xsl:template>

 <xsl:template match="EditionEntry">
  <xsl:for-each select="ChapterAtom">
   <xsl:variable name="timecode" as="xs:integer">
    <xsl:call-template name="timecode-to-ns">
     <xsl:with-param name="timecode" select="ChapterTimeStart"/>
    </xsl:call-template>
   </xsl:variable>
   <xsl:if test="$timecode &gt; 0">
    <xsl:number format="01" value=" $timecode idiv 60000000000"/>
    <xsl:text>:</xsl:text>
    <xsl:number format="01" value="($timecode idiv  1000000000) mod 60"/>
    <xsl:text>.</xsl:text>
    <xsl:number format="001" value="($timecode mod 1000000000) idiv 1000000"/>
    <xsl:text>&#xa;</xsl:text>
   </xsl:if>
  </xsl:for-each>
 </xsl:template>
</xsl:stylesheet>
