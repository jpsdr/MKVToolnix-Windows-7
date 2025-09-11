class Controller
  attr_accessor :test_failed, :test_new, :test_date_after, :teset_date_before, :update_failed, :num_failed, :record_duration, :show_duration
  attr_reader   :num_threads, :results

  def initialize
    @results          = Results.new(self)
    @test_failed      = false
    @test_new         = false
    @test_date_after  = nil
    @test_date_before = nil
    @update_failed    = false
    @num_threads      = self.get_num_processors
    @record_duration  = false
    @show_duration    = false
    @for_mkvgoodbad   = %r{^(1|true)$}i.match(ENV['MKVGOODBAD_OUTPUT'] || '')

    @tests            = Array.new
    @exclusions       = Array.new
    @dir_entries      = Dir.entries(".")
  end

  def get_num_processors
    np = case RUBY_PLATFORM
         when $is_macos then `/usr/sbin/sysctl -n hw.availcpu`.to_i
         else                `nproc`.to_i
         end
    [ np, 0 ].max + 1
  end

  def num_threads=(num)
    error_and_exit "Invalid number of threads: must be > 0" if 0 >= num
    @num_threads = num
  end

  def add_test_case(num)
    @tests += @dir_entries.select { |entry| /^test-#{num}/.match(entry) }
  end

  def exclude_test_case(num)
    @exclusions += @dir_entries.select { |entry| /^test-#{num}/.match(entry) }
  end

  def get_tests_to_run
    test_all = !@test_failed && !@test_new
    tests    = @tests.empty? ? @dir_entries : @tests
    tests   -= @exclusions

    return tests.collect do |entry|
      if (FileTest.file?(entry) && (entry =~ /^test-.*\.rb$/))
        class_name  = file_name_to_class_name entry
        test_this   = test_all
        test_this ||= (@results.exist?(class_name) && ((@test_failed      && (@results.status?(class_name) == :failed)) || (@test_new && (@results.status?(class_name) == :new))) ||
                                                       (@test_date_after  && (@results.date_added?(class_name) < @test_date_after)) ||
                                                       (@test_date_before && (@results.date_added?(class_name) > @test_date_before)))
        test_this ||= !@results.exist?(class_name) && (@test_new || @test_failed)

        test_this ? class_name : nil
      else
        nil
      end
    end.compact.sort_by { |class_name| @results.duration? class_name }
  end

  def go
    @num_failed    = 0
    @tests_to_run  = self.get_tests_to_run
    num_tests      = @tests_to_run.size

    @tests_mutex   = Mutex.new
    @results_mutex = Mutex.new

    start          = Time.now

    self.run_threads
    self.join_threads

    test_end = Time.now
    duration = test_end - start

    show_message "#{@num_failed}/#{num_tests} failed (" + (num_tests > 0 ? (@num_failed * 100 / num_tests).to_s : "0") + "%). " +
      "Tests took #{duration}s (#{start.strftime("%Y-%m-%d %H:%M:%S")} – #{test_end.strftime("%Y-%m-%d %H:%M:%S")})"
  end

  def run_threads
    @threads = Array.new

    (1..@num_threads).each do |number|
      @threads << Thread.new(number) do |thread_number|
        Thread.current[:number] = thread_number

        while true
          @tests_mutex.lock
          class_name = @tests_to_run.shift
          @tests_mutex.unlock

          break unless class_name
          self.run_test class_name
        end
      end
    end
  end

  def join_threads
    @threads.each &:join
  end

  def run_test(class_name)
    begin
      current_test = SimpleTest.instantiate class_name
    rescue Exception => ex
      self.add_result class_name, :failed, :message => " Failed to load or create an instance of class '#{class_name}'. #{ex}"
      return
    end

    if (current_test.description == "INSERT DESCRIPTION")
      show_message "Skipping '#{class_name}': Not implemented yet"
      return
    end

    if current_test.methods.include?(:skip?) and current_test.skip?
      show_message "Skipping '#{class_name}': disabled"
      return
    end

    show_message "Running '#{class_name}': #{current_test.description}"

    expected_results = @results.exist?(class_name) ? @results.hash?(class_name).split(/-/) : nil

    start    = Time.now
    result   = current_test.run_test expected_results
    duration = Time.now - start

    puts "Finished '#{class_name}' after #{sprintf('%0.3f', duration)}s" if self.show_duration

    if (result)
      if result.include?("failed")
        self.add_result class_name, :failed, :message => "  #{class_name} FAILED: test failed with an exception"

      elsif (!@results.exist? class_name)
        self.add_result class_name, :passed, :message => "  NEW test. Storing result '#{result}'.", :checksum => result, :duration => duration

      elsif (@results.hash?(class_name) == result)
        self.add_result class_name, :passed, :duration => duration

      else
        for_mkvgoodbad = @for_mkvgoodbad ? " for mkvgoodbad" : ""
        msg            =  "  #{class_name} FAILED: checksum is different. Commands#{ for_mkvgoodbad}:\n"
        actual_results = result.split(/-/)
        idx            = 0

        if (@for_mkvgoodbad)
          current_test.commands.each do |command|
            command = { :command => command } unless command.is_a?(Hash)

            if !command[:no_result] && (expected_results[idx] != actual_results[idx])
              mkvgoodbad_cmd = command[:command].gsub(%r{^\.\./src/| -o [a-z0-9._/-]+| *>.*}i, '').gsub(%r{  +}, ' ')

              if %r{^mkvmerge}.match(mkvgoodbad_cmd)
                mkvgoodbad_cmd.
                  gsub!(%r{^mkvmerge}, 'mkvgoodbad').
                  gsub!(%r{ *--engage no_variable_data *}, ' ').
                  gsub!(%r{  +}, ' ')
              end

              if %r{ -i | -J |--identify}.match(mkvgoodbad_cmd)
                mkvgoodbad_cmd.gsub!(%r{^mkvgoodbad}, 'mkvgoodbadidentify')
              end

              msg += "#{mkvgoodbad_cmd}\n"
            end

            idx += 1 unless command[:no_result]
          end

        else
          current_test.commands.each do |command|
            command = { :command => command } unless command.is_a?(Hash)
            prefix  = !command[:no_result] && (expected_results[idx] != actual_results[idx]) ? "(*)" : "   "
            msg    += "  #{prefix} #{command[:command]}\n"
            idx    += 1 unless command[:no_result]
          end
        end

        if (update_failed && actual_results.include?("failed"))
          self.add_result class_name, :failed, :message => msg + "  FATAL: cannot update result as test results include 'failed'"
        elsif (update_failed)
          self.add_result class_name, :passed, :message => msg + "  UPDATING result\n", :checksum => result, :duration => duration
        else
          self.add_result class_name, :failed, :message => msg
        end
      end

    else
      self.add_result class_name, :failed, :message => "  #{class_name} FAILED: no result from test"
    end
  end

  def add_result(class_name, result, opts = {})
    @results_mutex.lock

    show_message opts[:message] if opts[:message]
    @num_failed += 1            if result == :failed

    if !@results.exist? class_name
      @results.add class_name, opts[:checksum]
    else
      @results.set      class_name, result
      @results.set_hash class_name, opts[:checksum] if opts[:checksum]
    end

    @results.set_duration class_name, opts[:duration] if opts[:duration] && (result == :passed)

    @results_mutex.unlock
  end

  def list_failed_ids
    entries = @dir_entries.collect do |entry|
      if (FileTest.file?(entry) && (entry =~ /^test-(\d+).*\.rb$/))
        class_name  = file_name_to_class_name entry
        @results.status?(class_name) == :failed ? $1 : nil
      else
        nil
      end
    end

    puts entries.reject(&:nil?).sort_by(&:to_i).join(" ")
  end
end
