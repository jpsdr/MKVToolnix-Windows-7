$build_start = Time.now

if Signal.list.key?('ALRM')
  Signal.trap('ALRM') { |signo| }
end

version = RUBY_VERSION.gsub(/[^0-9\.]+/, "").split(/\./).collect(&:to_i)
version << 0 while version.size < 3
if (version[0] < 2) && (version[1] < 9)
  fail "Ruby 1.9.x or newer is required for building"
end

# Change to base directory before doing anything
$source_dir = File.absolute_path(File.dirname(__FILE__))
$build_dir  = $source_dir

if FileUtils.pwd != File.dirname(__FILE__)
  puts "Entering directory `#{$source_dir}'"
  Dir.chdir $source_dir
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

module Mtx
end

require_relative "rake.d/requirements"
require_relative "rake.d/extensions"
require_relative "rake.d/config"

$config               = read_build_config
$verbose              = ENV['V'].to_bool
$run_show_start_stop  = !$verbose && c?('RUN_SHOW_START_STOP')
$build_system_modules = {}
$have_gtest           = (c(:GTEST_TYPE) == "system") || (c(:GTEST_TYPE) == "internal")
$gtest_apps           = []

require_relative "rake.d/digest"
require_relative "rake.d/helpers"
require_relative "rake.d/target"
require_relative "rake.d/application"
require_relative "rake.d/iana_language_subtag_registry"
require_relative "rake.d/installer"
require_relative "rake.d/iso639"
require_relative "rake.d/iso3166"
require_relative "rake.d/iso15924"
require_relative "rake.d/library"
require_relative "rake.d/compilation_database"
require_relative "rake.d/format_string_verifier"
require_relative "rake.d/pch"
require_relative "rake.d/online_file"
require_relative "rake.d/po"
require_relative "rake.d/source_tests"
require_relative "rake.d/tarball"
require_relative 'rake.d/gtest' if $have_gtest

FileList["rake.d/local.d/*.rb"].each { |rb| require_relative rb }

