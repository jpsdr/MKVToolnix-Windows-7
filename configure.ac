AC_INIT([MKVToolNix],[95.0],[https://codeberg.org/mbunkus/mkvtoolnix/issues],[mkvtoolnix])
AC_PREREQ([2.69])

AC_CONFIG_HEADERS(config.h:config.h.in)

dnl Optional features that are built and those that aren't
opt_features_yes=""
opt_features_no=""

m4_include(ac/other_features.m4)
m4_include(ac/pkg.m4)
m4_include(ac/initialization.m4)
m4_include(ac/check_version.m4)
m4_include(ac/compiler_version.m4)
m4_include(ac/c++-features.m4)
m4_include(ac/endianess.m4)
m4_include(ac/macos.m4)
m4_include(ac/mingw.m4)
m4_include(ac/extra_inc_lib.m4)
m4_include(ac/drmingw.m4)
m4_include(ac/debugging_profiling.m4)
m4_include(ac/precompiled_headers.m4)
m4_include(ac/inttypes.m4)
m4_include(ac/iconv.m4)
m4_include(ac/nl_langinfo.m4)
m4_include(ac/ogg.m4)
m4_include(ac/static.m4)
m4_include(ac/vorbis.m4)
m4_include(ac/flac.m4)
m4_include(ac/matroska.m4)
m4_include(ac/pugixml.m4)
m4_include(ac/nlohmann_jsoncpp.m4)
m4_include(ac/utf8cpp.m4)
m4_include(ac/fmt.m4)
m4_include(ac/zlib.m4)
m4_include(ac/qt6.m4)
m4_include(ac/cmark.m4)
m4_include(ac/gnurx.m4)
m4_include(ac/ax_boost_base.m4)
m4_include(ac/ax_boost_check_headers.m4)
m4_include(ac/ax_boost_filesystem.m4)
m4_include(ac/ax_boost_system.m4)
m4_include(ac/boost.m4)
m4_include(ac/gtest.m4)
m4_include(ac/benchmark.m4)
m4_include(ac/pandoc.m4)
m4_include(ac/ax_docbook.m4)
m4_include(ac/tiocgwinsz.m4)
m4_include(ac/dvdread.m4)
m4_include(ac/po4a.m4)
m4_include(ac/translations.m4)
m4_include(ac/manpages_translations.m4)

AC_CONFIG_FILES([build-config])
AC_OUTPUT

echo ''
echo 'The mkvtoolnix configuration is complete.'
printf 'Optional features that are built: '"$opt_features_yes"'\n'
printf 'Optional features that are NOT built: '"$opt_features_no"'\n'
