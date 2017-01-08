#!/usr/bin/env ruby
# coding: utf-8

require "fastimage"
require "fileutils"

$base_dir = File.absolute_path(File.dirname(__FILE__) + "/../..")

ARGV.each do |file_name|
  size = FastImage.size file_name
  if !size
    puts "Warning: couldn't determine size for #{file_name}"
    next
  end

  dir = "#{$base_dir}/share/icons/#{size[0]}x#{size[1]}"
  if !FileTest.exist? dir
    puts "Warning: no destination directory for size #{size[0]}x#{size[1]}"
    next
  end

  target = "#{dir}/" + file_name.gsub(/.*\//, '')

  puts "#{file_name} → #{target}"
  FileUtils.cp file_name, target
end

puts "Updating Qt resources"
Dir.chdir $base_dir
system "rake dev:update-qt-resources"
