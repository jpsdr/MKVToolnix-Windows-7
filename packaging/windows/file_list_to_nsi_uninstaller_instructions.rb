#!/usr/bin/env ruby

require "pp"
require_relative "conf"

fail "Missing file list name" if ARGV.empty?

config    = read_config
file_name = ARGV[0]
file_name = "#{config['file_list_dir']}/#{file_name}.txt" if !FileTest.exist?(file_name)
files     = IO.readlines(file_name).map { |file| file.chomp.gsub(%r{^\.}, '').gsub(%r{^/}, '').gsub(%r{/}, '\\') }
dirs      = {}

files.each do |file|
  puts %Q(  Delete "$INSTDIR\\#{file}")

  dir = file
  while true
    break unless %r{\\}.match(file)

    dir.gsub!(%r{\\[^\\]+$}, '')
    dirs[dir] = true
  end
end

puts

dirs.
  keys.
  sort_by { |dir| [ dir.gsub(%r{[^\\]+}, '').length, dir ] }.
  reverse.
  each    { |dir| puts %Q(  RMDir "$INSTDIR\\#{dir}") }