def setup_globals
  $po4a_cfg                 = c(:PO4A).empty? ? nil : "doc/man/po4a/po4a.cfg"
  $po4a_stamp               = "doc/man/po4a/latest_po4a_run.stamp"
  $po4a_pot                 = "doc/man/po4a/po/mkvtoolnix.pot"
  $po4a_translated_programs = !$po4a_cfg ? [] : IO.readlines($po4a_cfg).
    select { |line| %r{^\[ *type *: *docbook *\]}.match(line) }.
    map    { |line| line.chomp.gsub(%r{.*\] *doc/man/|\.xml.*}, '') }

  $building_for = {
    :linux   => %r{linux}i.match(c(:host)),
    :macos   => %r{darwin}i.match(c(:host)),
    :windows => %r{mingw}i.match(c(:host)),
  }

  $build_mkvtoolnix      ||=  c?(:BUILD_MKVTOOLNIX)
  $build_mkvtoolnix_gui  ||=  c?(:BUILD_GUI)

  $programs                =  %w{mkvmerge mkvinfo mkvextract mkvpropedit}
  $programs                << "mkvtoolnix"     if $build_mkvtoolnix
  $programs                << "mkvtoolnix-gui" if $build_mkvtoolnix_gui
  $tools                   =  %w{ac3parser base64tool bluray_dump checksum diracparser dovic_dump dts_dump ebml_validator hevcc_dump pgs_dump vc1parser xyzvc_dump}

  $application_subdirs     =  { "mkvtoolnix-gui" => "mkvtoolnix-gui/", "mkvtoolnix" => "mkvtoolnix/" }
  $applications            =  $programs.map { |name| "src/#{$application_subdirs[name]}#{name}" + c(:EXEEXT) }
  $manpages                =  $programs.map { |name| "doc/man/#{name}.1" }

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

  $qrc                     = $build_mkvtoolnix_gui ? FileList[ "src/mkvtoolnix-gui/qt_resources.qrc" ].to_a : []
  $qt_resources            = $qrc.map { |file| file.ext('rcc') }

  $dependency_dir          = "#{$source_dir}/rake.d/dependency.d"
  $dependency_tmp_dir      = "#{$dependency_dir}/tmp"

  $version_header_name     = "#{$build_dir}/src/common/mkvtoolnix_version.h"

  $languages               =  {
    :programs              => c(:TRANSLATIONS).split(/\s+/),
    :manpages              => c(:MANPAGES_TRANSLATIONS).split(/\s+/),
  }

  $translations            =  {
    :programs              =>             $languages[:programs].collect { |language| "po/#{language}.mo" },
    :manpages              => $po4a_cfg ? $languages[:manpages].collect { |language| $programs.select { |program| $po4a_translated_programs.include?(program) }.map { |program| "doc/man/#{language}/#{program}.1" } }.flatten : [],
  }

  $available_languages     =  {
    :programs              => FileList[ "#{$source_dir }/po/*.po"              ].collect { |name| File.basename name, '.po'        },
    :manpages              => FileList[ "#{$source_dir }/doc/man/po4a/po/*.po" ].collect { |name| File.basename name, '.po'        },
  }

  $unwrapped_po            = %{ca es eu it nl uk pl sr_RS@latin tr}
  $po_multiple_sources     = %{sv}

  $benchmark_sources       = c?(:GOOGLE_BENCHMARK) ? FileList["src/benchmark/*.cpp"].to_a : []
  $benchmark_programs      = $benchmark_sources.map { |src| src.gsub(%r{\.cpp$}, '') + c(:EXEEXT) }

  $libmtxcommon_as_dll     = $building_for[:windows] && %r{shared}i.match(c(:host))

  cflags_common            = "-Wall -Wno-comment -Wfatal-errors #{c(:WLOGICAL_OP)} #{c(:WNO_MISMATCHED_TAGS)} #{c(:WNO_SELF_ASSIGN)} #{c(:QUNUSED_ARGUMENTS)}"
  cflags_common           += " #{c(:WNO_INCONSISTENT_MISSING_OVERRIDE)} #{c(:WNO_POTENTIALLY_EVALUATED_EXPRESSION)}"
  cflags_common           += " #{c(:OPTIMIZATION_CFLAGS)} -D_FILE_OFFSET_BITS=64"
  cflags_common           += " -DQT_NO_KEYWORDS"
  cflags_common           += " -DMTX_LOCALE_DIR=\\\"#{c(:localedir)}\\\" -DMTX_PKG_DATA_DIR=\\\"#{c(:pkgdatadir)}\\\" -DMTX_DOC_DIR=\\\"#{c(:docdir)}\\\""
  cflags_common           += determine_stack_protector_flags
  cflags_common           += determine_optimization_cflags
  cflags_common           += " -g3 -DDEBUG"                                              if c?(:USE_DEBUG)
  cflags_common           += " -pg"                                                      if c?(:USE_PROFILING)
  cflags_common           += " -fsanitize=undefined"                                     if c?(:USE_UBSAN)
  cflags_common           += " -fsanitize=address -fno-omit-frame-pointer"               if c?(:USE_ADDRSAN)
  cflags_common           += " -Ilib/libebml -Ilib/libmatroska"                          if c?(:EBML_MATROSKA_INTERNAL)
  cflags_common           += " -Ilib/nlohmann-json/include"                              if c?(:NLOHMANN_JSON_INTERNAL)
  cflags_common           += " -Ilib/fmt/include"                                        if c?(:FMT_INTERNAL)
  cflags_common           += " #{c(:MATROSKA_CFLAGS)} #{c(:EBML_CFLAGS)} #{c(:PUGIXML_CFLAGS)} #{c(:CMARK_CFLAGS)} #{c(:DVDREAD_CFLAGS)} #{c(:FLAC_CFLAGS)}  #{c(:EXTRA_CFLAGS)} #{c(:USER_CPPFLAGS)}"
  cflags_common           += " -mno-ms-bitfields -DWINVER=0x0601 -D_WIN32_WINNT=0x0601 " if $building_for[:windows] # 0x0601 = Windows 7/Server 2008 R2
  cflags_common           += " -march=i686"                                              if $building_for[:windows] && /i686/.match(c(:host))
  cflags_common           += " -fPIC "                                                   if !$building_for[:windows]
  cflags_common           += " -DQT_STATICPLUGIN"                                        if  $building_for[:windows]
  cflags_common           += " -DUSE_DRMINGW -I#{c(:DRMINGW_PATH)}/include"              if c?(:USE_DRMINGW) &&  $building_for[:windows]

  cflags                   = "#{cflags_common} #{c(:USER_CFLAGS)}"

  cxxflags                 = "#{cflags_common} #{c(:STD_CXX)}"
  cxxflags                += " -Wnon-virtual-dtor -Wextra -Wno-missing-field-initializers -Wunused -Wpedantic"
  cxxflags                += " -Woverloaded-virtual"                                                     if is_clang? # too many false positives in EbmlElement.h on g++ 8
  cxxflags                += " -Wno-maybe-uninitialized -Wlogical-op"                                    if is_gcc?
  cxxflags                += " -Wshadow -Qunused-arguments -Wno-self-assign -Wno-mismatched-tags"        if is_clang?
  cxxflags                += " -Wno-inconsistent-missing-override -Wno-potentially-evaluated-expression" if is_clang?
  cxxflags                += " -Wno-extra-semi"                                                          if is_clang? || check_compiler_version("gcc", "8.0.0")
  cxxflags                += " -Wmisleading-indentation -Wduplicated-cond"                               if check_compiler_version("gcc", "6.0.0")
  cxxflags                += " -Wshadow-compatible-local -Wduplicated-branches"                          if check_compiler_version("gcc", "7.0.0")
  cxxflags                += " -Wno-deprecated-copy -Wno-stringop-overflow"                              if check_compiler_version("gcc", "9.0.0")
  cxxflags                += " -Wno-c++23-attribute-extensions"                                          if check_compiler_version("clang", "20.0.0")
  cxxflags                += " #{c(:QT_CFLAGS)} #{c(:BOOST_CPPFLAGS)} #{c(:USER_CXXFLAGS)} #{c(:GTEST_CFLAGS)}"

  ldflags                  = ""
  ldflags                 += determine_stack_protector_flags
  ldflags                 += " -pg"                                     if c?(:USE_PROFILING)
  ldflags                 += " -fuse-ld=lld"                            if is_clang? && !c(:LLVM_LLD).empty? && !$building_for[:macos]
  ldflags                 += " -Llib/libebml/src -Llib/libmatroska/src" if c?(:EBML_MATROSKA_INTERNAL)
  ldflags                 += " -Llib/fmt/src"                           if c?(:FMT_INTERNAL)
  ldflags                 += " #{c(:EXTRA_LDFLAGS)} #{c(:USER_LDFLAGS)} #{c(:LDFLAGS_RPATHS)} #{c(:BOOST_LDFLAGS)}"
  ldflags                 += " -Wl,--dynamicbase,--nxcompat"               if $building_for[:windows]
  ldflags                 += " -L#{c(:DRMINGW_PATH)}/lib"                  if c?(:USE_DRMINGW) &&  $building_for[:windows]
  ldflags                 += " -fsanitize=undefined"                       if c?(:USE_UBSAN)
  ldflags                 += " -fsanitize=address -fno-omit-frame-pointer" if c?(:USE_ADDRSAN)
  ldflags                 += " -headerpad_max_install_names"               if $building_for[:macos]

  windres                  = ""
  windres                 += " -DMINGW_PROCESSOR_ARCH_AMD64=1" if c(:MINGW_PROCESSOR_ARCH) == 'amd64'

  mocflags                 = c(:QT_CFLAGS).gsub(%r{ +}, '').split(' ').select { |opt| %r{^-[DI]}.match(opt) }.join(' ')
  mocflags                += $building_for[:macos] ? " -DSYS_APPLE" : $building_for[:windows] ? " -DSYS_WINDOWS" : ""

  ranlibflags              = $building_for[:macos] ? "-no_warning_for_no_symbols" : ""

  po4aflags                = "#{c(:PO4A_FLAGS)} --msgmerge-opt=--no-wrap"

  $flags                   = {
    :cflags                => cflags,
    :cxxflags              => cxxflags,
    :ldflags               => ldflags,
    :windres               => windres,
    :moc                   => mocflags,
    :ranlib                => ranlibflags,
    :po4a                  => po4aflags,
  }

  setup_macos_specifics if $building_for[:macos]
  setup_compiler_specifics

  $build_system_modules.values.each { |bsm| bsm[:setup].call if bsm[:setup] }

  $magick_convert = !c(:MAGICK).empty? ? c(:MAGICK) + " convert" : !c(:CONVERT).empty? ? c(:CONVERT) : nil
