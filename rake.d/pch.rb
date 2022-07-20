#
#   mkvtoolnix - programs for manipulating Matroska files
#   Copyright © 2003…2016 Moritz Bunkus
#
#   This file is distributed as part of mkvtoolnix and is distributed under the
#   GNU General Public License Version 2, or (at your option) any later version.
#   See accompanying file COPYING for details or visit
#   http://www.gnu.org/licenses/gpl.html .
#
#   precompiled header support
#
#   Authors: KonaBlend <kona8lend@gmail.com>
#

module PCH
  extend Rake::DSL

  #############################################################################
  #
  # Parser for customized variable number of task[options...] in white-space
  # separated, name=value pairs, with quoted strings permitted.
  #
  #   name       --> toggle action (must be a boolean)
  #   name=      --> set to default value
  #   name=value --> set to value
  #
  class Options
    def initialize(t, spec)
      @task = t
      @spec = spec
      @pairs = {}
    end

    def handler(key, &block)
      key = key.to_s
      if block.arity == 0 # reader has 0 args
        define_singleton_method(key.to_sym) { block.call }
      else
        @pairs.store(key, block)
      end
    end

    def parse
      return unless @spec
      parse_root do |k,v|
        fail "unknown name '#{k}' specified for task '#{@task.name}'" unless @pairs.has_key?(k)
        @pairs.fetch(k).call(k, v)
      end
    end

    def parse_root
      @index = 0
      @name = ''
      @value = nil

      while @index < @spec.length
        c = @spec[@index]
        if c == " " || c == "\t"
          @index += 1
        else
          parse_name
          yield @name,@value
          @name = ''
          @value = nil
        end
      end
    end

    def parse_name
      while @index < @spec.length
        c = @spec[@index]
        case
        when c == " "
          @index += 1
          break
        when c == "="
          fail "unexpected '#{c}' while parsing name" if @name.empty?
          @index += 1
          parse_value
          break
        else
          @name += c
          @index += 1
        end
      end
    end

    def parse_value
      @value = ''
      while @index < @spec.length
        c = @spec[@index]
        case
        when c == " "
          @index += 1
          break
        when c == '"' && (@index+1) < @spec.length
          @index += 1
          parse_quote
        else
          @value += c
          @index += 1
        end
      end
    end

    def parse_quote
      open = true
      while @index < @spec.length
        c = @spec[@index]
        case
        when c == '"'
          open = false
          @index += 1
          break
        when c == '\\' && (@index+1) < @spec.length
          @value += @spec[@index+1]
          @index += 2
        else
          @value += c
          @index += 1
        end
      end
      fail "unterminated quote while parsing value" if open
    end
  end

  #############################################################################

  def self.engage(&cxx_compiler)
    return unless c?(:USE_PRECOMPILED_HEADERS)
    load_config
    @users.clear
    $all_sources.each { |source| @users.store(source, nil) }
    @moc_users.each_key { |moc| @users.store(moc, Pathname.new(moc).sub_ext(".h").to_s) }
    namespace :pch do
      t = file(@config_file.to_s => @config_deps) { scan_users }
      t.invoke
    end
    add_tasks(&cxx_compiler)
    add_prerequisites
  end

  #############################################################################

  def self.add_tasks(&compiler)
    pchs = []
    headers = {}
    @db_scan.each_value { |h| headers.store(h, nil) }
    headers.keys.each do |header|
      pch = "#{header}#{@extension}"
      pchs.push(file pch => "#{header}", &compiler)
    end

    desc "Set pch related options (persistent)"
    task :pch, :options do |t,args|
      o = Options.new(t, args.options)

      o.handler(:htrace) { @htrace }
      o.handler(:htrace) do |k,v|
        case v
        when nil
          @htrace = !@htrace
        when ''
          @htrace = false
        when /^(true|yes|1)$/i
          @htrace = true
        when /^(false|no|0)$/i
          @htrace = false
        else
          fail "invalid value for pch[htrace]: #{v}"
        end
        puts "pch[htrace] = #{@htrace}"
      end

      o.handler(:pretty) { @pretty }
      o.handler(:pretty) do |k,v|
        case v
        when nil
          @pretty = !@pretty
        when ''
          @pretty = false
        when /^(true|yes|1)$/i
          @pretty = true
        when /^(false|no|0)$/i
          @pretty = false
        else
          fail "invalid value for pch[pretty]: #{v}"
        end
        puts "pch[pretty] = #{@pretty}"
      end

      o.parse
    end

    namespace :pch do
      desc "Overview"
      rself = Pathname.new(__FILE__)
      rself = rself.relative_path_from(Pathname.new(Dir.pwd)) unless rself.relative?
      task :overview do
        text = <<-ENDTEXT
