class Target
  public

  begin
    include Rake::DSL
  rescue
  end

  def initialize(target)
    @target       = target
    @aliases      = []
    @objects      = []
    @libraries    = []
    @dependencies = []
    @file_deps    = []
    @only_if      = true
    @debug        = {}
    @desc         = nil
    @namespace    = nil
  end

  def debug(category)
    @debug[category] = !@debug[category]
    self
  end

  def debug_do
    yield self
    self
  end

  def description(description)
    @desc = description
    self
  end

  def only_if(condition)
    @only_if = condition
    self
  end

  def end_if
    only_if true
  end

  def extract_options(list)
    options = list.empty? || !list.last.is_a?(Hash) ? {} : list.pop
    return list, options
  end

  def aliases(*list)
    @aliases += list.compact
    self
  end

  def sources(*list)
    list, options = extract_options list

    if @debug[:sources]
      puts "Target::sources: only_if #{@only_if}; list & options:"
      pp list
      pp options
    end

    return self if !@only_if || (options.include?(:if) && !options[:if])

    ext_map = {
      '.ui'  => 'h',
      '.moc' => '.moco',
    }

    obj_re          = /\.(?:moc)?o$/
    no_file_deps_re = /\.o$/

    list           = list.collect { |e| e.respond_to?(:to_a) ? e.to_a : e }.flatten
    file_mode      = (options[:type] || :file) == :file
    except         = !file_mode && options[:except].is_a?(Array) ? options[:except].collect { |file| list.collect { |dir| "#{dir}/#{file}" } }.flatten.to_hash_by : {}
    new_sources    = list.collect { |entry| file_mode ? (entry.respond_to?(:to_a) ? entry.to_a : entry) : FileList["#{entry}/*.c", "#{entry}/*.cpp", "#{entry}/*.cc", "#{entry}/*.mm"].to_a }.flatten.select { |file| !except[file] }.for_target!
    new_deps       = new_sources.collect { |file| [ file.ext(ext_map[ file.pathmap('%x') ] || 'o'), file ] }
    new_file_deps  = new_deps.reject { |src, tgt| no_file_deps_re.match src }
    new_file_deps += new_sources.select { |file| %r{\.moc$}.match file }.map { |file| [ file, file.ext('h') ] }
    @objects       = ( @objects      + new_deps.collect { |a| a.first }.select { |file| obj_re.match file } ).uniq
    @dependencies  = ( @dependencies + new_deps.collect { |a| a.first }                                     ).uniq
    @file_deps     = ( @file_deps    + new_file_deps                                                        ).uniq

    PCH.moc_users(new_deps.select { |dep| /\.moco$/.match dep[0] }.map { |dep| dep[0].ext(".moc") }.to_a)
    self
  end

  def qt_dependencies_and_sources(subdir, options = {})
    ui_files         = FileList["src/#{subdir}/forms/**/*.ui"].to_a
    ui_h_files       = ui_files.collect { |ui| ui.ext 'h' }
    cpp_files        = FileList["src/#{subdir}/**/*.cpp", "src/#{subdir}/**/*.mm"].to_a.for_target! - (options[:cpp_except] || [])
    h_files          = FileList["src/#{subdir}/**/*.h"].to_a.for_target! - ui_h_files
    content          = read_files(cpp_files + h_files)
    qobject_h_files  = h_files.select { |h| content[h].any? { |line| /\bQ_OBJECT\b/.match line } }

    form_include_re  = %r{^\s* \# \s* include \s+ \" (#{subdir}/forms/[^\"]+) }x
    dep_builder      = lambda do |files|
      extra_dependencies = {}

      files.each do |file_name|
        content[file_name].each do |line|
          next unless form_include_re.match line

          extra_dependencies[file_name] ||= []
          extra_dependencies[file_name]  << "src/#{$1}"
        end
      end

      extra_dependencies
    end

    dep_builder.call(cpp_files).      each { |cpp, ui_hs| file cpp.ext('o')   => ui_hs }
    dep_builder.call(qobject_h_files).each { |h,   ui_hs| file h.  ext('moc') => ui_hs }

    self.
      sources(ui_files).
      sources(qobject_h_files.collect { |h| h.ext 'moc' }).
      sources(cpp_files)
  end

  def dependencies(*list)
    @dependencies += list.flatten.select { |entry| !entry.blank? } if @only_if
    self
  end

  def libraries(*list)
    list, options = extract_options list

    return self if !@only_if || (options.include?(:if) && !options[:if])

    list.flatten!

    @dependencies += list.collect do |entry|
      case entry
      when :mtxcommon   then "src/common/libmtxcommon.#{$libmtxcommon_as_dll ? "dll" : "a"}"
      when :mtxinput    then "src/input/libmtxinput.a"
      when :mtxoutput   then "src/output/libmtxoutput.a"
      when :mtxmerge    then "src/merge/libmtxmerge.a"
      when :mtxinfo     then "src/info/libmtxinfo.a"
      when :mtxextract  then "src/extract/libmtxextract.a"
      when :mtxpropedit then "src/propedit/libmtxpropedit.a"
      when :mtxunittest then "tests/unit/libmtxunittest.a"
      when :avi         then "lib/avilib-0.6.10/libavi.a"
      when :rmff        then "lib/librmff/librmff.a"
      when :mpegparser  then "src/mpegparser/libmpegparser.a"
      when :ebml        then c?("EBML_MATROSKA_INTERNAL") ? "lib/libebml/src/libebml.a"         : nil
      when :matroska    then c?("EBML_MATROSKA_INTERNAL") ? "lib/libmatroska/src/libmatroska.a" : nil
      when :gtest       then $gtest_internal              ? "lib/gtest/src/libgtest.a"          : nil
      when :pugixml     then c?(:PUGIXML_INTERNAL)        ? "lib/pugixml/src/libpugixml.a"      : nil
      when :fmt         then c?(:FMT_INTERNAL)            ? "lib/fmt/src/libfmt.a"              : nil
      else                   nil
      end
    end.compact

    @libraries += list.collect do |entry|
      case entry
      when nil               then nil
      when :flac             then c(:FLAC_LIBS)
      when :gtest            then $gtest_internal ? [ '-Llib/gtest/src', '-lgtest' ] : c(:GTEST_LIBS)
      when :iconv            then c(:ICONV_LIBS)
      when :intl             then c(:LIBINTL_LIBS)
      when :cmark            then c(:CMARK_LIBS)
      when :dvdread          then c(:DVDREAD_LIBS)
      when :boost_filesystem then c(:BOOST_FILESYSTEM_LIB)
      when :boost_system     then c(:BOOST_SYSTEM_LIB)
      when :pugixml          then c?(:PUGIXML_INTERNAL) ? [ '-Llib/pugixml/src', '-lpugixml' ] : c(:PUGIXML_LIBS)
      when :qt               then c(:QT_LIBS)
      when :qt_non_gui       then c(:QT_LIBS_NON_GUI)
      when :static           then c(:LINK_STATICALLY)
      when :stdcppfs         then c(:STDCPPFS_LIBS)
      when :mpegparser       then [ '-Lsrc/mpegparser', '-lmpegparser'  ]
      when :mtxinput         then [ '-Lsrc/input',      '-lmtxinput'    ]
      when :mtxoutput        then [ '-Lsrc/output',     '-lmtxoutput'   ]
      when :mtxmerge         then [ '-Lsrc/merge',      '-lmtxmerge'    ]
      when :mtxextract       then [ '-Lsrc/extract',    '-lmtxextract'  ]
      when :mtxpropedit      then [ '-Lsrc/propedit',   '-lmtxpropedit' ]
      when :mtxunittest      then [ '-Ltests/unit',     '-lmtxunittest' ]
      when :ebml             then c(:EBML_LIBS,     '-lebml')
      when :matroska         then c(:MATROSKA_LIBS, '-lmatroska')
      when :CoreFoundation   then '-framework CoreFoundation'
      when String            then entry
      else                        "-l#{entry}"
      end
    end.compact

    self
  end

  def dump
    %w{aliases sources objects dependencies libraries}.each do |type|
      puts "@#{type}:"
      pp instance_variable_get("@#{type}")
    end
    self
  end

  def create
    definition = Proc.new do
      @file_deps.each do |spec|
        file spec.first => spec.last unless spec.first == spec.last
      end

      @aliases.each_with_index do |name, idx|
        desc @desc if (0 == idx) && !@desc.empty?
        task name => @target
      end
    end

    if @namespace
      namespace @namespace, &definition
    else
      definition.call
    end

    create_specific

    self
  end
end