end

def setup_overrides
  [ :programs, :manpages ].each do |type|
    value                      = c("AVAILABLE_LANGUAGES_#{type.to_s.upcase}")
    $available_languages[type] = value.split(/\s+/) unless value.empty?
  end
end

def setup_macos_specifics
  $macos_config = read_config_file("packaging/macos/config.sh")

  if ENV['MACOSX_DEPLOYMENT_TARGET'].to_s.empty? && !$macos_config[:MACOSX_DEPLOYMENT_TARGET].to_s.empty?
    ENV['MACOSX_DEPLOYMENT_TARGET'] = $macos_config[:MACOSX_DEPLOYMENT_TARGET]
  end
end

def setup_compiler_specifics
  if %r{zapcc}.match(c(:CXX) + c(:CC))
    # ccache and certain versions of zapcc don't seem to be playing well
    # together. As zapcc's servers do the caching already, disable
    # ccache if compiling with zapcc.
    ENV['CCACHE_DISABLE'] = "1"

    # zapcc doesn't support pre-compiled headers.
    ENV['USE_PRECOMPILED_HEADERS'] = "0"
  end
end

def determine_optimization_cflags
  return ""                             if !c?(:USE_OPTIMIZATION)
  return " -O#{c(:OPTIMIZATION_LEVEL)}" if c(:OPTIMIZATION_LEVEL).to_s != ""
  return " -O1"                         if is_clang? && !check_compiler_version("clang", "3.8.0") # LLVM bug 11962
  return " -O2 -fno-ipa-icf"            if $building_for[:windows] && check_compiler_version("gcc", "5.1.0") && !check_compiler_version("gcc", "7.2.0")
  return " -O3"