:
:   PCH - precompiled header support
:
:   Precompiled headers are active when the host operating system and tools
:   are capable (as detected and enabled by configure). A PCH configuration
:   is saved in '#{@config_file.to_s}' and is automatically managed and
:   also participates with task 'clean:dist'.
:
:   The config is a task resolved at rake load-time. That is to say, this
:   task effectively behaves like a prerequisite to all other tasks.
:
:   PCH system will bootstrap by scanning all known user files (.cpp, .moc)
:   for idiomatic precompiled header usage. The pattern scanned for is:
:
:       #{@scan_include_re.to_s}
:
:   PCH will re-bootstrap if '#{@config_file.to_s} is out of date to any of:
:
<% @config_deps.each do |x| %>
:       <%= x %>
<% end %>
:
:   During source file compilation, logic has been added to probe give PCH
:   opportunity to supply additional compiler flags (-include) and thus use
:   a precompiled header.
:
:   The pch binary is added as a prerequisite for all of its users, so
:   pch generation is automatic, but tasks 'pch:all' and 'pch:clean' may be
:   used manually.
:
:   'pch:status' generates a short report on PCH state of affairs. Verbosity
:   may be increased like so; note shell quotes placed around target to
:   escape shell-special treatment of square-brackets.
:
:       #{Rake.application.name} "pch:status[verbose]"
:
:   'pch[htrace]' is persistent flag which (when true) causes all subsequent
:   source file compilation to record which header files were opened. The
:   output is saved to .htrace files and .htrace files are clean as part
:   of task 'clean'.
:
:   'pch[pretty]' is persistent flag which (when true) adds more information
:   about files as they are compiled.
:
        ENDTEXT
        ERB.new(text, nil, trim_mode="<>").run(binding)
      end

      desc "Generate all precompiled headers"
      task :all => pchs

      desc "Remove all precompiled headers"
      task :clean do
        pats = []
        pchs.each { |x| pats << x.name << (x.name + "-????????") }
        remove_files_by_patterns(pats)
      end

      desc "Scan candidate user files for precompiled header pattern."
      task :scan do
        scan_users
      end

      desc "Status of database"
      task :status, :options do |t,args|
        o = Options.new(t, args.options)

        o.handler(:verbose) { o.instance_exec { @verbose }}
        o.handler(:verbose) do |k,v|
          case v
          when nil
            o.instance_exec { @verbose = !@verbose }
          when ""
            o.instance_exec { @verbose = false }
          when /^(true|yes|1)$/i
            o.instance_exec { @verbose = true }
          when /^(false|no|0)$/i
            o.instance_exec { @verbose = false }
          else
            fail "invalid value for pch[verbose]: #{v}"
          end
        end
        o.parse
        status(o)
      end
    end

    Rake::Task['clean:dist'].enhance { @endblock_skip_save = true }
  end

  #############################################################################

  def self.add_prerequisites
    @db_scan.each_pair do |user, header|
      case File.extname(user)
      when '.moc'
        object = user + 'o'
        file object => user
      else
        object = Pathname.new(user).sub_ext('.o')
      end
      file object => "#{header}#{@extension}"
    end
  end

  #############################################################################

  # return hash of file types (extension) => count
  def self.file_types(files)
    r = {}
    files.each do |name|
      key = File.extname(name)
      i = r.fetch(key, 0)
      r.store(key, i+1)
    end
    r.sort_by { |k,v| k }.collect { |x| "#{x[0]}=#{x[1]}" }.join(", ")
  end

  def self.status(options)
    headers = {}
    @db_scan.each_value do |h|
       i = headers.fetch(h, 0)
       headers.store(h, i+1)
    end
    unmarked = @users.keys.clone
    unmarked.delete_if { |k,v| @db_scan.has_key?(k) }

    text = <<-ENDTEXT
