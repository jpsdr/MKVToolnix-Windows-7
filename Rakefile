# coding: utf-8

if Signal.list.key?('ALRM')
  Signal.trap('ALRM') { |signo| }
end

version = RUBY_VERSION.gsub(/[^0-9\.]+/, "").split(/\./).collect(&:to_i)
version << 0 while version.size < 3
if (version[0] < 2) && (version[1] < 9)
  fail "Ruby 1.9.x or newer is required for building"
end

# Change to base directory before doing anything
if FileUtils.pwd != File.dirname(__FILE__)
  new_dir = File.absolute_path(File.dirname(__FILE__))
  puts "Entering directory `#{new_dir}'"
  Dir.chdir new_dir
end

# Set number of threads to use if it is unset and we're running with
# drake.
if Rake.application.options.respond_to?(:threads) && [nil, 0, 1].include?(Rake.application.options.threads) && !ENV['DRAKETHREADS'].nil?
  Rake.application.options.threads = ENV['DRAKETHREADS'].to_i
end

# For newer rake turn on parallel processing, too. Newer rake versions
# use an OpenStruct, though, so testing with responds_to? won't work.
version = Rake::VERSION.gsub(%r{[^0-9\.]+}, "").split(%r{\.}).map(&:to_i)
if (version[0] > 10) || ((version[0] == 10) && (version[1] >= 3))
  Rake.application.options.always_multitask = true
end

require "pp"

module Mtx
end

require_relative "rake.d/extensions"
require_relative "rake.d/config"

$config               = read_build_config
$verbose              = ENV['V'].to_bool
$build_system_modules = {}
$have_gtest           = (c(:GTEST_TYPE) == "system") || (c(:GTEST_TYPE) == "internal")
$gtest_apps           = []

require_relative "rake.d/helpers"
require_relative "rake.d/target"
require_relative "rake.d/application"
require_relative "rake.d/installer"
require_relative "rake.d/library"
require_relative "rake.d/format_string_verifier"
require_relative "rake.d/pch"
require_relative "rake.d/po"
require_relative "rake.d/source_tests"
require_relative "rake.d/tarball"
require_relative 'rake.d/gtest' if $have_gtest

