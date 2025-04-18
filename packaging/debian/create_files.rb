#!/usr/bin/env ruby

require "erb"
require "fileutils"

$distribution         = nil
$distribution_version = nil

def show_help error
  puts "Error: #{error}"
  puts

  puts "Syntax: #{__FILE__} [-d|--distribution <distribution> -v|--version <version>]"
  puts "  Example 1: #{__FILE__} -d Ubuntu -v 22.04"
  puts "  Example 2: #{__FILE__} --distribution Debian --version 11"
  puts
  puts "If at least one of them isn't specified, guessing the distribution &"
  puts "version the scripts runs under is attempted."

  exit 1
end

def create_file erb_file_name, erb_binding
  puts "handling #{erb_file_name}"

  dest_file_name = erb_file_name.gsub(%r{\.erb$}, '')
  content        = IO.read(erb_file_name, encoding: "utf-8")

  File.open(dest_file_name, 'w') { |file| file.puts(ERB.new(content, trim_mode: '<>').result(erb_binding)) }

  if %r{rules$}.match(dest_file_name)
    FileUtils.chmod 0755, dest_file_name
  end
end

def handle_all_erb_files
  distro  = $distribution.downcase
  version = Gem::Version.new($distribution_version)

  puts "Creating files for #{distro} #{version}"

  Dir.glob(File.absolute_path(File.dirname(__FILE__)) + "/*.erb").each do |file_name|
    create_file(file_name, binding)
  end
end

def parse_cli_args
  while !ARGV.empty?
    arg = ARGV.shift

    if %r{^-(?:d|-distribution)$}.match(arg)
      show_help("Missing argument to '#{arg}'") if ARGV.empty?
      $distribution = ARGV.shift

    elsif %r{^-(?:v|-version)$}.match(arg)
      show_help("Missing argument to '#{arg}'") if ARGV.empty?
      $distribution_version = ARGV.shift

    else
      show_help("Unrecognized option '#{arg}'")
    end
  end
end

def parse_lsb_release
  lsb_file = "/etc/lsb-release"

  return false if !FileTest.file?(lsb_file)

  values = Hash[*
    IO.readlines(lsb_file).
    map(&:chomp).
    select { |line| %r{.+=.+}.match(line)                                      }.
    map    { |line| a = line.split('=', 2); [ a[0], a[1].gsub(%r{^"|"$}, '') ] }.
    flatten
  ]

  $distribution         = values["DISTRIB_ID"]
  version               = (values["DISTRIB_RELEASE"] || '').gsub(%r{[^\d.].*}, '')
  version               = version.gsub(%r{\..*}, '') if ($distribution.downcase == 'linuxmint')
  $distribution_version = version != '' ? version : nil

  return $distribution && $distribution_version
end

def parse_sources_list
  content = ([ "/etc/apt/sources.list" ] + Dir.glob("/etc/apt/sources.list.d/*")).
    select     { |file_name| FileTest.file?(file_name) }.
    map        { |file_name| IO.readlines(file_name, encoding: "utf-8") }.
    inject([]) { |accu, lines| accu + lines }

  ### Ubuntu

  if content.any? { |line| %r{linuxming.*(vanessa|vera|victoria)}.match(line) }
    $distribution         = 'linuxmint'
    $distribution_version = '21'
    return true
  end

  if content.any? { |line| %r{ubuntu.*kinetic}.match(line) }
    $distribution         = 'ubuntu'
    $distribution_version = '22.10'
    return true
  end

  if content.any? { |line| %r{ubuntu.*jammy}.match(line) }
    $distribution         = 'ubuntu'
    $distribution_version = '22.04'
    return true
  end

  if content.any? { |line| %r{ubuntu.*impish}.match(line) }
    $distribution         = 'ubuntu'
    $distribution_version = '21.10'
    return true
  end

  if content.any? { |line| %r{ubuntu.*hirsute}.match(line) }
    $distribution         = 'ubuntu'
    $distribution_version = '21.04'
    return true
  end

  if content.any? { |line| %r{ubuntu.*groovy}.match(line) }
    $distribution         = 'ubuntu'
    $distribution_version = '20.10'
    return true
  end

  if content.any? { |line| %r{ubuntu.*focal}.match(line) }
    $distribution         = 'ubuntu'
    $distribution_version = '20.04'
    return true
  end

  if content.any? { |line| %r{ubuntu.*eoan}.match(line) }
    $distribution         = 'ubuntu'
    $distribution_version = '19.10'
    return true
  end

  if content.any? { |line| %r{ubuntu.*disco}.match(line) }
    $distribution         = 'ubuntu'
    $distribution_version = '19.04'
    return true
  end

  if content.any? { |line| %r{ubuntu.*cosmic}.match(line) }
    $distribution         = 'ubuntu'
    $distribution_version = '18.10'
    return true
  end

  if content.any? { |line| %r{ubuntu.*bionic}.match(line) }
    $distribution         = 'ubuntu'
    $distribution_version = '18.04'
    return true
  end

  ### Debian

  if content.any? { |line| %r{debian.*bookworm}.match(line) }
    $distribution         = 'debian'
    $distribution_version = '12'
    return true
  end

  if content.any? { |line| %r{debian.*bullseye}.match(line) }
    $distribution         = 'debian'
    $distribution_version = '11'
    return true
  end

  if content.any? { |line| %r{debian.*buster}.match(line) }
    $distribution         = 'debian'
    $distribution_version = '10'
    return true
  end

  if content.any? { |line| %r{debian.*stretch}.match(line) }
    $distribution         = 'debian'
    $distribution_version = '9'
    return true
  end

  return false
end

def guess_distribution
  return if parse_lsb_release
  return if parse_sources_list
end

def main
  parse_cli_args

  guess_distribution                                  if !$distribution || !$distribution_version
  show_help("Could not guess distribution & version") if !$distribution || !$distribution_version

  handle_all_erb_files
end

main