PCH status: <%= c?(:USE_PRECOMPILED_HEADERS) ? "enabled" : "disabled" %>
:
:   htrace = #{@htrace}
:   pretty = #{@pretty}
:
: <%= "%8d %-40s (%s)" % [@users.size, "total users", file_types(@users.keys)] %>
: <%= "%8d %-40s (%s)" % [headers.size, "unique pch headers", file_types(headers.keys)] %>
: <%= "%8d %-40s (%s)" % [@db_scan.size, "users marked for pch use", file_types(@db_scan.keys)] %>
: <%= "%8d %-40s (%s)" % [unmarked.size, "users not marked", file_types(unmarked)] %>
<% headers.each_pair do |k,v| %>
: <%= "%8d %-40s (%s)" % [v,"users of %s" % [k], file_types(@db_scan.select { |k1,v1| v1 == k }.keys)] %>
<% end %>
    ENDTEXT

    text.concat( <<-ENDTEXT
<% lines = @db_scan.each_pair.collect { |k,v| ":   marked %s -> %s" % [k,v] } %>
<% unless lines.empty? %>

<% end %>
<%= lines.join("\n") %>
<% lines = unmarked.collect { |u| ": unmarked %s" % [u] } %>
<% unless lines.empty? %>

<% end %>
<%= lines.join("\n") %>
    ENDTEXT
  ) if options.verbose

    text << ":\n"
    ERB.new(text, nil, trim_mode="<>").run(binding)
  end

  #############################################################################

  def self.scan_users
    if @verbose
      verb = @config_file.exist? ? "rescan" : "scan"
    else
      verb = "%*s" % [$action_width, (@config_file.exist? ? "rescan" : "scan").upcase]
    end
    puts "#{verb} pch candidates (total=#{@users.size}, #{file_types(@users.keys)})"
    @users.each_pair { |k,v| scan_user(k,v) }
  end

  def self.scan_user(user, indirect)
    found = nil
    input = indirect ? indirect : user
    File.open(input) do |f|
      f.each_line do |line|
        line.force_encoding("UTF-8")
        next if !@scan_include_re.match(line)
        @scan_candidates.each do |pair|
          (dir,header) = *pair
          next unless $1 == header
          found = dir ? "#{dir}/#{$1}" : $1
          break
        end
        break if found
      end
    end
    return unless found
    @db_scan.store(user, found)
  rescue StandardError => e
    puts "WARNING: unable to read #{input}: #{e}"
  end

  #############################################################################

  class Info
    attr_accessor :language
    attr_accessor :use_flags
    attr_accessor :extra_flags
    attr_accessor :pretty_flags
  end

  def self.info_for_user(user, ofile)
    f = Info.new
    if c?(:USE_PRECOMPILED_HEADERS) && !%r{src/common/iso639_language_list.cpp}.match(user)
      user = Pathname.new(user).cleanpath.to_s
      f.language = "c++-header" if user.end_with?(".h")
      header = @db_scan.fetch(user, nil)
      f.use_flags = header ? " -include #{header}" : nil
      f.extra_flags = @htrace ? "-H" : nil
      f.pretty_flags = (@pretty || @htrace) ? {
        htrace: @htrace ? (ofile + ".htrace") : nil,
        user: @users.has_key?(user),
        precompile: f.use_flags != nil,
      } : nil
    end
    f
  end

  #############################################################################

  def self.moc_users(others)
    others.each { |other| @moc_users.store(other, nil) }
  end

  #############################################################################

  def self.make_task_filter(name)
    return nil unless @htrace

    lambda do |code,lines|
      rx_stat = /^([.]*[!x]?)\s(.*)/
      rx_ignore_begin = /^Multiple include guards may be useful for/i
      rx_ignore_more = /\//

      ignore = false

      File.open(name + ".htrace", "w") do |io|
        lines.each do |line|
          case
          when line =~ rx_stat
            ;
          when !ignore && line =~ rx_ignore_begin
            ignore = true
            next
          when ignore && line =~ rx_ignore_more
            next
          else
            puts line
            next
          end
          io.write("#{line}")
        end
      end
    end
  end

  #############################################################################

  def self.load_config
    # puts "load #{@config_file}" if @verbose
    config = {}
    config = @config_file.open { |f| JSON.load(f) } if @config_file.exist?
    @htrace = config.fetch('htrace', @htrace)
    @pretty = config.fetch('pretty', @pretty)
    @db_scan = config.fetch('scan', @db_scan)
  end

  def self.save_config
    # puts "save #{@config_file}" if @verbose
    root = {
      htrace: @htrace,
      pretty: @pretty,
      scan: @db_scan,
    }
    @config_file.open("w") do |out|
      out.write(JSON.generate(root, space:" ", indent:"  ", object_nl:"\n"))
      out.write("\n")
    end
  end

  #############################################################################

  def self.clean_patterns
    %W[
      **/*.[gp]ch **/*.pch-????????
      **/*.htrace
      #{@config_file}
    ]
  end

  #############################################################################
  #
  # Execute a system command, similar to helpers.rb/runq but differs mainly
  # in offering generator-style filter model, delayed, and in some cases
  # more information on normal rake stdout.
  #
  # All output is accumulated into a buffer until the command completes.
  # Advantage of keeping command-output together in a parallel build.
  # Disadvantage of delaying output until command is complete.
  #
  def self.runq(action, subject, command, options={})
    command = command.gsub(/\n/, ' ').gsub(/^\s+/, '').gsub(/\s+$/, '').gsub(/\s+/, ' ')
    if @verbose
      puts command
    else
      h = options.fetch(:htrace, nil) ? 't' : '-'
      u = options.fetch(:user, nil) ? 'u' : '-'
      p = options.fetch(:precompile, nil) ? 'p' : '-'
      puts "%c%c%c %*s %s" % [h, u, p, $action_width - 4, action.gsub(/ +/, '_').upcase, subject]
    end
    htrace = options.fetch(:htrace, nil)
    return execute(command, options) unless htrace

    # log -H output, return other
    File.open(htrace, "w") do |io|
      rx_stat = /^([.]*[!x]?)\s(.*)/
      rx_ignore_begin = /^Multiple include guards may be useful for/i
      rx_ignore_more = /\//
      ignore = false
      # generator records (sends to rake output) only lines we return
      execute(command, options) do |line|
        case
        when line =~ rx_stat
          io.write("#{line}")
          next nil
        when !ignore && line =~ rx_ignore_begin
          ignore = true
          next nil
        when ignore && line =~ rx_ignore_more
          next nil
        else
          next line
        end
      end
    end
  end

  def self.execute(command, options={}, &block)
    if STDOUT.tty? && $pty_module_available
      execute_tty(command, options, &block)
    else
      execute_command(command, options, &block)
    end
  end

  def self.execute_command(command, options={})
    lines = []
    IO.popen(command, :err => [:child, :out]) do |io|
      if block_given?
        io.each_line { |s| lines.push(s) if s = yield(s) }
      else
        io.each_line { |s| lines << s }
      end
    end
    end_execute(lines, options)
  end

  def self.execute_tty(command, options={})
    lines = []
    IO.pipe do |read,write|
      master,slave = PTY.open
      begin
        pid = spawn(command, :in => read, [:out, :err] => slave)
        read.close
        slave.close
        begin
          if block_given?
            master.each_line { |s| lines.push(s) if s = yield(s) }
          else
            master.each_line { |s| lines << s }
          end
        rescue
          ;
        ensure
          Process.wait(pid)
        end
      ensure
        master.close unless master.closed?
        slave.close unless slave.closed?
      end
    end
    end_execute(lines, options)
  end

  def self.end_execute(lines, options)
    if options.fetch(:allow_failure, nil) != true
      exit(1) if $?.exitstatus == nil
      exit($?.exitstatus) if $?.exitstatus != 0
    end
    ps = $?.clone
    puts lines
    [ps, lines]
  end

  #############################################################################

  # True when tracing headers during compilation. This controls when a filter
  # in CXX compilation action engages, and also causes adds -H compiler flag.
  @htrace = false

  # True when PCH enhances typical rake output with extra information.
  @pretty = false

  # GCC and clang differ in their extension preferences for precompiled
  # headers. clang has some knowledge of gch and at the time of this writing,
  # actually does search for both { .pch, .gch } but it's not documented.
  @extension = c(:COMPILER_TYPE) == "clang" ? ".pch" : ".gch"

  # Cache global verbose flag.
  @verbose = $verbose

  #  Each pch file must be unambiguous for simple scanning techniques.
  #  The scanner needs to know the string of a pch file as it is used in
  #  CPP include directives. It also needs to know the prefix pathname
  #  as it exists in source tree, relative to Rakefile.
  @scan_candidates = [ ["src", "common/common_pch.h"] ]

  # All usage of project pch files are expected to be between quotes,
  # and not angle brackets.
  @scan_include_re = /^\s*#\s*include\s+"([^"]+)"/

  # Location of pch configuration.
  @config_file = Pathname.new("config.pch.json")

  # Populated by project rakefile.
  @moc_users = {}

  # All candidate user files (.cpp, .moc)
  # user => intermediary
  # where user is the actual source file being compiled
  #   and intermediary is nil or alternate file to scan (used by .moc)
  @users = {}

  # Resident scan DB. Has mapping of user files (.cpp, .moc) to a pch file
  # (as used in #include directive).
  @db_scan = {}

  rself = Pathname.new(__FILE__)
  rself = rself.relative_path_from(Pathname.new(Dir.pwd)) unless rself.relative?
  @config_deps = [Rake.application.rakefile, 'build-config', rself.to_s]

  #############################################################################

  @endblock_skip_save = false

  END {
    next if @endblock_skip_save
    begin
      save_config
    rescue StandardError => e
      puts "WARNING: failed to save config: #{e}"
    end
  }

  #############################################################################

end # module PCH