end

def determine_stack_protector_flags
  return " -fstack-protector"        if is_gcc? && !check_compiler_version("gcc", "4.9.0")
  return " -fstack-protector-strong" if check_compiler_version("gcc", "4.9.0") || check_compiler_version("clang", "3.5.0")
  return ""
end

def generate_helper_files
  return unless c?(:EBML_MATROSKA_INTERNAL)

  ensure_file("lib/libebml/ebml/ebml_export.h",             "#define EBML_DLL_API\n")
  ensure_file("lib/libmatroska/matroska/matroska_export.h", "#define MATROSKA_DLL_API\n")
end

def define_default_task
  # The applications themselves
  targets  = $applications.clone
  targets += $tools.map { |name| "src/tools/#{$application_subdirs[name]}#{name}" + c(:EXEEXT) }
  targets += $qt_resources

  targets << "msix-assets" if $building_for[:windows] && $magick_convert
  targets += (c(:ADDITIONAL_TARGETS) || '').split(%r{ +})

  # Build the unit tests only if requested
  targets << ($run_unit_tests ? 'tests:run_unit' : 'tests:unit') if $have_gtest

  # The tags file -- but only if it exists already
  if FileTest.exist?("TAGS")
    targets << "TAGS"   if !c(:ETAGS).empty?
    targets << "BROWSE" if !c(:EBROWSE).empty?
  end

  # Build developer documentation?
  targets << "doc/development.html" if !c(:PANDOC).empty?

  # Build man pages and translations?
  targets << "manpages"
  targets << "translations:manpages" if $po4a_cfg

  # Build translations for the programs
  targets << "translations:programs"

  # The Qt translation files: only for Windows
  targets << "translations:qt" if $building_for[:windows] && !c(:LCONVERT).blank?

  # Build ebml_validator by default when not cross-compiling as it is
  # needed for running the tests.
  targets << "apps:tools:ebml_validator" if c(:host) == c(:build)

  targets += $benchmark_programs

  task :default_targets => targets

  if !c(:DEFAULT_TARGET).empty?
    desc "Build target '#{c(:DEFAULT_TARGET)}' by default"
    task :default => c(:DEFAULT_TARGET)

  else
    desc "Build everything"
    task :default => :default_targets do
      build_duration = Time.now - $build_start
      build_duration = sprintf("%02d:%02d.%03d", (build_duration / 60).to_i, build_duration.to_i % 60, (build_duration * 1000).to_i % 1000)

      puts "Done after #{build_duration}. Enjoy :)"
    end
  end

  return if %r{drake(?:\.exe)?$}i.match($PROGRAM_NAME)
  return if !Rake.application.top_level_tasks.empty? && (Rake.application.top_level_tasks != ["default"])

  Rake.application.top_level_tasks.clear
  Rake.application.top_level_tasks.append(*targets.clone)
end

# main
setup_globals
setup_overrides
import_dependencies
generate_helper_files
update_version_number_include

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
  flags  = $flags[:cxxflags].dup

  if %r{lib/fmt/}.match(source)
    flags.gsub!(%r{-Wpedantic}, '')
  end

  args = [
    "cxx", source,
    "#{c(:CXX)} #{flags}#{pchu}#{pchx} #{$system_includes} -c -MMD -MF #{dep} -o #{t.name} -x #{lang} #{source}",
    :allow_failure => true
  ]

  Mtx::CompilationDatabase.add "directory" => $source_dir, "file" => source, "command" => args[2]

  if pchi.pretty_flags
    PCH.runq(*args[0..2], args[3].merge(pchi.pretty_flags))
  else
    runq(*args)
  end

  handle_deps t.name, last_exit_code, true
end

qrc_compiler = lambda do |*args|
  t       = args[0]
  sources = t.prerequisites

  runq "rcc", sources[0], "#{c(:RCC)} -o #{t.name} -binary #{sources.join(" ")}"
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

rule '.xml' => '.xml.erb' do |t|
  process_erb src: t.prerequisites[0], dest: t.name
end

rule '.svg' => '.svgz' do |t|
  runq_code "gunzip", :target => t.source do
    Zlib::GzipReader.open(t.source) do |file|
      IO.write(t.name, file.read)
    end
  end
end

# Resources depend on the manifest.xml file for Windows builds.
if $building_for[:windows]
  $programs.each do |program|
    path = FileTest.directory?("src/#{program}") ? program : program.gsub(/^mkv/, '')
    icon = program == 'mkvinfo' ? 'share/icons/windows/mkvinfo.ico' : 'share/icons/windows/mkvtoolnix-gui.ico'
    file "src/#{path}/resources.o" => [ "src/#{path}/manifest.xml", "src/#{path}/resources.rc", icon ]
  end

  task 'msix-assets' => 'packaging/windows/msix/assets/StoreLogo.png'

  file 'packaging/windows/msix/assets/StoreLogo.png' => 'packaging/windows/msix/generate_assets.sh' do
    runq "generate", 'packaging/windows/msix/assets', 'packaging/windows/msix/generate_assets.sh'
  end