def setup_globals
  $building_for = {
    :linux   => %r{linux}i.match(c(:host)),
    :macos   => %r{darwin}i.match(c(:host)),
    :windows => %r{mingw}i.match(c(:host)),
  }

  $build_mkvtoolnix_gui  ||=  c?(:USE_QT)
  $build_mkvinfo_gui       =  c?(:USE_QT) && ($building_for[:windows] || $building_for[:macos])
  $build_tools           ||=  c?(:BUILD_TOOLS)

  $programs                =  %w{mkvmerge mkvinfo mkvextract mkvpropedit}
  $programs                << "mkvinfo-gui"    if $build_mkvinfo_gui
  $programs                << "mkvtoolnix-gui" if $build_mkvtoolnix_gui
  $tools                   =  %w{ac3parser base64tool checksum diracparser ebml_validator hevc_dump hevcc_dump mpls_dump vc1parser}

  $application_subdirs     =  { "mkvtoolnix-gui" => "mkvtoolnix-gui/" }
  $applications            =  $programs.collect { |name| "src/#{$application_subdirs[name]}#{name}" + c(:EXEEXT) }
  $manpages                =  $programs.reject { |name| %r{mkvinfo-gui}.match(name) }.map { |name| "doc/man/#{name}.1" }
  $manpages                << "doc/man/mkvtoolnix-gui.1" if $build_mkvtoolnix_gui

  $system_includes         = "-I. -Ilib -Ilib/avilib-0.6.10 -Isrc"
  $system_includes        += " -Ilib/utf8-cpp/source" if c?(:UTF8CPP_INTERNAL)
  $system_includes        += " -Ilib/pugixml/src"     if c?(:PUGIXML_INTERNAL)
  $system_libdirs          = "-Llib/avilib-0.6.10 -Llib/librmff -Lsrc/common"

  $source_directories      =  %w{lib/avilib-0.6.10 lib/librmff src/common src/extract src/info src/input src/merge src/mkvtoolnix-gui src/mpegparser src/output src/propedit}
  $all_sources             =  $source_directories.collect { |dir| FileList[ "#{dir}/**/*.c", "#{dir}/**/*.cpp" ].to_a }.flatten.sort
  $all_headers             =  $source_directories.collect { |dir| FileList[ "#{dir}/**/*.h",                   ].to_a }.flatten.sort
  $all_objects             =  $all_sources.collect { |file| file.ext('o') }
  $all_app_po_files        =  FileList["po/*.po"].to_a.sort
  $all_man_po_files        =  FileList["doc/man/po4a/po/*.po"].to_a.sort
  $all_po_files            =  $all_app_po_files + $all_man_po_files

  $gui_ui_files            = FileList["src/mkvtoolnix-gui/forms/**/*.ui"].to_a
  $gui_ui_h_files          = $gui_ui_files.collect { |file| file.ext 'h' }

  $top_srcdir              = c(:top_srcdir)
  $dependency_dir          = "#{$top_srcdir}/rake.d/dependency.d"
  $dependency_tmp_dir      = "#{$dependency_dir}/tmp"

  $languages               =  {
    :programs              => c(:TRANSLATIONS).split(/\s+/),
    :manpages              => c(:MANPAGES_TRANSLATIONS).split(/\s+/),
  }

  $translations            =  {
    :programs              =>                         $languages[:programs].collect { |language| "po/#{language}.mo" },
    :manpages              => !c?(:PO4A_WORKS) ? [] : $languages[:manpages].collect { |language| $manpages.collect { |manpage| manpage.gsub(/man\//, "man/#{language}/") } }.flatten,
  }

  $available_languages     =  {
    :programs              => FileList[ "#{$top_srcdir }/po/*.po"              ].collect { |name| File.basename name, '.po'        },
    :manpages              => FileList[ "#{$top_srcdir }/doc/man/po4a/po/*.po" ].collect { |name| File.basename name, '.po'        },
  }

  $unwrapped_po            = %{ca es eu it nl uk pl sr_RS@latin tr}
  $po_multiple_sources     = %{sv}

  $benchmark_sources       = FileList["src/benchmark/*.cpp"].to_a if c?(:GOOGLE_BENCHMARK)

  cflags_common            = "-Wall -Wno-comment -Wfatal-errors #{c(:WLOGICAL_OP)} #{c(:WNO_MISMATCHED_TAGS)} #{c(:WNO_SELF_ASSIGN)} #{c(:QUNUSED_ARGUMENTS)}"
  cflags_common           += " #{c(:WNO_INCONSISTENT_MISSING_OVERRIDE)} #{c(:WNO_POTENTIALLY_EVALUATED_EXPRESSION)}"
  cflags_common           += " #{c(:OPTIMIZATION_CFLAGS)} -D_FILE_OFFSET_BITS=64"
  cflags_common           += " -DMTX_LOCALE_DIR=\\\"#{c(:localedir)}\\\" -DMTX_PKG_DATA_DIR=\\\"#{c(:pkgdatadir)}\\\" -DMTX_DOC_DIR=\\\"#{c(:docdir)}\\\""
  cflags_common           += " #{c(:FSTACK_PROTECTOR)}"
  cflags_common           += " -fsanitize=undefined"                                     if c?(:UBSAN)
  cflags_common           += " -fsanitize=address -fno-omit-frame-pointer"               if c?(:ADDRSAN)
  cflags_common           += " -Ilib/libebml -Ilib/libmatroska"                          if c?(:EBML_MATROSKA_INTERNAL)
  cflags_common           += " #{c(:MATROSKA_CFLAGS)} #{c(:EBML_CFLAGS)} #{c(:PUGIXML_CFLAGS)} #{c(:EXTRA_CFLAGS)} #{c(:DEBUG_CFLAGS)} #{c(:PROFILING_CFLAGS)} #{c(:USER_CPPFLAGS)}"
  cflags_common           += " -mno-ms-bitfields -DWINVER=0x0601 -D_WIN32_WINNT=0x0601 " if $building_for[:windows] # 0x0601 = Windows 7/Server 2008 R2
  cflags_common           += " -march=i686"                                              if $building_for[:windows] && /i686/.match(c(:host))
  cflags_common           += " -fPIC "                                                   if c?(:USE_QT) && !$building_for[:windows]
  cflags_common           += " -DQT_STATICPLUGIN"                                        if c?(:USE_QT) &&  $building_for[:windows]
  cflags_common           += " -DMTX_APPIMAGE"                                           if c?(:APPIMAGE_BUILD) && !$building_for[:windows]

  cflags                   = "#{cflags_common} #{c(:USER_CFLAGS)}"

  cxxflags                 = "#{cflags_common} #{c(:STD_CXX)}"
  cxxflags                += " -Wnon-virtual-dtor -Woverloaded-virtual -Wextra -Wno-missing-field-initializers #{c(:WNO_MAYBE_UNINITIALIZED)}"
  cxxflags                += " #{c(:QT_CFLAGS)} #{c(:BOOST_CPPFLAGS)} #{c(:USER_CXXFLAGS)}"

  ldflags                  = ""
  ldflags                 += " -fuse-ld=lld"                            if (c(:COMPILER_TYPE) == "clang") && !c(:LLVM_LLD).empty?
  ldflags                 += " -Llib/libebml/src -Llib/libmatroska/src" if c?(:EBML_MATROSKA_INTERNAL)
  ldflags                 += " #{c(:EXTRA_LDFLAGS)} #{c(:PROFILING_LIBS)} #{c(:USER_LDFLAGS)} #{c(:LDFLAGS_RPATHS)} #{c(:BOOST_LDFLAGS)}"
  ldflags                 += " -Wl,--dynamicbase,--nxcompat"               if $building_for[:windows]
  ldflags                 += " -fsanitize=undefined"                       if c?(:UBSAN)
  ldflags                 += " -fsanitize=address -fno-omit-frame-pointer" if c?(:ADDRSAN)
  ldflags                 += " #{c(:FSTACK_PROTECTOR)}"

  windres                  = ""
  windres                 += " -DMINGW_PROCESSOR_ARCH_AMD64=1" if c(:MINGW_PROCESSOR_ARCH) == 'amd64'

  mocflags                 = $building_for[:macos] ? "-DSYS_APPLE" : $building_for[:windows] ? "-DSYS_WINDOWS" : ""

  $flags                   = {
    :cflags                => cflags,
    :cxxflags              => cxxflags,
    :ldflags               => ldflags,
    :windres               => windres,
    :moc                   => mocflags,
  }

  setup_macos_specifics if $building_for[:macos]

  $build_system_modules.values.each { |bsm| bsm[:setup].call if bsm[:setup] }
end

def setup_overrides
  [ :programs, :manpages ].each do |type|
    value                      = c("AVAILABLE_LANGUAGES_#{type.to_s.upcase}")
    $available_languages[type] = value.split(/\s+/) unless value.empty?
  end
end

def setup_macos_specifics
  $macos_config = read_config_file("tools/macos/config.sh")

  if ENV['MACOSX_DEPLOYMENT_TARGET'].to_s.empty? && !$macos_config[:MACOSX_DEPLOYMENT_TARGET].to_s.empty?
    ENV['MACOSX_DEPLOYMENT_TARGET'] = $macos_config[:MACOSX_DEPLOYMENT_TARGET]
  end
end

def define_default_task
  desc "Build everything"

  # The applications themselves
  targets = $applications.clone

  targets << "apps:tools" if $build_tools
  targets += (c(:ADDITIONAL_TARGETS) || '').split(%r{ +})

  # Build the unit tests only if requested
  targets << ($run_unit_tests ? 'tests:run_unit' : 'tests:unit') if $have_gtest

  # The tags file -- but only if it exists already
  if File.exists?("TAGS")
    targets << "TAGS"   if !c(:ETAGS).empty?
    targets << "BROWSE" if !c(:EBROWSE).empty?
  end

  # Build developer documentation?
  targets << "doc/development.html" if !c(:PANDOC).empty?

  # Build man pages and translations?
  targets << "manpages"
  targets << "translations:manpages" if c?(:PO4A_WORKS)

  # Build translations for the programs
  targets << "translations:programs"

  # The Qt translation files: only for Windows
  targets << "translations:qt" if $building_for[:windows] && !c(:LCONVERT).blank?

  # Build ebml_validator by default when not cross-compiling as it is
  # needed for running the tests.
  targets << "apps:tools:ebml_validator" if c(:host) == c(:build)

  targets << "src/benchmark/benchmark" if c?(:GOOGLE_BENCHMARK) && !$benchmark_sources.empty?

  task :default => targets do
    puts "Done. Enjoy :)"
  end
end

# main
setup_globals
setup_overrides
import_dependencies

# Default task
define_default_task

# A helper task to check if there are any unstaged changes.
task :no_unstaged_changes do
  has_changes = false
  has_changes = true if !system("git rev-parse --verify HEAD &> /dev/null")                                 || $?.exitstatus != 0
  has_changes = true if !system("git update-index -q --ignore-submodules --refresh &> /dev/null")           || $?.exitstatus != 0
  has_changes = true if !system("git diff-files --quiet --ignore-submodules &> /dev/null")                  || $?.exitstatus != 0
  has_changes = true if !system("git diff-index --cached --quiet --ignore-submodules HEAD -- &> /dev/null") || $?.exitstatus != 0

  fail "There are unstaged changes; the operation cannot continue." if has_changes
end

desc "Build all applications"
task :apps => $applications

desc "Build all command line applications"
namespace :apps do
  task :cli => %w{apps:mkvmerge apps:mkvinfo apps:mkvextract apps:mkvpropedit}

  desc "Strip all apps"
  task :strip => $applications do
    runq "strip", nil, "#{c(:STRIP)} #{$applications.join(' ')}"
  end
end

# Store compiler block for re-use
cxx_compiler = lambda do |*args|
  t = args.first

  create_dependency_dirs

  source = t.source ? t.source : t.prerequisites.first
  dep    = dependency_output_name_for t.name
  pchi   = PCH.info_for_user(source, t.name)
  pchu   = pchi.use_flags ? " #{pchi.use_flags}" : ""
  pchx   = pchi.extra_flags ? " #{pchi.extra_flags}" : ""
  lang   = pchi.language ? pchi.language : "c++"

  args = [
    "cxx", source,
    "#{c(:CXX)} #{$flags[:cxxflags]}#{pchu}#{pchx} #{$system_includes} -c -MMD -MF #{dep} -o #{t.name} -x #{lang} #{source}",
    :allow_failure => true
  ]

  if pchi.pretty_flags
    PCH.runq(*args[0..2], args[3].merge(pchi.pretty_flags))
  else
    runq(*args)
  end

  handle_deps t.name, last_exit_code, true
end

# Pattern rules
rule '.o' => '.cpp', &cxx_compiler
rule '.o' => '.cc',  &cxx_compiler

rule '.o' => '.c' do |t|
  create_dependency_dirs
  dep = dependency_output_name_for t.name

  runq "cc", t.source, "#{c(:CC)} #{$flags[:cflags]} #{$system_includes} -c -MMD -MF #{dep} -o #{t.name} #{t.sources.join(" ")}", :allow_failure => true
  handle_deps t.name, last_exit_code
end

rule '.o' => '.rc' do |t|
  runq "windres", t.source, "#{c(:WINDRES)} #{$flags[:windres]} -o #{t.name} #{t.sources.join(" ")}"
end

# Resources depend on the manifest.xml file for Windows builds.
if $building_for[:windows]
  $programs.each do |program|
    path = program.gsub(/^mkv/, '')
    icon = program == 'mkvinfo' ? 'share/icons/windows/mkvinfo.ico' : 'share/icons/windows/mkvtoolnix-gui.ico'
    file "src/#{path}/resources.o" => [ "src/#{path}/manifest-#{c(:MINGW_PROCESSOR_ARCH)}.xml", "src/#{path}/resources.rc", icon ]
  end
end

rule '.mo' => '.po' do |t|
  runq "msgfmt", t.source, "msgfmt -c -o #{t.name} #{t.sources.join(" ")}"
end

if !c(:LCONVERT).blank?
  rule '.qm' => '.ts' do |t|
    runq "lconvert", t.source, "#{c(:LCONVERT)} -o #{t.name} -i #{t.sources.join(" ")}"
  end
end

# man pages from DocBook XML
rule '.1' => '.xml' do |t|
  filter = lambda do |code, lines|
    if (0 == code) && lines.any? { |line| /^error|parser error/i.match(line) }
      File.unlink(t.name) if File.exists?(t.name)
      result = 1
      puts lines.join('')

    elsif 0 != code
      result = code
      puts lines.join('')

    else
      result = 0
      puts lines.join('') if $verbose
    end

    result
  end

  stylesheet = "#{c(:DOCBOOK_ROOT)}/manpages/docbook.xsl"

  if !FileTest.exists?(stylesheet)
    puts "Error: the DocBook stylesheet '#{stylesheet}' does not exist."
    exit 1
  end

  runq "xsltproc", t.source, "#{c(:XSLTPROC)} #{c(:XSLTPROC_FLAGS)} -o #{t.name} #{stylesheet} #{t.sources.join(" ")}", :filter_output => filter
end

$manpages.each do |manpage|
  file manpage => manpage.ext('xml')
  $available_languages[:manpages].each do |language|
    localized_manpage = manpage.gsub(/.*\//, "doc/man/#{language}/")
    file localized_manpage => localized_manpage.ext('xml')
  end
end

# Qt files
rule '.h' => '.ui' do |t|
  runq "uic", t.source, "#{c(:UIC)} --translate QTR #{t.sources.join(" ")} > #{t.name}"
end

rule '.cpp' => '.qrc' do |t|
  runq "rcc", t.source, "#{c(:RCC)} #{t.sources.join(" ")} > #{t.name}"
end

rule '.moc' => '.h' do |t|
  runq "moc", t.source, "#{c(:MOC)} #{c(:QT_CFLAGS)} #{$system_includes} #{$flags[:moc]} -nw #{t.source} > #{t.name}"
end

rule '.moco' => '.moc' do |t|
  cxx_compiler.call t
end

# Tag files
desc "Create tags file for Emacs"
task :tags => "TAGS"

desc "Create browse file for Emacs"
task :browse => "BROWSE"

file "TAGS" => $all_sources do |t|
  runq 'etags', nil, "#{c(:ETAGS)} -o #{t.name} #{t.prerequisites.join(" ")}"
end

file "BROWSE" => ($all_sources + $all_headers) do |t|
  runq 'ebrowse', nil, "#{c(:EBROWSE)} -o #{t.name} #{t.prerequisites.join(" ")}"
end

file "doc/development.html" => [ "doc/development.md", "doc/pandoc-template.html" ] do |t|
  runq "pandoc", t.prerequisites.first, <<COMMAND
    #{c(:PANDOC)} -o #{t.name} --standalone --from markdown_strict --to html --number-sections --table-of-contents --css=pandoc.css --template=doc/pandoc-template.html doc/development.md
COMMAND
end

file "po/mkvtoolnix.pot" => $all_sources + $all_headers + $gui_ui_h_files + %w{Rakefile} do |t|
  sources   = (t.prerequisites.dup - %w{Rakefile}).sort.uniq

  keywords  = %w{--keyword=Y --keyword=NY:1,2}   # singular & plural forms returning std::string
  keywords += %w{--keyword=YF --keyword=NYF:1,2} # singular & plural forms returning std::string which aren't format strings
  keywords += %w{--keyword=YT}                   # singular form returning translatable_string_c
  keywords += %w{--keyword=QTR}                  # singular form returning QString, used by uic
  keywords += %w{--keyword=QY --keyword=QNY:1,2} # singular & plural forms returning QString
  keywords += %w{--keyword=QYH}                  # singular form returning HTML-escaped QString

  flags     = %w{--flag=QY:1:no-c-format --flag=QNY:1:no-c-format}
  flags    += %w{--flag=YF:1:no-c-format --flag=YF:1:no-boost-format}
  flags    += %w{--flag=NYF:1:no-c-format --flag=NYF:1:no-boost-format}

  options   = %w{--default-domain=mkvtoolnix --from-code=UTF-8 --sort-output --boost}
  options  += ["'--msgid-bugs-address=Moritz Bunkus <moritz@bunkus.org>'"]
  options  += ["'--copyright-holder=Moritz Bunkus <moritz@bunkus.org>'", "--package-name=MKVToolNix", "--package-version=#{c(:PACKAGE_VERSION)}", "--foreign-user"]

  runq "xgettext", t.name, "xgettext #{keywords.join(" ")} #{flags.join(" ")} #{options.join(" ")} -o #{t.name} #{sources.join(" ")}"
end

task :manpages => $manpages

# Translations for the programs
namespace :translations do
  desc "Create a template for translating the programs"
  task :pot => "po/mkvtoolnix.pot"

  desc "Create a new .po file with an empty template"
  task "new-po" => "po/mkvtoolnix.pot" do
    %w{LANGUAGE EMAIL}.each { |e| fail "Variable '#{e}' is not set" if ENV[e].blank? }

    require 'rexml/document'
    iso639_file = "/usr/share/xml/iso-codes/iso_639.xml"
    node        = REXML::XPath.first REXML::Document.new(File.new(iso639_file)), "//iso_639_entry[@name='#{ENV['LANGUAGE']}']"
    locale      = node ? node.attributes['iso_639_1_code'] : nil
    if locale.blank?
      if /^ [a-z]{2} (?: _ [A-Z]{2} )? $/x.match(ENV['LANGUAGE'])
        locale = ENV['LANGUAGE']
      else
        fail "Unknown language/ISO-639-1 code not found in #{iso639_file}"
      end
    end

    puts_action "create", "po/#{locale}.po"
    File.open "po/#{locale}.po", "w" do |out|
      now           = Time.now
      email         = ENV['EMAIL']
      email         = "YOUR NAME <#{email}>" unless /</.match(email)
      header        = <<EOT
# translation of mkvtoolnix.pot to #{ENV['LANGUAGE']}
# Copyright (C) #{now.year} Moritz Bunkus
# This file is distributed under the same license as the mkvtoolnix package.
#
msgid ""
EOT

      content = IO.
        readlines("po/mkvtoolnix.pot").
        join("").
        gsub(/\A.*?msgid ""\n/m, header).
        gsub(/^"PO-Revision-Date:.*?$/m, %{"PO-Revision-Date: #{now.strftime('%Y-%m-%d %H:%M%z')}\\n"}).
        gsub(/^"Last-Translator:.*?$/m,  %{"Last-Translator: #{email}\\n"}).
        gsub(/^"Language-Team:.*?$/m,    %{"Language-Team: #{ENV['LANGUAGE']} <moritz@bunkus.org>\\n"}).
        gsub(/^"Language: \\n"$/,        %{"Language: #{locale}\\n"})

      out.puts content
    end
  end

  def verify_format_strings languages
    is_ok = true

    languages.
      collect { |language| "po/#{language}.po" }.
      sort.
      each do |file_name|
      puts "VERIFY #{file_name}"
      is_ok &&= FormatStringVerifier.new.verify file_name
    end

    is_ok
  end

  desc "Verify format strings in translations"
  task "verify-format-strings" do
    languages = (ENV['LANGUAGES'] || '').split(/ +/)
    languages = $available_languages[:programs] if languages.empty?

    exit 1 if !verify_format_strings(languages)
  end

  desc "Verify format strings in staged translations"
  task "verify-format-strings-staged" do
    languages = `git --no-pager diff --cached --name-only`.
      split(/\n/).
      select { |file| /po\/.*\.po$/.match(file) }.
      map    { |file| file.gsub(/.*\//, '').gsub(/\.po/, '') }

    if languages.empty?
      puts "Error: Nothing staged"
      exit 2
    end


    exit 1 if !verify_format_strings(languages)
  end

  $all_po_files.each do |po_file|
    task "normalize-po-#{po_file}" do
      normalize_po po_file
    end
  end

  desc "Normalize all .po files to a standard format"
  task "normalize-po" => $all_po_files.map { |e| "translations:normalize-po-#{e}" }

  [ :programs, :manpages ].each { |type| task type => $translations[type] }

  task :qt => FileList[ "#{$top_srcdir }/po/qt/*.ts" ].collect { |file| file.ext 'qm' }

  if c?(:PO4A_WORKS)
    filter = lambda do |code, lines|
      lines = lines.reject { |l| %r{seems outdated.*differ between|please consider running po4a-updatepo|^$}i.match(l) }
      puts lines.join('') unless lines.empty?
      code
    end

    $available_languages[:manpages].each do |language|
      $manpages.each do |manpage|
        name = manpage.gsub(/man\//, "man/#{language}/")
        file name            => [ name.ext('xml'),     "doc/man/po4a/po/#{language}.po" ]
        file name.ext('xml') => [ manpage.ext('.xml'), "doc/man/po4a/po/#{language}.po" ] do |t|
          runq "po4a", "#{manpage.ext('.xml')} (#{language})", "#{c(:PO4A_TRANSLATE)} #{c(:PO4A_TRANSLATE_FLAGS)} -m #{manpage.ext('.xml')} -p doc/man/po4a/po/#{language}.po -l #{t.name}", :filter_output => filter
        end
      end
    end
  end

  desc "Update all translation files"
  task :update => [ "translations:update:programs", "translations:update:manpages", "translations:update:translations" ]

  namespace :update do
    desc "Update the program's translation files"
    task :programs => [ "po/mkvtoolnix.pot", ] + $available_languages[:programs].collect { |language| "translations:update:programs:#{language}" }

    namespace :programs do
      $available_languages[:programs].each do |language|
        task language => "po/mkvtoolnix.pot" do |t|
          po       = "po/#{language}.po"
          tmp_file = "#{po}.new"
          no_wrap  = $unwrapped_po.include?(language) ? "" : "--no-wrap"
          runq "msgmerge", po, "msgmerge -q -s #{no_wrap} #{po} po/mkvtoolnix.pot > #{tmp_file}", :allow_failure => true

          exit_code = last_exit_code
          if 0 != exit_code
            File.unlink tmp_file
            exit exit_code
          end

          FileUtils.move tmp_file, po

          normalize_po po
        end
      end
    end

    if c?(:PO4A_WORKS)
      desc "Update the man pages' translation files"
      task :manpages do
        runq "po4a", "doc/man/po4a/po4a.cfg", "#{c(:PO4A)} #{c(:PO4A_FLAGS)} --msgmerge-opt=--no-wrap doc/man/po4a/po4a.cfg"
        $all_man_po_files.each do |po_file|
          normalize_po po_file
        end
      end
    end

    desc "Update the Windows installer's translation files"
    task :installer do |t|
      Mtx::Installer.update_all_translation_files
    end
  end

  namespace :transifex do
    transifex_pull_targets = {
      "man-pages" => [],
      "programs"  => [],
    }

    transifex_push_targets = {
      "man-pages" => [],
      "programs"  => [],
    }

    $all_po_files.each do |po_file|
      language    = /\/([^\/]+)\.po$/.match(po_file)[1]
      resource    = /doc\/man/.match(po_file) ? "man-pages" : "programs"
      pull_target = "pull-#{resource}-#{language}"
      push_target = "push-#{resource}-#{language}"

      transifex_pull_targets[resource] << "translations:transifex:#{pull_target}"
      transifex_push_targets[resource] << "translations:transifex:#{push_target}"

      desc "Fetch and merge #{resource} translations from Transifex (#{language})" if list_targets?("transifex", "transifex_pull")
      task pull_target => :no_unstaged_changes do
        transifex_pull_and_merge resource, language
      end

      desc "Push #{resource} translations to Transifex (#{language})" if list_targets?("transifex", "transifex_push")
      task push_target => :no_unstaged_changes do
        transifex_remove_fuzzy_and_push resource, language
      end
    end

    desc "Fetch and merge program translations from Transifex"
    task "pull-programs" => transifex_pull_targets["programs"]

    desc "Fetch and merge man page translations from Transifex"
    task "pull-man-pages" => transifex_pull_targets["man-pages"]

    desc "Fetch and merge all translations from Transifex"
    task "pull" => transifex_pull_targets.values.flatten

    desc "Push program translation source file to Transifex"
    task "push-programs-source" => "po/mkvtoolnix.pot" do
      runq "tx push", "po/mkvtoolnix.pot", "tx push -s -r mkvtoolnix.programs > /dev/null"
    end

    desc "Push man page translation source file to Transifex"
    task "push-man-pages-source" => "doc/man/po4a/po/mkvtoolnix.pot" do
      runq "tx push", "doc/man/po4a/po/mkvtoolnix.pot", "tx push -s -r mkvtoolnix.man-pages > /dev/null"
    end
    desc "Push program translations to Transifex"
    task "push-programs" => transifex_push_targets["programs"]

    desc "Push man page translations to Transifex"
    task "push-man-pages" => transifex_push_targets["man-pages"]

    desc "Push all translations to Transifex"
    task "push" => transifex_push_targets.values.flatten
  end

  [ :stats, :statistics ].each_with_index do |task_name, idx|
    desc "Generate statistics about translation coverage" if 0 == idx
    task task_name do
      FileList["po/*.po", "doc/man/po4a/po/*.po"].each do |name|
        command = "msgfmt --statistics -o /dev/null #{name} 2>&1"
        if $verbose
          runq "msgfmt", name, command, :allow_failure => true
        else
          puts "#{name} : " + `#{command}`.split(/\n/).first
        end
      end
    end
  end
end

# HTML generation for the man pages
targets = ([ 'en' ] + $languages[:manpages]).collect do |language|
  dir = language == 'en' ? '' : "/#{language}"
  FileList[ "doc/man#{dir}/*.xml" ].collect { |name| "man2html:#{language}:#{File.basename(name, '.xml')}" }
end.flatten

%w{manpages-html man2html}.each_with_index do |task_name, idx|
  desc "Create HTML files for the man pages" if 0 == idx
  task task_name => targets
end

namespace :man2html do
  ([ 'en' ] + $languages[:manpages]).collect do |language|
    namespace language do
      dir  = language == 'en' ? '' : "/#{language}"
      xml  = FileList[ "doc/man#{dir}/*.xml" ].to_a
      html = xml.map { |name| name.ext(".html") }

      xml.each do |name|
        file name.ext('html') => %w{manpages translations:manpages} do
          runq "saxon-he", name, "java -classpath lib/saxon-he/saxon9he.jar net.sf.saxon.Transform -o:#{name.ext('html')} -xsl:doc/stylesheets/docbook-to-html.xsl #{name}"
        end

        task File.basename(name, '.xml') => name.ext('html')
      end

      task :all => html
    end
  end
end

# Developer helper tasks
namespace :dev do
  if $build_mkvtoolnix_gui
    desc "Update Qt resource files"
    task "update-qt-resources" do
      require 'rexml/document'

      qrc      = "src/mkvtoolnix-gui/qt_resources.qrc"
      doc      = REXML::Document.new File.new(qrc)
      existing = Hash.new

      doc.elements.to_a("/RCC/qresource/file").each do |node|
        if File.exists? "src/mkvtoolnix-gui/#{node.text}"
          existing[node.text] = true
        else
          puts "Removing <file> for non-existing #{node.text}"
          node.remove
        end
      end

      parent = doc.elements.to_a("/RCC/qresource")[0]
      FileList["share/icons/*/*.png"].select { |file| !existing["../../#{file}"] }.each do |file|
        puts "Adding <file> for #{file}"
        node                     = REXML::Element.new "file"
        node.attributes["alias"] = file.gsub(/share\//, '')
        node.text                = "../../#{file}"
        parent << node
      end

      formatter         = REXML::Formatters::Pretty.new 1
      formatter.compact = true
      formatter.width   = 9999999
      formatter.write doc, File.open(qrc, "w")
    end

    desc "Update Qt project file"
    task "update-qt-project" do
      project_file = "src/mkvtoolnix-gui/mkvtoolnix-gui.pro"
      content      = []
      skipping     = false
      IO.readlines(project_file).collect { |line| line.chomp }.each do |line|
        content << line unless skipping

        if /^FORMS\b/.match line
          skipping  = true
          content  << $gui_ui_files.collect { |ui| ui.gsub!(/.*forms\//, 'forms/'); "    #{ui}" }.sort.join(" \\\n")

        elsif skipping && /^\s*$/.match(line)
          skipping  = false
          content  << line
        end
      end

      File.open(project_file, "w") do |file|
        file.puts content.join("\n")
      end
    end
  end

  desc "Create source code tarball from current version in .."
  task :tarball do
    create_source_tarball
  end

  desc "Create source code tarball from current version in .. with git revision in name"
  task "tarball-rev" do
    revision = `git rev-parse --short HEAD`.chomp
    create_source_tarball "-#{revision}"
  end
end

# Installation tasks
desc "Install all applications and support files"
targets  = [ "install:programs", "install:manpages", "install:translations:programs" ]
targets += [ "install:translations:manpages" ] if c?(:PO4A_WORKS)
targets += [ "install:shared" ]                if c?(:USE_QT)
task :install => targets

namespace :install do
  application_name_mapper = lambda do |name|
    File.basename name
  end

  task :programs => $applications do
    install_dir :bindir
    $applications.each { |application| install_program "#{c(:bindir)}/#{application_name_mapper[application]}", application }
  end

  task :shared do
    install_dir :desktopdir, :mimepackagesdir
    install_data :mimepackagesdir, FileList[ "#{$top_srcdir}/share/mime/*.xml" ]
    if c?(:USE_QT)
      install_data :desktopdir, "#{$top_srcdir}/share/desktop/org.bunkus.mkvinfo.desktop"
      install_data :desktopdir, "#{$top_srcdir}/share/desktop/org.bunkus.mkvtoolnix-gui.desktop"
    end

    wanted_apps     = %w{mkvmerge mkvtoolnix-gui mkvinfo mkvextract mkvpropedit}.collect { |e| "#{e}.png" }.to_hash_by
    wanted_dirs     = %w{16x16 24x24 32x32 48x48 64x64 96x96 128x128 256x256}.to_hash_by
    dirs_to_install = FileList[ "#{$top_srcdir}/share/icons/*"   ].select { |dir|  wanted_dirs[ dir.gsub(/.*icons\//, '').gsub(/\/.*/, '') ] }.sort.uniq

    dirs_to_install.each do |dir|
      dest_dir = "#{c(:icondir)}/#{dir.gsub(/.*icons\//, '')}/apps"
      install_dir dest_dir
      install_data "#{dest_dir}/", FileList[ "#{dir}/*" ].to_a.select { |file| wanted_apps[ file.gsub(/.*\//, '') ] }
    end

    if c?(:USE_QT)
      sounds_dir ="#{c(:pkgdatadir)}/sounds"

      install_dir  sounds_dir
      install_data sounds_dir, FileList["#{$top_srcdir}/share/sounds/*"]
    end
  end

  man_page_name_mapper = lambda do |name|
    File.basename name
  end

  task :manpages => $manpages do
    install_dir :man1dir
    $manpages.each { |manpage| install_data "#{c(:man1dir)}/#{man_page_name_mapper[manpage]}", manpage }
  end

  namespace :translations do
    task :programs => $translations[:programs] do
      install_dir $languages[:programs].collect { |language| "#{c(:localedir)}/#{language}/LC_MESSAGES" }
      $languages[:programs].each do |language|
        install_data "#{c(:localedir)}/#{language}/LC_MESSAGES/mkvtoolnix.mo", "po/#{language}.mo"
      end
    end

    if c?(:PO4A_WORKS)
      task :manpages => $translations[:manpages] do
        install_dir $languages[:manpages].collect { |language| "#{c(:mandir)}/#{language}/man1" }
        $languages[:manpages].each do |language|
          $manpages.each { |manpage| install_data "#{c(:mandir)}/#{language}/man1/#{man_page_name_mapper[manpage]}", manpage.sub(/man\//, "man/#{language}/") }
        end
      end
    end
  end
end

# Cleaning tasks
desc "Remove all compiled files"
task :clean do
  puts_action "clean"

  patterns = %w{
    **/*.a
    **/*.autosave
    **/*.deps
    **/*.dll
    **/*.exe
    **/*.exe
    **/*.mo
    **/*.moc
    **/*.moco
    **/*.o
    **/*.pot
    **/*.qm
    doc/man/*.1
    doc/man/*.html
    doc/man/*/*.1
    doc/man/*/*.html
    doc/man/*/*.xml
    src/*/qt_resources.cpp
    src/info/ui/*.h
    src/mkvtoolnix-gui/forms/**/*.h
    src/benchmark/benchmark
    tests/unit/all
    tests/unit/merge/merge
    tests/unit/propedit/propedit
  }
  patterns += $applications + $tools.collect { |name| "src/tools/#{name}" }
  patterns += PCH.clean_patterns

  remove_files_by_patterns patterns

  if Dir.exists? $dependency_dir
    puts_vaction "rm -rf", "#{$dependency_dir}"
    FileUtils.rm_rf $dependency_dir
  end
end

namespace :clean do
  desc "Remove all compiled and generated files ('tarball' clean)"
  task :dist => :clean do
    run "rm -f config.h config.log config.cache build-config TAGS src/info/static_plugins.cpp src/mkvtoolnix-gui/static_plugins.cpp", :allow_failure => true
  end

  desc "Remove all compiled and generated files ('git' clean)"
  task :maintainer => "clean:dist" do
    run "rm -f configure config.h.in", :allow_failure => true
  end

  desc "Remove compiled objects and programs in the unit test suite"
  task :unittests do
    patterns  = %w{tests/unit/*.o tests/unit/*/*.o tests/unit/*.a tests/unit/*/*.a}
    patterns += $gtest_apps.collect { |app| "tests/unit/#{app}/#{app}" }
    remove_files_by_patterns patterns
  end
end

# Tests
desc "Run all tests"
task :tests => [ 'tests:products' ]
namespace :tests do
  desc "Run product tests from 'tests' sub-directory (requires data files to be present)"
  task :products do
    run "cd tests && ./run.rb"
  end

  desc "Run built-in tests on source code files"
  task :source do
    Mtx::SourceTests.test_include_guards
  end
end

#
# avilib-0.6.10
# librmff
# spyder's MPEG parser
# src/common
# src/input
# src/output
#

[ { :name => 'avi',         :dir => 'lib/avilib-0.6.10'                                                              },
  { :name => 'rmff',        :dir => 'lib/librmff'                                                                    },
  { :name => 'pugixml',     :dir => 'lib/pugixml/src'                                                                },
  { :name => 'mpegparser',  :dir => 'src/mpegparser'                                                                 },
  { :name => 'mtxcommon',   :dir => [ 'src/common' ] + FileList['src/common/*'].select { |e| FileTest.directory? e } },
  { :name => 'mtxinput',    :dir => 'src/input'                                                                      },
  { :name => 'mtxoutput',   :dir => 'src/output'                                                                     },
  { :name => 'mtxmerge',    :dir => 'src/merge',    :except => [ 'mkvmerge.cpp' ],                                   },
  { :name => 'mtxinfo',     :dir => 'src/info',     :except => %w{qt_ui.cpp  mkvinfo.cpp mkvinfo-gui.cpp static_plugins.cpp}, },
  { :name => 'mtxextract',  :dir => 'src/extract',  :except => [ 'mkvextract.cpp' ],                                 },
  { :name => 'mtxpropedit', :dir => 'src/propedit', :except => [ 'mkvpropedit.cpp' ],                                },
  { :name => 'ebml',        :dir => 'lib/libebml/src'                                                                },
  { :name => 'matroska',    :dir => 'lib/libmatroska/src'                                                            },
].each do |lib|
  Library.
    new("#{[ lib[:dir] ].flatten.first}/lib#{lib[:name]}").
    sources([ lib[:dir] ].flatten, :type => :dir, :except => lib[:except]).
    build_dll(lib[:name] == 'mtxcommon').
    libraries(:iconv, :z, :matroska, :ebml, :rpcrt4).
    create
end

# libraries required for all programs via mtxcommon
$common_libs = [
  :mtxcommon,
  :magic,
  :matroska,
  :ebml,
  :z,
  :pugixml,
  :intl,
  :iconv,
  :boost_regex,
  :boost_filesystem,
  :boost_system,
]

# custom libraries
$custom_libs = [
  :static,
]

#
# mkvmerge
#

Application.new("src/mkvmerge").
  description("Build the mkvmerge executable").
  aliases(:mkvmerge).
  sources("src/merge/mkvmerge.cpp").
  sources("src/merge/resources.o", :if => $building_for[:windows]).
  libraries(:mtxmerge, :mtxinput, :mtxoutput, :mtxmerge, $common_libs, :avi, :rmff, :mpegparser, :flac, :vorbis, :ogg, $custom_libs).
  create

#
# mkvinfo
#

$mkvinfo_ui_files   = FileList["src/info/ui/*.ui"].to_a
$mkvinfo_ui_h_files = $mkvinfo_ui_files.collect { |file| file.ext('h') }

%w{qt_ui.o qt_ui.moc}.each do |name|
  file "src/info/#{name}" => $mkvinfo_ui_h_files
end

Application.new("src/mkvinfo").
  description("Build the mkvinfo executable").
  aliases(:mkvinfo).
  sources("src/info/mkvinfo.cpp").
  sources("src/info/resources.o", :if => $building_for[:windows]).
  libraries(:mtxinfo, $common_libs).
  only_if(c?(:USE_QT)).
  sources("src/info/sys_windows.o", :if => $building_for[:windows]).
  sources("src/info/qt_ui.cpp", "src/info/qt_ui.moc", "src/info/rightclick_tree_widget.moc", $mkvinfo_ui_files).
  sources('src/info/qt_resources.cpp').
  sources('src/info/static_plugins.cpp', :if => File.exist?('src/info/static_plugins.cpp')).
  libraries(:qt).
  end_if.
  libraries($custom_libs).
  create

if $build_mkvinfo_gui
  Application.new("src/mkvinfo-gui").
    description("Build the mkvinfo-gui executable").
    aliases("mkvinfo-gui").
    sources("src/info/mkvinfo-gui.cpp").
    sources("src/info/resources.o", :if => $building_for[:windows]).
    sources('src/info/static_plugins.cpp', :if => File.exist?('src/info/static_plugins.cpp')).
    libraries(:qt, $custom_libs).
    libraries("-mwindows", :if => $building_for[:windows]).
    create
end

#
# mkvextract
#

Application.new("src/mkvextract").
  description("Build the mkvextract executable").
  aliases(:mkvextract).
  sources("src/extract/mkvextract.cpp").
  sources("src/extract/resources.o", :if => $building_for[:windows]).
  libraries(:mtxextract, $common_libs, :avi, :rmff, :vorbis, :ogg, $custom_libs).
  create

#
# mkvpropedit
#

Application.new("src/mkvpropedit").
  description("Build the mkvpropedit executable").
  aliases(:mkvpropedit).
  sources("src/propedit/propedit.cpp").
  sources("src/propedit/resources.o", :if => $building_for[:windows]).
  libraries(:mtxpropedit, $common_libs, $custom_libs).
  create

#
# mkvtoolnix-gui
#

if $build_mkvtoolnix_gui
  ui_h_files      = $gui_ui_files.collect { |ui| ui.ext 'h' }
  cpp_files       = FileList['src/mkvtoolnix-gui/**/*.cpp'].to_a.for_target!
  h_files         = FileList['src/mkvtoolnix-gui/**/*.h'].to_a.for_target! - ui_h_files
  cpp_content     = read_files cpp_files
  h_content       = read_files h_files

  qobject_h_files = h_files.select { |h| h_content[h].any? { |line| /\bQ_OBJECT\b/.match line } }

  form_include_re = %r{^\s* \# \s* include \s+ \" (mkvtoolnix-gui/forms/[^\"]+) }x
  dependencies    = {}

  cpp_files.each do |cpp|
    cpp_content[cpp].each do |line|
      next unless form_include_re.match line

      dependencies[cpp] ||= []
      dependencies[cpp]  << "src/#{$1}"
    end
  end

  dependencies.each { |cpp, ui_hs| file cpp.ext('o') => ui_hs }

  Application.new("src/mkvtoolnix-gui/mkvtoolnix-gui").
    description("Build the mkvtoolnix-gui executable").
    aliases("mkvtoolnix-gui").
    sources(qobject_h_files.collect { |h| h.ext 'moc' }).
    sources(cpp_files, $gui_ui_files, 'src/mkvtoolnix-gui/qt_resources.cpp').
    sources("src/mkvtoolnix-gui/resources.o", :if => $building_for[:windows]).
    libraries($common_libs, :qt).
    libraries("-mwindows", :powrprof, :if => $building_for[:windows]).
    libraries($custom_libs).
    create
end

#
# benchmark
#

if c?(:GOOGLE_BENCHMARK) && !$benchmark_sources.empty?
  Application.new("src/benchmark/benchmark").
    description("Build the benchmark executable").
    aliases(:benchmark, :bench).
    sources($benchmark_sources).
    libraries($common_libs, :benchmark, :qt).
    create
end

#
# Applications in src/tools
#
namespace :apps do
  task :tools => $tools.collect { |name| "apps:tools:#{name}" }
end

#
# tools: ac3parser
#
Application.new("src/tools/ac3parser").
  description("Build the ac3parser executable").
  aliases("tools:ac3parser").
  sources("src/tools/ac3parser.cpp").
  libraries($common_libs).
  create

#
# tools: base64tool
#
Application.new("src/tools/base64tool").
  description("Build the base64tool executable").
  aliases("tools:base64tool").
  sources("src/tools/base64tool.cpp").
  libraries($common_libs).
  create

#
# tools: checksum
#
Application.new("src/tools/checksum").
  description("Build the checksum executable").
  aliases("tools:checksum").
  sources("src/tools/checksum.cpp").
  libraries($common_libs).
  create

#
# tools: diracparser
#
Application.new("src/tools/diracparser").
  description("Build the diracparser executable").
  aliases("tools:diracparser").
  sources("src/tools/diracparser.cpp").
  libraries($common_libs).
  create

#
# tools: ebml_validator
#
Application.new("src/tools/ebml_validator").
  description("Build the ebml_validator executable").
  aliases("tools:ebml_validator").
  sources("src/tools/ebml_validator.cpp", "src/tools/element_info.cpp").
  libraries($common_libs).
  create

#
# tools: hevc_dump
#
Application.new("src/tools/hevc_dump").
  description("Build the hevc_dump executable").
  aliases("tools:hevc_dump").
  sources("src/tools/hevc_dump.cpp").
  libraries($common_libs).
  create

#
# tools: hevcs_dump
#
Application.new("src/tools/hevcc_dump").
  description("Build the hevcc_dump executable").
  aliases("tools:hevcc_dump").
  sources("src/tools/hevcc_dump.cpp").
  libraries($common_libs).
  create

#
# tools: mpls_dump
#
Application.new("src/tools/mpls_dump").
  description("Build the mpls_dump executable").
  aliases("tools:mpls_dump").
  sources("src/tools/mpls_dump.cpp").
  libraries($common_libs).
  create

#
# tools: vc1parser
#
Application.new("src/tools/vc1parser").
  description("Build the vc1parser executable").
  aliases("tools:vc1parser").
  sources("src/tools/vc1parser.cpp").
  libraries($common_libs).
  create

# Engage pch system
PCH.engage(&cxx_compiler)

$build_system_modules.values.each { |bsm| bsm[:define_tasks].call if bsm[:define_tasks] }

# Local Variables:
# mode: ruby
# End:
