class SimpleTest
  @@json_schema_identification = nil

  EXIT_CODE_ALIASES = {
    :success => 0,
    :warning => 1,
    :error   => 2,
  }

  def self.instantiate class_name
    file_name = class_name_to_file_name class_name
    content   = IO.readlines(file_name).join("")

    if ! %r{class\s+.*?\s+<\s+Test}.match(content)
      content = %Q!
        class ::#{class_name} < SimpleTest
          def initialize
            super

            #{content}
          end
        end
!
    else
      content.gsub!(/class\s+/, 'class ::')
    end

    eval content, nil, file_name, 5

    constantize(class_name).new
  end

  def initialize
    @commands      = []
    @tmp_num       = 0
    @tmp_num_mutex = Mutex.new
    @blocks        = {
      :setup       => [],
      :tests       => [],
      :cleanup     => [],
    }
  end

  def commands
    @commands
  end

  def describe description
    @description = description
  end

  def setup &block
    @blocks[:setup] << block
  end

  def cleanup &block
    @blocks[:cleanup] << block
  end

  def tmp_name_prefix
    [ "/tmp/mkvtoolnix-auto-test-#{self.class.name}", $$.to_s, Thread.current[:number] ].join("-") + "-"
  end

  def tmp_name
    @tmp_num_mutex.lock
    @tmp_num ||= 0
    @tmp_num  += 1
    result      = self.tmp_name_prefix + @tmp_num.to_s
    @tmp_num_mutex.unlock

    result
  end

  def tmp
    @tmp ||= tmp_name
  end

  def clean_tmp
    File.unlink(@tmp) if File.exists?(@tmp) && (ENV["KEEP_TMPFILES"] != "1")
    @tmp = nil
  end

  def hash_file name
    md5 name
  end

  def hash_tmp erase = true
    output = hash_file @tmp

    clean_tmp if erase

    output
  end

  def unlink_tmp_files
    return if ENV["KEEP_TMPFILES"] == "1"
    re = %r{^#{self.tmp_name_prefix}}
    Dir.entries("/tmp").each do |entry|
      file = "/tmp/#{entry}"
      File.unlink(file) if re.match(file) and File.exists?(file)
    end
  end

  def test name, &block
    @blocks[:tests] << { :name => name, :block => block }
  end

  def exit_code_string exit_code
    return exit_code == 0 ? 'ok'      \
         : exit_code == 1 ? 'warning' \
         : exit_code == 2 ? 'error'   \
         :                  "unknown(#{exit_code})"
  end

  def test_merge file, *args
    options                 = args.extract_options!
    full_command_line       = [ options[:args], file ].flatten.join(' ')
    options[:name]        ||= full_command_line
    options[:result_type] ||= :hash
    @blocks[:tests] << {
      :name  => full_command_line,
      :block => lambda {
        output       = options[:output] || tmp
        _, exit_code = *merge(full_command_line, :exit_code => options[:exit_code], :output => output)
        result       = options[:exit_code]   == :error ? 'error'           \
                     : options[:result_type] == :hash  ? hash_file(output) \
                     :                                   exit_code_string(exit_code)

        clean_tmp unless options[:keep_tmp]

        result
      },
    }
  end

  def test_identify file, *args
    options             = args.extract_options!
    options[:format]    = :json if options[:format].nil?
    full_command_line   = [ "--identification-format", options[:format].to_s, "--identify", options[:args], file ].flatten.join(' ')
    options[:name]    ||= full_command_line
    @blocks[:tests] << {
      :name  => full_command_line,
      :block => lambda {
        sys "../src/mkvmerge #{full_command_line} --engage no_variable_data > #{tmp}", :exit_code => options[:exit_code]

        text = IO.readlines(tmp).reject { |line| %r{^\s*"identification_format_version":\s*\d+}.match(line) }.join('')
        File.open(tmp, 'w') { |tmp_file| tmp_file.puts text }

        if options[:filter]
          text = options[:filter].call(IO.readlines(tmp).join(''))
          File.open(tmp, 'w') { |tmp_file| tmp_file.puts text }
        end
        options[:keep_tmp] ? hash_file(tmp) : hash_tmp
      },
    }
  end

  def test_info file, *args
    options             = args.extract_options!
    full_command_line   = [ options[:args], file ].flatten.join(' ')
    options[:name]    ||= full_command_line
    @blocks[:tests] << {
      :name  => full_command_line,
      :block => lambda {
        output = options[:output] || tmp
        info full_command_line, :exit_code => options[:exit_code], :output => output
        options[:keep_tmp] ? hash_file(output) : hash_tmp
      },
    }
  end

  def test_merge_unsupported file, *args
    options             = args.extract_options!
    full_command_line   = [ options[:args], file ].flatten.join(' ')
    options[:name]    ||= full_command_line
    @blocks[:tests] << {
      :name  => full_command_line,
      :block => lambda {
        sys "../src/mkvmerge --identify-verbose #{full_command_line} > #{tmp}", :exit_code => 3
        %r{unsupported container}.match(IO.readlines(tmp).first || '') ? :ok : :bad
      },
    }
  end

  def test_ui_locale locale, *args
    describe "mkvmerge / UI locale: #{locale}"

    @blocks[:tests] << {
      :name  => "mkvmerge UI locale #{locale}",
      :block => lambda {
        sys "../src/mkvmerge -o /dev/null --ui-language #{locale} data/avi/v.avi | head -n 2 | tail -n 1 > #{tmp}-#{locale}"
        result = hash_file "#{tmp}-#{locale}"
        self.error 'Locale #{locale} not supported by MKVToolNix' if result == 'f54ee70a6ad9bfc5f61de5ba5ca5a3c8'
        result
      },
    }
    @blocks[:tests] << {
      :name  => "mkvinfo UI locale #{locale}",
      :block => lambda {
        sys "../src/mkvinfo --ui-language #{locale} data/mkv/complex.mkv | head -n 2 > #{tmp}-#{locale}"
        result = hash_file "#{tmp}-#{locale}"
        self.error 'Locale #{locale} not supported by MKVToolNix' if result == 'f54ee70a6ad9bfc5f61de5ba5ca5a3c8'
        result
      },
    }
  end

  def description
    @description || fail("Class #{self.class.name} misses its description")
  end

  def run_test
    @blocks[:setup].each(&:call)

    results = @blocks[:tests].collect do |test|
      result = nil
      begin
        result = test[:block].call
      rescue RuntimeError => ex
        show_message "Test case '#{self.class.name}', sub-test '#{test[:name]}': #{ex}"
        result = :failed
      end
      result
    end

    @blocks[:cleanup].each(&:call)

    unlink_tmp_files

    results.join '-'
  end

  def merge *args
    options = args.extract_options!
    fail ArgumentError if args.empty?

    output  = options[:output] || self.tmp
    command = "../src/mkvmerge --engage no_variable_data -o #{output} #{args.first}"
    self.sys command, :exit_code => options[:exit_code], :no_result => options[:no_result]
  end

  def identify *args
    options = args.extract_options!
    fail ArgumentError if args.empty?

    verbose = !options[:verbose].nil? ? options[:verbose]                             : true
    format  = options[:format]        ? options[:format].to_s.downcase.gsub(/_/, '-') : verbose ? 'verbose-text' : 'text'

    command = "../src/mkvmerge --identify --identification-format #{format} --engage no_variable_data #{args.first}"

    self.sys command, :exit_code => options[:exit_code], :no_result => options[:no_result]
  end

  def identify_json *args
    options = args.extract_options!
    fail ArgumentError if args.empty?

    command = "../src/mkvmerge --identify --identification-format json --engage no_variable_data #{args.first}"

    output, _ = self.sys(command, :exit_code => options[:exit_code], :no_result => options[:no_result])

    return JSON.load(output.join(''))
  end

  def info *args
    options = args.extract_options!
    fail ArgumentError if args.empty?

    output  = options[:output] || self.tmp
    output  = "> #{output}" unless %r{^[>\|]}.match(output)
    output  = '' if options[:output] == :return
    command = "../src/mkvinfo --engage no_variable_data --ui-language en_US #{args.first} #{output}"
    self.sys command, :exit_code => options[:exit_code], :no_result => options[:no_result]
  end

  def extract *args
    options = args.extract_options!
    fail ArgumentError if args.empty?

    mode     = options[:mode] || :tracks
    command  = "../src/mkvextract --engage no_variable_data #{mode} #{args.first} " + options.keys.select { |key| key.is_a?(Numeric) }.sort.collect { |key| "#{key}:#{options[key]}" }.join(' ')

    self.sys command, :exit_code => options[:exit_code], :no_result => options[:no_result]
  end

  def propedit file_name, *args
    options = args.extract_options!
    fail ArgumentError if args.empty?

    command = "../src/mkvpropedit --engage no_variable_data #{file_name} #{args.first}"
    *result = self.sys command, :exit_code => options[:exit_code], :no_result => options[:no_result]

    self.sys "../src/tools/ebml_validator -M #{file_name}", dont_record_command: true if FileTest.exists?("../src/tools/ebml_validator")

    return *result
  end

  def sys *args
    options             = args.extract_options!
    options[:exit_code] = EXIT_CODE_ALIASES[ options[:exit_code] ] || options[:exit_code] || 0
    fail ArgumentError if args.empty?

    command    = args.shift
    @commands << { :command => command, :no_result => options[:no_result] } unless options[:dont_record_command]

    if !%r{>}.match command
      temp_file = Tempfile.new('mkvtoolnix-test-output')
      temp_file.close
      command  << " >#{temp_file.path} 2>&1 "
    end

    puts "COMMAND #{command}" if ENV['DEBUG']

    exit_code = 0
    if !system(command)
      exit_code = $? >> 8
      self.error "system command failed: #{command} (#{exit_code})" if options[:exit_code] != exit_code
    end

    return IO.readlines(temp_file.path), exit_code if temp_file
    return exit_code
  end

  def cp source, target, *args
    options = args.extract_options!
    options[:no_result] = true unless options.key?(:no_result)

    sys "cp '#{source}' '#{target}'", options
  end

  def error reason
    show_message "  Failed. Reason: #{reason}"
    raise "test failed"
  end

  def json_schema_identification
    return @@json_schema_identification if @@json_schema_identification

    require "json_schema"

    json_store = JsonSchema::DocumentStore.new
    parser     = JsonSchema::Parser.new
    expander   = JsonSchema::ReferenceExpander.new
    version    = IO.readlines("../src/merge/id_result.h").detect { |line| /#define +ID_JSON_FORMAT_VERSION\b/.match line }.gsub(/.* .* /, '').chop
    schema     = parser.parse JSON.load(File.read("../doc/json-schema/mkvmerge-identification-output-schema-v#{version}.json"))

    expander.expand(schema, store: json_store)
    json_store.add_schema schema

    @@json_schema_identification = schema
  end

  def json_summarize_tracks json
    basics     = %w{id type codec}
    properties = %w{language number audio_channels audio_sampling_frequency pixel_dimensions}

    json["tracks"].
      map do |t|
        (basics.map { |b| t[b] } +
         properties.map { |p| t["properties"][p] }).
        reject(&:nil?).
        join("+").
        gsub(%r{[:/+-]+}, "_")
      end.
      join("/")
  end
end