end

rule '.mo' => '.po' do |t|
  if !%r{doc/man}.match(t.sources[0])
    runq_code "VERIFY-PO-FMT", :target => t.sources[0] do
      FormatStringVerifier.new.verify t.sources[0]
    end
  end

  runq "msgfmt", t.source, "#{c(:MSGFMT)} -c -o #{t.name} #{t.sources.join(" ")}"
end

if !c(:LCONVERT).blank?
  rule '.qm' => '.ts' do |t|
    runq "lconvert", t.source, "#{c(:LCONVERT)} -o #{t.name} -i #{t.sources.join(" ")}"
  end
end

# man pages from DocBook XML
rule '.1' => '.xml' do |t|
  filter = lambda do |code, lines|
    lines.reject! { |line| /^No localization exists for.*Using default/i.match(line) }

    if (0 == code) && lines.any? { |line| /^error|parser error/i.match(line) }
      File.unlink(t.name) if FileTest.exist?(t.name)
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

rule '.moc' => '.h' do |t|
  runq "moc", t.source, "#{c(:MOC)} #{$system_includes} #{$flags[:moc]} -nw #{t.source} > #{t.name}"
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
    #{c(:PANDOC)} -o #{t.name} --standalone --from markdown_strict --to html --number-sections --table-of-contents --css=pandoc.css --template=doc/pandoc-template.html --metadata title='MKVToolNix development' doc/development.md
COMMAND
end

file "po/mkvtoolnix.pot" => $all_sources + $all_headers + $gui_ui_h_files + %w{Rakefile} do |t|
  sources   = (t.prerequisites.dup - %w{Rakefile}).sort.uniq

  keywords  = %w{--keyword=Y --keyword=NY:1,2}   # singular & plural forms returning std::string
  keywords += %w{--keyword=FY --keyword=FNY:1,2} # singular & plural forms returning fmt::runtime(std::string)
  keywords += %w{--keyword=YT}                   # singular form returning translatable_string_c
  keywords += %w{--keyword=QTR}                  # singular form returning QString, used by uic
  keywords += %w{--keyword=QY --keyword=QNY:1,2} # singular & plural forms returning QString
  keywords += %w{--keyword=QYH}                  # singular form returning HTML-escaped QString

  flags     = %w{Y NY FY FNY YT QTR QY QNY QYH}.map { |func| "--flag=#{func}:1:no-c-format --flag=#{func}:1:no-boost-format" }.join(" ")

  options   = %w{--default-domain=mkvtoolnix --from-code=UTF-8 --language=c++}
  options  += ["'--msgid-bugs-address=Moritz Bunkus <mo@bunkus.online>'"]
  options  += ["'--copyright-holder=Moritz Bunkus <mo@bunkus.online>'", "--package-name=MKVToolNix", "--package-version=#{c(:PACKAGE_VERSION)}", "--foreign-user"]

  runq "xgettext", t.name, "xgettext #{keywords.join(" ")} #{flags} #{options.join(" ")} -o #{t.name} #{sources.join(" ")}"
end

task :manpages => $manpages

# Translations for the programs
namespace :translations do
  desc "Create a template for translating the programs"
  task :pot => "po/mkvtoolnix.pot"



  desc "Create a new .po file for the programs with an empty template"
  task "new-programs-po" => "po/mkvtoolnix.pot" do
    create_new_po "po"
  end

  desc "Create a new .po file for the man pages with an empty template"
  task "new-man-po" => "doc/man/po4a/po/mkvtoolnix.pot" do
    create_new_po "doc/man/po4a/po"
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

  task :qt => FileList[ "#{$source_dir }/po/qt/*.ts" ].collect { |file| file.ext 'qm' }

  desc "Update all translation files"
  task :update => ([ "translations:update:programs", "translations:update:installer" ] + ($po4a_cfg ? [ "translations:update:manpages" ] : []))

  namespace :update do
    desc "Update the program's translation files"
    task :programs => [ "po/mkvtoolnix.pot", ] + $available_languages[:programs].collect { |language| "translations:update:programs:#{language}" }

    namespace :programs do
      $available_languages[:programs].each do |language|
        task language => "po/mkvtoolnix.pot" do |t|
          po       = "po/#{language}.po"
          tmp_file = "#{po}.new"
          no_wrap  = $unwrapped_po.include?(language) ? "" : "--no-wrap"
          runq "msgmerge", po, "msgmerge -q #{no_wrap} #{po} po/mkvtoolnix.pot > #{tmp_file}", :allow_failure => true

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

    if $po4a_cfg
      desc "Update the man pages' translation files"
      task :manpages do
        FileUtils.touch($po4a_pot) if !FileTest.exist?($po4a_pot)

        runq "po4a", "#{$po4a_cfg} (update PO/POT)", "#{c(:PO4A)} #{$flags[:po4a]} --no-translations #{$po4a_cfg}"
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

