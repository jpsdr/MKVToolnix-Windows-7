#!/usr/bin/env ruby

require "pp"
require "shellwords"

if ARGV.size < 2
  puts "Need two arguments: source file name & base name for NALUs"
  exit 1
end

nalus = []

`xyzvc_dump #{Shellwords.escape(ARGV[0])}`.
  split(%r{\n+}).
  each do |line|

  if %r{size (\d+) (?:marker size (\d+) )?at (\d+)}.match(line)
    # NALU type 0x00 (trail_n) size 10609 marker size 4 at 985088 checksum 0x4093bba9
    nalus << [ $1.to_i, $3.to_i + ($2 && !$2.empty? ? $2.to_i : 0) ]
  end
end

src_file     = File.open(ARGV[0], 'rb') || fail("Could not open #{ARGV[0]}")
dst_file_num = 0

nalus.each do |nalu|
  dst_file_name = sprintf ARGV[1], dst_file_num
  dst_file_num += 1
  dst_file      = File.open(dst_file_name, 'wb') || fail("Could not create #{dst_file_name}")

  src_file.seek(nalu[1])
  content = src_file.read(nalu[0])
  dst_file.write(content)
  dst_file.close
end
