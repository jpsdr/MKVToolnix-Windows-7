#!/usr/bin/env ruby

require "digest/md5"
require "json"
require "json-schema"
require "pp"
require "rexml/document"
require "tempfile"

require_relative "test.d/controller.rb"
require_relative "test.d/results.rb"
require_relative "test.d/test.rb"
require_relative "test.d/simple_test.rb"
require_relative "test.d/util.rb"
require_relative "../rake.d/config.rb"
require_relative "../rake.d/extensions.rb"

begin
  require "thread"
rescue
end

def setup
  $is_windows                          = %r{win|mingw}i.match(RUBY_PLATFORM)
  $is_macos                            = %r{darwin}i.match(RUBY_PLATFORM)
  $is_linux                            = !$is_windows && !$is_macos

  $ui_language_en_us                   = $is_windows ? "English" : "en_US"

  ENV[ $is_macos ? 'LANG' : 'LC_ALL' ] = 'en_US.UTF-8'
  ENV['MTX_ALWAYS_USE_UNIX_NEWLINES']  = '1'
  ENV['PATH']                          = "../src:" + ENV['PATH']

  $config                              = read_build_config
end

def main
  controller = Controller.new

  ARGV.each do |arg|
    if ((arg == "-f") or (arg == "--failed"))
      controller.test_failed = true
    elsif ((arg == "-n") or (arg == "--new"))
      controller.test_new = true
    elsif ((arg == "-u") or (arg == "--update-failed"))
      controller.update_failed = true
    elsif ((arg == "-r") or (arg == "--record-duration"))
      controller.record_duration = true
    elsif (arg =~ /-d([0-9]{4})([0-9]{2})([0-9]{2})-([0-9]{2})([0-9]{2})/)
      controller.test_date_after = Time.local($1, $2, $3, $4, $5, $6)
    elsif (arg =~ /-D([0-9]{4})([0-9]{2})([0-9]{2})-([0-9]{2})([0-9]{2})/)
      controller.test_date_before = Time.local($1, $2, $3, $4, $5, $6)
    elsif ((arg == "-s") or (arg == "--show-duration"))
      controller.show_duration = true
    elsif arg =~ /-j(\d+)/
      controller.num_threads = $1.to_i
    elsif /^ (!)? test-(\d{4}) .* \.rb $/x.match arg
      method = $1 == '!' ? :exclude_test_case : :add_test_case
      controller.send(method, $2)
    elsif /^ (!)? (\d{1,4}) (?: - (\d{1,4}) )?$/x.match arg
      method = $1 == '!' ? :exclude_test_case : :add_test_case
      $2.to_i.upto(($3 || $2).to_i) { |idx| controller.send(method, sprintf("%04d", idx)) }
    elsif %r{^ (!)? / (.+) / $}ix.match arg
      method = $1 == '!' ? :exclude_test_case : :add_test_case
      re     = Regexp.new "^T_(\\d+).*(?:#{$2})", Regexp::IGNORECASE
      tests  = controller.results.results.keys.collect { |e| re.match(e) ? $1 : nil }.compact
      error_and_exit "No tests matched RE #{re}" if tests.empty?
      tests.each { |e| controller.send(method, e) }
    elsif ((arg == "-F" || (arg == "--list-failed")))
      controller.list_failed_ids
      exit 0
    elsif ((arg == "-h") || (arg == "--help"))
      puts <<EOHELP
Syntax: run.rb [options]
  -F, --list-failed     list IDs of failed tests
  -f, --failed          only run tests marked as failed
  -n, --new             only run tests for which no entry exists in results.txt
  -dDATE                only run tests added after DATE (YYYYMMDD-HHMM)
  -DDATE                only run tests added before DATE (YYYYMMDD-HHMM)
  -u, --update-failed   update the results for tests that fail
  -r, --record-duration update the duration field of the tests run
  -s, --show-duration   show how long each test took to run
  -jNUM                 run NUM tests at once (default: number of CPU cores)
  123                   run test 123 (any number; can be given multiple times)
  12-345                run tests 12 through 345 (any range of numbers; can be given multiple times)
  !123                  do not run test 123 (any number; can be given multiple times)
  !12-345               do not run tests 12 through 345 (any range of numbers; can be given multiple times)
  /REGEX/               run tests whose names match REGEX (case insensitive; can be given multiple times)
  !/REGEX/              do not run tests whose names match REGEX (case insensitive; can be given multiple times)
EOHELP
      exit 0
    else
      error_and_exit "Unknown argument '#{arg}'."
    end
  end

  controller.go

  exit controller.num_failed > 0 ? 1 : 0
end

Dir.mktmpdir do |dir|
  $temp_dir = dir
  setup
  main
end
