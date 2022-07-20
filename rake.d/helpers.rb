require "shellwords"

$use_tempfile_for_run = defined?(RUBY_PLATFORM) && /mingw/i.match(RUBY_PLATFORM)

$git_mutex     = Mutex.new
$message_mutex = Mutex.new
$action_width  = 14

def puts(message)
  $message_mutex.synchronize {
    $stdout.puts message
    $stdout.flush
  }
end

def puts_action(action, options = {})
  msg  = sprintf "%*s", $action_width, action.gsub(/ +/, '_').upcase
  msg += " #{options[:target]}"       if !options[:target].blank?
  msg  = "#{options[:prefix]} #{msg}" if !options[:prefix].blank?
  puts msg
end

def puts_qaction(action, options = {})
  puts_action(action, options) unless $verbose
end

def puts_vaction(action, options = {})
  puts_action(action, options) if $verbose
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

  puts cmdline unless opts[:dont_echo].to_bool

  output = nil

  if opts[:filter_output]
    output   = Tempfile.new("mkvtoolnix-rake-output")
    cmdline += " > #{output.path} 2>&1"
  end

  if $use_tempfile_for_run
    shell = ENV["RUBYSHELL"].blank? ? (ENV["SHELL"].blank? ? "sh" : ENV["SHELL"]) : ENV["RUBYSHELL"]

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

def runq(action, target, cmdline = nil, options = {}, &runner)
  start    = Time.now
  runner ||= lambda { run_wrapper cmdline, options.clone.merge(:dont_echo => !$verbose) }

  puts_action action, :target => target, :prefix => $run_show_start_stop ? "[start          ]" : nil

  code     = options[:mutex] ? options[:mutex].synchronize(&runner) : runner.call
  duration = Time.now - start

  if $run_show_start_stop
    puts_action action, :target => target, :prefix => sprintf("[%s  %02d:%02d.%03d]", code == 0 ? "done" : "fail", (duration / 60).to_i, duration.to_i % 60, (duration * 1000).to_i % 1000)
  end

  if (code != 0) && !options[:allow_failure].to_bool
    if code == 127
      puts "Error: command not found: #{cmdline}"
    elsif code == -1
      puts "Error: could not create child process for: #{cmdline}"
    end

    exit code
  end
end

def runq_git(msg, cmdline, options = {})
  runq "git #{cmdline.split(/\s+/)[0]}", "#{msg}", "git #{cmdline}", :mutex => $git_mutex
end

def runq_code(action, options = {})
  start = Time.now

  puts_action action, :target => options[:target], :prefix => $run_show_start_stop ? "[start          ]" : nil

  ok       = yield
  duration = Time.now - start

  if $run_show_start_stop
    puts_action action, :target => options[:target], :prefix => sprintf("[%s  %02d:%02d.%03d]", ok ? "done" : "fail", (duration / 60).to_i, duration.to_i % 60, (duration * 1000).to_i % 1000)
  end

  exit 1 if !ok
end

def ensure_dir dir
  File.unlink(dir) if FileTest.exist?(dir) && !FileTest.directory?(dir)
  FileUtils.mkdir_p(dir)
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

  re_source_dir = Regexp.new("^" + Regexp::escape($source_dir) + "/*")

  File.open("#{$dependency_dir}/" + target.gsub(/[\/\.]/, '_') + '.dep', "w") do |out|
    sources = IO.readlines(dep_file).
      map { |l| l.chomp.split(%r{ +}) }.
      flatten.
      map{ |l| l.gsub(%r{.*:}, '').gsub(%r{^\s+}, '').gsub(%r{\s*\\\s*$}, '').gsub(re_source_dir, '') }.
      reject(&:empty?).
      reject { |l| skip_abspath && %r{^/}.match(l) }.
      sort

    out.puts(([ target ] + sources).join("\n"))
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
    puts_vaction "rm", :target => file_name
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

def process_erb args
  puts_action "ERB", :target => args[:dest]

  template = IO.readlines(args[:src]).
    map { |line| line.gsub(%r{\{\{(.+?)\}\}}) { |match| (variables[$1] || ENV[$1]).to_s } }.
    join("")

  File.open(args[:dest], 'w') { |file| file.puts(ERB.new(template).result) }
end

def is_clang?
  c(:COMPILER_TYPE) == "clang"
end

