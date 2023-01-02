#!/usr/bin/env ruby
require "fileutils"
require "pathname"
require "rake"
require "shellwords"
require "tmpdir"

$mtx_dir    = File.absolute_path(File.dirname(__FILE__) + "/../..")
$po_dir     = "#{$mtx_dir}/po"
$man_po_dir = "#{$mtx_dir}doc/man/po4a/po"

require "#{$mtx_dir}/rake.d/extensions"
require "#{$mtx_dir}/rake.d/helpers"
require "#{$mtx_dir}/rake.d/po"

module AddPo
  def self.handle_po file_name
    file_name = File.absolute_path(file_name)
    Dir.chdir($mtx_dir) { self.handle_po_impl file_name }
  end

  def self.handle_po_impl file_name
    file_name = File.absolute_path file_name
    content   = IO.
      readlines(file_name).
      map(&:chomp)

    language = content.
      map { |line| /^\" Language: \s+ ([a-z_.@]+)/ix.match(line) ? $1 : nil }.
      compact.
      first

    fail "Couldn't extract language from #{file_name}" unless language

    is_man_po = content.detect { |line| /<manvolnum>/.match line }
    dir       = is_man_po ? $man_po_dir : $po_dir
    target    = "#{dir}/#{language}.po"

    puts "#{file_name} → #{target}"

    runq_git target, "checkout HEAD -- #{target}"

    orig_items = read_po target

    puts_qaction "merge", :target => target

    updated_items = read_po file_name
    merged_items  = merge_po orig_items, updated_items, :headers_to_update => %w{Project-Id-Version}

    write_po target, merged_items

    File.unlink file_name
  end

  def self.unpack_p7z file_name
    system "7z x #{Shellwords.escape(file_name)}"
    fail "7z failed for #{file_name}" unless $?.exitstatus == 0
  end

  def self.unpack_rar file_name
    system "unrar x #{Shellwords.escape(file_name)}"
    fail "unrar failed for #{file_name}" unless $?.exitstatus == 0
  end

  def self.unpack_zip file_name
    system "unzip #{Shellwords.escape(file_name)}"
    fail "unzip failed for #{file_name}" unless $?.exitstatus == 0
  end

  def self.unpack_tar file_name
    system "bsdtar xvf #{Shellwords.escape(file_name)}"
    fail "bsdtar failed for #{file_name}" unless $?.exitstatus == 0
  end

  def self.handle_qm file_name
    Dir.mktmpdir do |dir|
      tm_name = Pathname.new(file_name).basename.sub_ext(".tm").to_s
      system "lconvert -i #{Shellwords.escape(file_name)} -o #{Shellwords.escape(tm_name)}"

      fail "lconvert failed for #{file_name}" unless $?.exitstatus == 0

      handle_ts tm_name

      File.unlink file_name
    end
  end

  def self.handle_ts file_name, language = nil
    content   = IO.
      readlines(file_name).
      map(&:chomp)

    base     = nil
    language = content.
      map { |line| /<TS [^>]* language="([a-z_@]+)"/ix.match(line) ? $1 : nil }.
      compact.
      first

    if !language
      language       = ENV['TS'] if ENV['TS'] && !ENV['TS'].empty?
      base, language = $1, $2    if !language && /qt(base)?_([a-zA-Z_@]+)/.match(file_name)
    end

    fail "Unknown language for Qt TS file #{file_name} (set TS)" if !language

    base ||= ''
    target = "#{$po_dir}/qt/qt#{base}_#{language}.ts"

    if !FileTest.exist?(target) && /^([a-z]+)_[a-z]+/i.match(language)
      target = "#{$po_dir}/qt/qt#{base}_#{$1}.ts"
    end

    fail "target file does not exist yet: #{target} (wrong language?)" if !FileTest.exist?(target)

    File.open(target, "w") { |file| file.puts content.map(&:chomp).join("\n") }
    File.unlink file_name
    File.chmod 0644, target
  end

  def self.handle_archive file_name, archive_type
    Dir.mktmpdir do |dir|
      Dir.chdir(dir) do
        send "unpack_#{archive_type}".to_sym, file_name
        Dir["**/*.po"].each { |unpacked_file| handle_po unpacked_file }
        Dir["**/*.qm"].each { |unpacked_file| handle_qm unpacked_file }
        Dir["**/*.ts"].each { |unpacked_file| handle_ts unpacked_file }
      end
    end

    File.unlink file_name
  end

  def self.add file_name
    return handle_archive(file_name, :rar) if /\.rar$/i.match file_name
    return handle_archive(file_name, :zip) if /\.zip$/i.match file_name
    return handle_archive(file_name, :p7z) if /\.7z$/i.match  file_name
    return handle_archive(file_name, :tar) if /\.tar(?:\.(?:gz|bz2|xz))?$/i.match  file_name
    return handle_po(file_name)            if /\.po$/i.match  file_name
    return handle_qm(file_name)            if /\.qm$/i.match  file_name
    return handle_ts(file_name)            if /\.ts$/i.match  file_name
    fail "Don't know how to handle #{file_name}"
  end
end

ARGV.each { |file_name| AddPo::add File.absolute_path(file_name) }
