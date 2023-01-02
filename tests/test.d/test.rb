class SubtestError < RuntimeError
end

class Test
  attr_reader :commands
  attr_reader :debug_commands

  def initialize
    @tmp_num        = 0
    @tmp_num_mutex  = Mutex.new
    @commands       = Array.new
    @debug_commands = Array.new
  end

  def description
    return "dummy test class"
  end

  def run
    return error("Trying to run the base class")
  end

  def unlink_tmp_files
    return if (ENV["KEEP_TMPFILES"] == "1")
    re = /^#{self.tmp_name_prefix}/
    Dir.entries($temp_dir).each do |entry|
      file = "#{$temp_dir}/#{entry}"
      File.unlink(file) if re.match(file) and FileTest.exist?(file)
    end
  end

  def run_test expected_results
    result = nil
    begin
      result = run
    rescue RuntimeError => ex
      show_message ex.to_s
    end
    unlink_tmp_files
    result
  end

  def error(reason)
    show_message "  Failed. Reason: #{reason}"
    raise "test failed"
  end

  def sys(command, *arg)
    options          = arg.extract_options!
    @commands       << { :command => command, :no_result => options[:no_result] } unless options[:dont_record_command]
    @debug_commands << command                                                    unless options[:dont_record_command]
    command         << " >/dev/null 2>/dev/null " unless (/>/.match(command))

    puts "COMMAND #{command}" if ENV['DEBUG']

    result = run_bash command

    error "system command failed: #{command} (" + ($? >> 8).to_s + ")" if !result && ((arg.size == 0) || ((arg[0] << 8) != $?))
  end

  def tmp_name_prefix
    [ "#{$temp_dir}/mkvtoolnix-auto-test", $$.to_s, Thread.current[:number] ].join("-") + "-"
  end

  def tmp_name
    @tmp_num_mutex.lock
    @tmp_num ||= 0
    @tmp_num  += 1
    result     = self.tmp_name_prefix + @tmp_num.to_s
    @tmp_num_mutex.unlock

    result
  end

  def tmp
    @tmp ||= tmp_name
  end

  def hash_file(name)
    md5 name
  end

  def hash_tmp(erase = true)
    output = hash_file @tmp

    if erase
      File.unlink(@tmp) if FileTest.exist?(@tmp) && (ENV["KEEP_TMPFILES"] != "1")
      @debug_commands << "rm #{@tmp}"
      @tmp = nil
    end

    output
  end

  def merge(*args)
    retcode = args.detect { |a| !a.is_a? String } || 0
    args.reject!          { |a| !a.is_a? String }

    raise "Wrong use of the 'merge' function." if args.empty? || (2 < args.size)

    command = "../src/mkvmerge --engage no_variable_data -o " + (args.size == 1 ? tmp : args.shift) + " " + args.shift
    sys command, retcode
  end

  def propedit *args
    retcode = args.detect { |a| !a.is_a? String } || 0
    args.reject!          { |a| !a.is_a? String }

    file_name = args.size == 1 ? tmp : args.shift

    command = "../src/mkvpropedit --engage no_variable_data #{file_name} #{args.shift}"
    *result = sys command, retcode

    sys "../src/tools/ebml_validator -M #{file_name}", :dont_record_command => true if FileTest.exist?("../src/tools/ebml_validator")

    return *result
  end

  def xtr_tracks_s(*args)
    command = "../src/mkvextract tracks data/mkv/complex.mkv --engage no_variable_data " + args.join(" ") + ":#{tmp}"
    sys command, 0
    hash_tmp
  end

  def xtr_tracks(*args)
    command = "../src/mkvextract tracks #{args.shift} --engage no_variable_data " + args.join(" ")
    sys command, 0
  end
end
