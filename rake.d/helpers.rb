# coding: utf-8
$use_tempfile_for_run = defined?(RUBY_PLATFORM) && /mingw/i.match(RUBY_PLATFORM)
require "tempfile"
require "fileutils"

$git_mutex     = Mutex.new
$message_mutex = Mutex.new
$action_width  = 12

def puts(message)
  $message_mutex.synchronize {
    $stdout.puts message
    $stdout.flush
  }
end

def puts_action(action, target=nil)
  msg  = sprintf "%*s", $action_width, action.gsub(/ +/, '_').upcase
  msg += " #{target}" if target && !target.empty?
  puts msg
end

def puts_qaction(action, target=nil)
  puts_action(action, target) unless $verbose
end

def puts_vaction(action, target=nil)
  puts_action(action, target) if $verbose
end

def error(message)
  puts message
  exit 1
end

def last_exit_code
  $?.respond_to?(:exitstatus) ? $?.exitstatus : $?.to_i
end

def run cmdline, opts = {}
  code = run_wrapper cmdline, opts
  exit code if (code != 0) && !opts[:allow_failure].to_bool
end

def run_wrapper cmdline, opts = {}
  cmdline = cmdline.gsub(/\n/, ' ').gsub(/^\s+/, '').gsub(/\s+$/, '').gsub(/\s+/, ' ')
  code    = nil
  shell   = ENV["RUBYSHELL"].blank? ? "c:/msys/bin/sh" : ENV["RUBYSHELL"]

  puts cmdline unless opts[:dont_echo].to_bool

  output = nil

  if opts[:filter_output]
    output   = Tempfile.new("mkvtoolnix-rake-output")
    cmdline += " > #{output.path} 2>&1"
  end

  if $use_tempfile_for_run
    Tempfile.open("mkvtoolnix-rake-run") do |t|
      t.puts cmdline
      t.flush
      system shell, t.path
      code = last_exit_code
      t.unlink
    end
  else
    system cmdline
    code = last_exit_code
  end

  code = opts[:filter_output].call code, output.readlines if opts[:filter_output]

  return code

ensure
  if output
    output.close
    output.unlink
  end
end

def runq(action, target, cmdline, options = {})
  puts_qaction action, target
  run cmdline, options.clone.merge(:dont_echo => !$verbose)
end

def runq_git(msg, cmdline, options = {})
  puts_qaction "git #{cmdline.split(/\s+/)[0]}", "#{msg}"
  $git_mutex.synchronize { run "git #{cmdline}", options.clone.merge(:dont_echo => !$verbose) }
end

def ensure_dir dir
  File.unlink(dir) if  FileTest.exist?(dir) && !FileTest.directory?(dir)
  Dir.mkdir(dir)   if !FileTest.exist?(dir)
end

def create_dependency_dirs
  [ $dependency_dir, $dependency_tmp_dir ].each { |dir| ensure_dir dir }
end

def dependency_output_name_for file_name
  $dependency_tmp_dir + "/" + file_name.gsub(/[\/\.]/, '_') + '.d'
end

def handle_deps(target, exit_code, skip_abspath=false)
  dep_file = dependency_output_name_for target
  get_out  = lambda do
    File.unlink(dep_file) if FileTest.exist?(dep_file)
    exit exit_code if 0 != exit_code
    return
  end

  FileTest.exist?(dep_file) || get_out.call

  create_dependency_dirs

  File.open("#{$dependency_dir}/" + target.gsub(/[\/\.]/, '_') + '.dep', "w") do |out|
    line = IO.readlines(dep_file).collect { |l| l.chomp }.join(" ").gsub(/\\/, ' ').gsub(/\s+/, ' ')
    if /(.+?):\s*([^\s].*)/.match(line)
      target  = $1
      sources = $2.gsub(/^\s+/, '').gsub(/\s+$/, '').split(/\s+/)

      if skip_abspath
        sources.delete_if { |entry| entry.start_with? '/' }
      end

      out.puts(([ target ] + sources).join("\n"))
    end
  end

  get_out.call
rescue
  get_out.call
end

def import_dependencies
  return unless FileTest.directory? $dependency_dir
  Dir.glob("#{$dependency_dir}/*.dep").each do |file_name|
    lines  = IO.readlines(file_name).collect(&:chomp)
    target = lines.shift
    file target => lines.select { |dep_name| File.exists? dep_name }
  end
end

def arrayify(*args)
  args.collect { |arg| arg.is_a?(Array) ? arg.to_a : arg }.flatten
end

def install_dir(*dirs)
  arrayify(*dirs).each do |dir|
    dir = c(dir) if dir.is_a? Symbol
    run "#{c(:mkinstalldirs)} #{c(:DESTDIR)}#{dir}"
  end
end

def install_program(destination, *files)
  destination = c(destination) + '/' if destination.is_a? Symbol
  arrayify(*files).each do |file|
    run "#{c(:INSTALL_PROGRAM)} #{file} #{c(:DESTDIR)}#{destination}"
  end
end

def install_data(destination, *files)
  destination = c(destination) + '/' if destination.is_a? Symbol
  arrayify(*files).each do |file|
    run "#{c(:INSTALL_DATA)} #{file} #{c(:DESTDIR)}#{destination}"
  end
end

def remove_files_by_patterns patterns
  patterns.collect { |pattern| FileList[pattern].to_a }.flatten.uniq.select { |file_name| File.exists? file_name }.each do |file_name|
    puts_vaction "rm", file_name
    File.unlink file_name
  end
end

def read_files *file_names
  Hash[ *file_names.flatten.collect { |file_name| [ file_name, IO.readlines(file_name).collect { |line| line.force_encoding "UTF-8" } ] }.flatten(1) ]
end

def list_targets? *targets
  spec = ENV['RAKE_LIST_TARGETS'].to_s
  targets.any? { |target| %r{\b#{target}\b}.match spec }
end

class Rake::Task
  def mo_all_prerequisites
    todo   = [name]
    result = []

    while !todo.empty?
      current = todo.shift
      prereqs = Rake::Task[current].prerequisites

      next if prereqs.empty?

      result << [current, prereqs]
      todo   += prereqs
    end

    result.uniq
  end

  def investigate
    result = "------------------------------\n"
    result << "Investigating #{name}\n"
    result << "class: #{self.class}\n"
    result <<  "task needed: #{needed?}\n"
    result <<  "timestamp: #{timestamp}\n"
    result << "pre-requisites:\n"
    prereqs = @prerequisites.collect { |name| Rake::Task[name] }
    prereqs.sort! { |a,b| a.timestamp <=> b.timestamp }
    result += prereqs.collect { |p| "--#{p.name} (#{p.timestamp})\n" }.join("")
    latest_prereq = @prerequisites.collect{|n| Rake::Task[n].timestamp}.max
    result <<  "latest-prerequisite time: #{latest_prereq}\n"
    result << "................................\n\n"
    return result
  end
end