if $po4a_cfg
  po4a_sources = []

  namespace :man2html do
    xsl_source_dir    = "doc/stylesheets"
    docbook_html_xsl  = FileList[ "#{xsl_source_dir}/*.xsl" ]
    po4a_sources     += docbook_html_xsl
    docbook_html_xsl  = docbook_html_xsl.map { |f| f.gsub(%r{.*/}, '') }

    docbook_html_xsl.each do |xsl|
      file "doc/man/#{xsl}" => "#{xsl_source_dir}/#{xsl}" do |t|
        FileUtils.copy(t.prerequisites.first, t.name)
      end
    end

    ([ 'en' ] + $languages[:manpages]).collect do |language|
      namespace language do
        dir  = language == 'en' ? '' : "/#{language}"
        xml  = FileList[ "doc/man#{dir}/*.xml" ].to_a
        html = xml.map { |name| name.ext(".html") }

        translated_xsls = docbook_html_xsl.map { |xsl| "doc/man#{dir}/#{xsl}" }

        if language != 'en'
          docbook_html_xsl.each do |xsl|
            file "doc/man/#{language}/#{xsl}" => $po4a_stamp do |t|
              runq_touch t.name
            end
          end
        end

        xml.each do |name|
          file name.ext('html') => ([ name ] + translated_xsls) do
            runq "saxon-he", name, "java -classpath lib/saxon-he/saxon9he.jar net.sf.saxon.Transform -o:#{name.ext('html')} '-xsl:doc/man#{dir}/docbook-to-html.xsl' '#{name}'"
          end

          task File.basename(name, '.xml') => name.ext('html')
        end

        task :all => html
      end
    end
  end

  po4a_output_filter = lambda do |code, lines|
    lines = lines.reject { |l| %r{seems outdated.*differ between|please consider running po4a-updatepo|^$}i.match(l) }
    puts lines.join('') unless lines.empty?
    code
  end

  po4a_sources += $manpages                      .map { |manpage|  manpage.ext('.xml')              }
  po4a_sources += $available_languages[:manpages].map { |language| "doc/man/po4a/po/#{language}.po" }

  file $po4a_stamp => po4a_sources do |t|
    File.unlink($po4a_stamp)   if  FileTest.exist?($po4a_stamp)
    FileUtils.touch($po4a_pot) if !FileTest.exist?($po4a_pot)

    runq "po4a", "#{$po4a_cfg}", "#{c(:PO4A)} #{$flags[:po4a]} #{$po4a_cfg}", :filter_output => po4a_output_filter

    $all_man_po_files.each do |po_file|
      normalize_po po_file
    end

    runq_touch $po4a_stamp
  end

  $available_languages[:manpages].each do |language|
    $manpages.each do |manpage|
      po  = manpage.gsub(/man\//, "man/#{language}/")
      xml = po.ext('xml')

      file xml => $po4a_stamp do |t|
        runq_touch xml
      end
    end
  end
end

# Developer helper tasks
namespace :dev do
  if $build_mkvtoolnix_gui
    desc "Update Qt resource files"
    task "update-qt-resources" do
      update_qrc
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

  desc "Dump dependencies of the task given with TASK environment variable"
  task "dependencies" do
    if ENV['TASK'].blank?
      puts "'TASK' environment variable not set"
      exit 1
    end

    task = Rake::Task[ENV['TASK']]
    if !task
      puts "No task named '#{ENV['MTASK']}'"
      exit 1
    end

    prereqs = task.mo_all_prerequisites
    longest = prereqs.map { |p| p[0].length }.max

    puts prereqs.map { |p| sprintf("%-#{longest}s => %s", p[0], p[1].sort.join(" ")) }.join("\n")
  end

  desc "Create iso639_language_list.cpp from online ISO code lists"
  task :iso639_list do
    create_iso639_language_list_file
  end

  desc "Create iso3166_country_list.cpp from online ISO & UN code lists"
  task :iso3166_list do
    create_iso3166_country_list_file
  end

  desc "Create iso15924_script_list.cpp from online ISO script list"
  task :iso15924_list do
    create_iso15924_script_list_file
  end

  desc "Create iana_language_subtag_registry_list.cpp from online IANA registry"
  task :iana_language_subtag_registry_list do
    Mtx::IANALanguageSubtagRegistry.create_cpp
  end

  desc "Create all auto-generated lists"
  task :lists => [ :iso639_list, :iso3166_list, :iso15924_list, :iana_language_subtag_registry_list ]

  desc "Update src/tools/element_info.cpp from libMatroska"
  task "update-element-info" do
    process_erb src: "src/tools/element_info.cpp.erb", dest: "src/tools/element_info.cpp"
  end
end

# Installation tasks
desc "Install all applications and support files"
targets  = [ "install:programs", "install:manpages", "install:translations:programs" ]
targets += [ "install:translations:manpages" ] if $po4a_cfg
targets += [ "install:shared" ]                if c?(:BUILD_GUI)
task :install => targets

namespace :install do
  application_name_mapper = lambda do |name|
    File.basename name
  end

  task :programs => $applications do
    install_dir :bindir
    $applications.each { |application| install_program "#{c(:bindir)}/#{application_name_mapper[application]}", application }
  end

  task :shared => $qt_resources do
    install_dir :desktopdir, :mimepackagesdir
    install_data :mimepackagesdir, FileList[ "#{$source_dir}/share/mime/*.xml" ]
    if c?(:BUILD_GUI)
      install_dir :appdatadir
      install_data :desktopdir, "#{$source_dir}/share/desktop/org.bunkus.mkvtoolnix-gui.desktop"
      install_data :appdatadir, "#{$source_dir}/share/metainfo/org.bunkus.mkvtoolnix-gui.appdata.xml"
    end

    wanted_apps     = %w{mkvmerge mkvtoolnix-gui mkvinfo mkvextract mkvpropedit}.collect { |e| "#{e}.png" }.to_hash_by
    wanted_dirs     = %w{16x16 24x24 32x32 48x48 64x64 96x96 128x128 256x256}.to_hash_by
    dirs_to_install = FileList[ "#{$source_dir}/share/icons/*"   ].select { |dir|  wanted_dirs[ dir.gsub(/.*icons\//, '').gsub(/\/.*/, '') ] }.sort.uniq

    dirs_to_install.each do |dir|
      dest_dir = "#{c(:icondir)}/#{dir.gsub(/.*icons\//, '')}/apps"
      install_dir dest_dir
      install_data "#{dest_dir}/", FileList[ "#{dir}/*" ].to_a.select { |file| wanted_apps[ file.gsub(/.*\//, '') ] }
    end

    if c?(:BUILD_GUI)
      sounds_dir ="#{c(:pkgdatadir)}/sounds"

      install_dir  sounds_dir
      install_data sounds_dir, FileList["#{$source_dir}/share/sounds/*"]
      install_data c(:pkgdatadir), $qt_resources
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

    if $po4a_cfg
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
    **/*.stamp
    doc/man/*.1
    doc/man/*.html
    doc/man/*.xsl
    doc/man/*/*.1
    doc/man/*/*.html
    doc/man/*/*.xml
    doc/man/*/*.xsl
    lib/libebml/ebml/ebml_export.h
    lib/libmatroska/matroska/matroska_export.h
    packaging/windows/msix/assets/*.png
    share/icons/**/*.svg
    src/**/manifest.xml
    src/info/ui/*.h
    src/mkvextract
    src/mkvinfo
    src/mkvmerge
    src/mkvpropedit
    src/mkvtoolnix-gui/forms/**/*.h
    src/mkvtoolnix-gui/mkvtoolnix-gui
    src/mkvtoolnix/mkvtoolnix
  }
  patterns += $tools.collect { |name| "src/tools/#{name}" } + $benchmark_programs
  patterns += $gtest_apps
  patterns += PCH.clean_patterns
  patterns += $qt_resources
  patterns << $version_header_name

  remove_files_by_patterns patterns

  if FileTest.exist? $dependency_dir
    puts_vaction "rm -rf", :target => "#{$dependency_dir}"
    FileUtils.rm_rf $dependency_dir
  end
end

namespace :clean do
  desc "Remove all compiled and generated files ('tarball' clean)"
  task :dist => :clean do
    run "rm -f config.h config.log config.cache build-config TAGS src/mkvtoolnix-gui/static_plugins.cpp #{Mtx::CompilationDatabase.database_file_name}", :allow_failure => true
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

# libraries required for all programs via mtxcommon
$common_libs = [
  :boost_filesystem,
  :flac,
  :z,
  :pugixml,
  :intl,
  :iconv,
  :fmt,
  :stdcppfs,
  :qt_non_gui,
  :gmp,
  "-lstdc++",
]

$common_libs += [:cmark]   if c?(:BUILD_GUI)
$common_libs += [:dvdread] if c?(:USE_DVDREAD)
$common_libs += [:exchndl] if c?(:USE_DRMINGW) && $building_for[:windows]
if !$libmtxcommon_as_dll
  $common_libs = [
    :matroska,
    :ebml,
  ] + $common_libs
end
$common_libs += [ :CoreFoundation ] if $building_for[:macos]

[ { :name => 'avi',         :dir => 'lib/avilib-0.6.10'                                                              },
  { :name => 'fmt',         :dir => 'lib/fmt/src',  :except => [ 'fmt.cc']                                           },
  { :name => 'rmff',        :dir => 'lib/librmff'                                                                    },
  { :name => 'pugixml',     :dir => 'lib/pugixml/src'                                                                },
  { :name => 'mpegparser',  :dir => 'src/mpegparser'                                                                 },
  { :name => 'mtxcommon',   :dir => [ 'src/common' ] + FileList['src/common/*'].select { |e| FileTest.directory? e } },
  { :name => 'mtxinput',    :dir => 'src/input'                                                                      },
  { :name => 'mtxoutput',   :dir => 'src/output'                                                                     },
  { :name => 'mtxmerge',    :dir => 'src/merge',    :except => [ 'mkvmerge.cpp' ],                                   },
  { :name => 'mtxextract',  :dir => 'src/extract',  :except => [ 'mkvextract.cpp' ],                                 },
  { :name => 'mtxpropedit', :dir => 'src/propedit', :except => [ 'propedit.cpp' ],                                   },
  { :name => 'ebml',        :dir => 'lib/libebml/src'                                                                },
  { :name => 'matroska',    :dir => 'lib/libmatroska/src'                                                            },
].each do |lib|
  Library.
    new("#{[ lib[:dir] ].flatten.first}/lib#{lib[:name]}").
    sources([ lib[:dir] ].flatten, :type => :dir, :except => lib[:except]).
    only_if(lib[:name] == 'mtxcommon').
    qt_dependencies_and_sources("common").
    end_if.
    only_if($libmtxcommon_as_dll && (lib[:name] == 'mtxcommon')).
    build_dll(true).
    libraries(:matroska, :ebml, $common_libs,:qt).
    end_if.
    create
end

$common_libs.unshift(:mtxcommon)

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
  libraries(:mtxmerge, :mtxinput, :mtxoutput, :mtxmerge, $common_libs, :avi, :rmff, :mpegparser, :vorbis, :ogg, $custom_libs).
  create

#
# mkvinfo
#

Application.new("src/mkvinfo").
  description("Build the mkvinfo executable").
  aliases(:mkvinfo).
  sources(FileList["src/info/*.cpp"]).
  sources("src/info/resources.o", :if => $building_for[:windows]).
  libraries($common_libs, $custom_libs).
  create

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
# mkvtoolnix
#

if $build_mkvtoolnix
  Application.new("src/mkvtoolnix/mkvtoolnix").
    description("Build the mkvtoolnix executable").
    aliases(:mkvtoolnix).
    sources("src/mkvtoolnix/mkvtoolnix.cpp").
    sources("src/mkvtoolnix/resources.o", :if => $building_for[:windows]).
    libraries($common_libs, $custom_libs).
    libraries("-mwindows", :powrprof, :if => $building_for[:windows]).
    create
end

#
# mkvtoolnix-gui
#

if $build_mkvtoolnix_gui
  add_qrc_dependencies(*$qrc)

  file "src/mkvtoolnix-gui/qt_resources.rcc" => $qrc, &qrc_compiler

  Application.new("src/mkvtoolnix-gui/mkvtoolnix-gui").
    description("Build the mkvtoolnix-gui executable").
    aliases("mkvtoolnix-gui").
    qt_dependencies_and_sources("mkvtoolnix-gui").
    sources("src/mkvtoolnix-gui/resources.o", :if => $building_for[:windows]).
    libraries($common_libs - [ :qt_non_gui ], :qt, :vorbis, :ogg).
    libraries("-mwindows", :ole32, :powrprof, :uuid, :if => $building_for[:windows]).
    libraries("-framework IOKit", :if => $building_for[:macos]).
    libraries($custom_libs).
    create
end

#
# benchmark
#

if c?(:GOOGLE_BENCHMARK) && !$benchmark_programs.empty?
  $benchmark_programs.each do |program|
    Application.new(program).
      sources(program.gsub(%r{\.exe$}, '') + '.cpp').
      libraries($common_libs, :qt, :benchmark).
      create
  end

  desc "Build the benchmark executables"
  task :benchmarks => $benchmark_programs
end

#
# Applications in src/tools
#
namespace :apps do
  task :tools => $tools.collect { |name| "apps:tools:#{name}" }
end

#
# tools
#
$tool_sources = {
  "ebml_validator" => ["ebml_validator", "element_info"],
}
$tools.each do |tool|
  Application.new("src/tools/#{tool}").
    description("Build the #{tool} executable").
    aliases("tools:#{tool}").
    sources(* ($tool_sources[tool] || [tool]).map { |src| "src/tools/#{src}.cpp" }).
    libraries($common_libs).
    create
end

# Engage pch system
PCH.engage(&cxx_compiler)

$build_system_modules.values.each { |bsm| bsm[:define_tasks].call if bsm[:define_tasks] }

# Local Variables:
# mode: ruby
# End:
