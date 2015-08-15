#!/usr/bin/env ruby
# coding: utf-8

require "fileutils"
require "shellwords"
require "tmpdir"

$po_dir = File.absolute_path(File.dirname(__FILE__) + "/../po")

module AddPo
  def self.handle_po file_name
    file_name = File.absolute_path file_name
    content   = IO.
      readlines(file_name).
      map(&:chomp)

    language = content.
      map { |line| /^\" Language: \s+ ([a-z_]+)/ix.match(line) ? $1 : nil }.
      compact.
      first

    fail "Couldn't extract language from #{file_name}" unless language

    target = "#{$po_dir}/#{language}.po"

    File.open(target, "w") { |file| file.puts content.map(&:chomp).join("\n") }
    File.unlink file_name
    File.chmod 0644, target

    puts "#{file_name} → #{target}"
  end

  def self.unpack_rar file_name
    system "unrar x #{Shellwords.escape(file_name)}"
    fail "unrar failed for #{file_name}" unless $?.exitstatus == 0
  end

  def self.unpack_zip file_name
    system "unzip #{Shellwords.escape(file_name)}"
    fail "unzip failed for #{file_name}" unless $?.exitstatus == 0
  end

  def self.handle_archive file_name, archive_type
    Dir.mktmpdir do |dir|
      Dir.chdir(dir) do
        send "unpack_#{archive_type}".to_sym, file_name
        Dir["**/*.po"].each { |unpacked_file| handle_po unpacked_file }
      end
    end

    File.unlink file_name
  end

  def self.add file_name
    return handle_archive(file_name, :rar) if /\.rar$/.match file_name
    return handle_archive(file_name, :zip) if /\.zip$/.match file_name
    return handle_po(file_name)            if /\.po$/.match file_name
    fail "Don't know how to handle #{file_name}"
  end
end

begin
  # Running under Rake?
  Rake
rescue
  ARGV.each { |file_name| AddPo::add File.absolute_path(file_name) }
end