def is_gcc?
  c(:COMPILER_TYPE) == "gcc"
end

def check_compiler_version compiler, required_version
  return false if (c(:COMPILER_TYPE) != compiler)
  return check_version(required_version, c(:COMPILER_VERSION))
end

def check_version required, actual
  return Gem::Version.new(required) <= Gem::Version.new(actual)
end

def ensure_file file_name, content = ""
  if FileTest.exists?(file_name)
    current_content = IO.readlines(file_name).join("\n")
    return if current_content == content
  end

  File.open(file_name, 'w') { |file| file.write(content) }
end

def update_version_number_include
  git_dir         = $source_dir + '/.git'
  current_version = nil
  wanted_version  = c(:PACKAGE_VERSION)

  if FileTest.exists?($version_header_name)
    lines = IO.readlines($version_header_name)

    if !lines.empty? && %r{#define.*?"([0-9.]+)"}.match(lines[0])
      current_version = $1
    end
  end

  if FileTest.directory?(git_dir)
    command = ["git", "--git-dir=#{$source_dir}/.git", "describe", "HEAD"].
      map { |e| Shellwords.escape(e) }.
      join(' ')

    description = `#{command} 2> /dev/null`.chomp

    if %r{^release-#{Regexp.escape(c(:PACKAGE_VERSION))}-(\d+)}.match(description) && ($1.to_i != 0)
      wanted_version += ".#{$1}"
    end
  end

  return if current_version == wanted_version

  File.open($version_header_name, "w") do |file|
    file.puts("#define MKVTOOLNIX_VERSION \"#{wanted_version}\"")
  end
end

def format_table rows, options = {}
  column_suffix = options[:column_suffix] || ''
  max_lengths   = [0] * rows[0].count

  rows.each do |row|
    row.each_with_index do |value, column|
      max_lengths[column] = [ max_lengths[column], value.length + (column == row.count - 1 ? 0 : column_suffix.length) ].max
    end
  end

  row_prefix = options[:row_prefix]    || ''
  row_suffix = options[:row_suffix]    || ''
  format     = max_lengths.map { |len| "%-#{len}s" }.join(' ')

  return rows.map do |row|
    formatted_row = sprintf(format, *(0..row.count - 1).map { |column| row[column].to_s + (column == row.count - 1 ? '' : column_suffix) })
    row_prefix + formatted_row + row_suffix
  end
end

def update_qrc
  require 'rexml/document'

  qrc = "src/mkvtoolnix-gui/qt_resources.qrc"

  runq_code "update", :target => qrc do
    update_qrc_worker qrc
  end
end

def update_qrc_worker qrc
  doc = REXML::Document.new File.new(qrc)

  doc.elements.to_a("/RCC/qresource/file").select { |node| %r{icons/}.match(node.text) }.each(&:remove)

  parent   = doc.elements.to_a("/RCC/qresource")[0]
  icons    = FileList["share/icons/*/*.png"].sort
  seen     = Hash.new
  add_node = lambda do |name_to_add, attributes|
    node      = REXML::Element.new "file"
    node.text = "../../#{name_to_add}"

    attributes.keys.sort.each { |key| node.attributes[key] = attributes[key] }

    parent << node
  end

  add_node.call('share/icons/index.theme', 'alias' => 'icons/mkvtoolnix-gui/index.theme')

  icons.each do |file|
    add_node.call(file, 'alias' => file.gsub(%r{^share/icons}, 'icons/mkvtoolnix-gui'))

    base_name   = file.gsub(%r{.*/|\.png$},      '')
    size        = file.gsub(%r{.*/(\d+)x\d+/.*}, '\1').to_i
    name_alias  = file.gsub(%r{\.png},           '@2x.png').gsub(%r{.*/}, "icons/mkvtoolnix-gui/#{size}x#{size}@2/")
    double_size = size * 2
    double_file = "share/icons/#{double_size}x#{double_size}/#{base_name}.png"

    next unless FileTest.exists?(double_file)

    add_node.call(double_file, 'alias' => name_alias)
    seen[file.gsub(%r{.*/icons}, 'icons')] = true
  end

  FileList["share/icons/**/*.svgz"].
    sort.
    each do |file|
    file.gsub!(%r{z$}, '')
    name_alias = file.gsub(%r{.*/icons}, 'icons/mkvtoolnix-gui')
    add_node.call(file, 'alias' => name_alias, 'compress' => '9', 'compress-algo' => 'zstd')
  end

  output            = ""
  formatter         = REXML::Formatters::Pretty.new 1
  formatter.compact = true
  formatter.width   = 9999999
  formatter.write doc, output
  File.open(qrc, "w").write(output.gsub(%r{ +\n}, "\n"))

  warned = Hash.new
  files  = read_files( %w{ui cpp}.map { |ext| FileList["src/mkvtoolnix-gui/**/*.#{ext}"].to_a.reject { |name| %r{qt_resources\.cpp$}.match(name) } }.flatten )
  files.each do |file, lines|
    line_num = 0

    lines.each do |line|
      line_num += 1

      line.scan(%r{icons/\d+x\d+/.*?\.png}) do |icon|
        next if seen[icon] || warned[icon]
        next if %r{%}.match(icon)

        puts "#{file}:#{line_num}: warning: no double-sized variant found for '#{icon}'"
        warned[icon] = true
      end
    end
  end
end

def add_qrc_dependencies *qrcs
  qrc_content = read_files(*qrcs)
  qrc_content.each do |file_name, content|
    dir          = file_name.gsub(%r{[^/]+$}, '')
    pwd          = Pathname.new(Dir.pwd)
    dependencies = content.
      join('').
      scan(%r{<file[^>]*>([^<]+)}).
      map do |matches|
      path = Pathname.new(File.absolute_path(dir + matches[0]))
      path = path.relative_path_from(pwd) unless path.relative?
      path.to_s
    end

    file file_name => dependencies do
      runq_code "touch", :target => file_name do
        FileUtils.touch file_name
      end
    end
  end
end

def parse_html_extract_table_data content, find_table_re
  require "cgi"

  content = content.force_encoding("ASCII-8BIT")

  if %r{<meta[^>]+charset="?([a-z0-9-]+)}im.match(content)
    content = content.force_encoding($1).encode("UTF-8")
  else
    content = content.force_encoding("UTF-8")
  end

  return content.
    gsub(%r{<!--.*?-->},                  '').                # remove comments
    gsub(%r{>[ \t\r\n]+<},                '><').              # completely remove whitespace between tags
    gsub(%r{[ \t\r\n]+},                  ' ').               # compress consecutive white space
    gsub(find_table_re,                   '').                # drop stuff before relevant table
    gsub(%r{</table>.*}i,                 '').                # drop stuff after relevant table
    gsub(%r{(/?<[^ >]+)[^>]*>},           '\1>').             # remove attributes
    gsub(%r{</?t(head|body|foot(er)?)>}i, '').                # remove thead/tbody/tfoot tags
    gsub(%r{<(/?)th}i,                    '<\1td').           # convert <th> tags to <td>
    gsub(%r{</tr><tr>}i,                  '<row-sep>').       # convert consecutive table row end+start to line separators to split on later
    gsub(%r{</td><td>}i,                  '<col-sep>').       # convert consecutive table column end+start to column separators to split on later
    gsub(%r{</?t[rd][^>]*>}i,             '').                # remove remaining table row/data tags
    gsub(%r{&nbsp;}i,                     '').                # ignore non-blanking spaces
    split(%r{<row-sep>}).                                     # split rows
    map { |row| CGI::unescapeHTML(row).split(%r{<col-sep>}) } # split each row into columns
end

def libmatroska_elements
  return $libmatroska_elements_cache if $libmatroska_elements_cache

  $libmatroska_elements_cache = []

  %w{libebml/ebml libebml/src libmatroska/matroska libmatroska/src}.
    map { |subdir| Dir.glob("lib/#{subdir}/*") }.
    flatten.
    select { |name| FileTest.file?(name) && %r{\.(cpp|h)$}.match(name) }.
    each do |file_name|
    IO.readlines(file_name).each do |line|
      next unless %r{^DEFINE_(?:MKX|EBML)_([A-Z]+)(?:_[_A-Z]+)? *\([^,]+, *(0x[0-9a-fA-F]+),[^"]+"([^"\\]+)}.match(line)
      $libmatroska_elements_cache << {
        name: $3,
        id:   $2.downcase,
        type: $1.downcase.to_sym,
      }
    end
  end

  $libmatroska_elements_cache.sort_by! { |element| element[:name].downcase }

  return $libmatroska_elements_cache
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
