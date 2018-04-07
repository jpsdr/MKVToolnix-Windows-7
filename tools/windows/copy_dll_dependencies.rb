#!/usr/bin/env ruby

$mtx_dir = File.realpath(File.dirname(__FILE__) + "/../..")

require "fileutils"
require "find"
require "pp"
require "shellwords"
require "#{$mtx_dir}/rake.d/config"

ENV['LC_MESSAGES'] = 'C'
$title             = "CopyDLLDependencies"
$config            = read_config_file("#{$mtx_dir}/build-config")
$mxe_dir            = File.realpath(File.dirname(`which #{Shellwords.escape(c(:CXX))}`.chomp) + "/../#{c(:host)}")
$dll_locations     = {}
$dependencies      = Hash[ *%w{advapi32 d3d9 dwmapi dxva2 evr gdi32 imm32 iphlpapi kernel32 mf mfplat msvcrt powrprof shell32 ole32 oleaut32 user32 uxtheme winmm ws2_32}.map { |e| [ "#{e}.dll", true ] }.flatten ]
$dependencies["winspool.drv"] = true

def info text
  return unless (ENV["DEBUG"] || "0").to_i == 1
  puts "#{$title}: #{text}"
end

def locate_dlls base_path
  info "locating DLLs"

  $dll_locations = {}

  Find.find(base_path) do |path|
    next unless FileTest.file?(path) and %r{\.dll}i.match(path)

    base_name = File.basename(path)

    if $dll_locations.key?(base_name.downcase)
      raise "More than one location for DLL '#{base_name}': #{$dll_locations[base_name]}  and  #{path}"
    end

    $dll_locations[base_name.downcase] = path
  end
end

def locate_dependencies file
  `#{Shellwords.escape(c(:OBJDUMP))} -x #{Shellwords.escape(file)} 2>&1`.
    split(%r{\n+}).
    select { |line| %r{^\s+dll\s+name\s*:}i.match(line) }.
    map    { |line| line.gsub(%r{.*:\s*|\s*$}, '') }.
    each do |dll_name|

    dll_name_l = dll_name.downcase

    next if $dependencies[dll_name_l]

    if !$dll_locations[dll_name_l]
      raise "Dependency for '#{file}' not found: #{dll_name}"
    end

    $dependencies[dll_name_l] = $dll_locations[dll_name_l]

    locate_dependencies dll_name
  end
end

def copy_dlls
  to_copy = $dependencies.values.select { |entry| entry.is_a?(String) }.sort

  info "copying #{to_copy.size} DLLs"

  to_copy.each { |dll| FileUtils.cp dll, "." }
end

if ARGV.size < 1
  info "error: need at least one argument: a file to locate dependencies for"
  exit 1
end

locate_dlls($mxe_dir)

info "locating dependencies"
ARGV.each { |file| locate_dependencies(file) }

copy_dlls
