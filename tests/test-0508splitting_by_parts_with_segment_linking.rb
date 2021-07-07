#!/usr/bin/ruby -w

# T_508splitting_by_parts_with_segment_linking
describe "mkvmerge / splitting by parts with segment linking enabled"

self.sys "../src/mkvmerge -o #{tmp} data/avi/v-h264-aac.avi --split parts:00:00:00-00:00:10,00:00:30-00:00:40,00:00:50- --link", :keep_tmp => true, :exit_code => :success

test "segment UIDs" do
  uids   = []
  result = []

  (1..3).each do |idx|
    uids << info("#{tmp}-00#{idx}", :output => :return).
      first.
      select { |line| /segment uid/i.match(line) }.
      map    { |line| line.gsub(/^\| *\+ */, '').chomp.downcase.split(/ *uid: */) }.
      to_h
  end

  result << "existence0"

  result <<  uids[0]['previous segment'].nil?
  result << !uids[0]['segment'].nil?
  result << !uids[0]['next segment'].nil?

  result << "existence1"

  result << !uids[1]['previous segment'].nil?
  result << !uids[1]['segment'].nil?
  result << !uids[1]['next segment'].nil?

  result << "existence2"

  result << !uids[2]['previous segment'].nil?
  result << !uids[2]['segment'].nil?
  result <<  uids[2]['next segment'].nil?

  result << "equality_previous"

  result << (uids[0]['segment'] == uids[1]['previous segment'])
  result << (uids[1]['segment'] == uids[2]['previous segment'])

  result << "equality_next"

  result << (uids[1]['segment'] == uids[0]['next segment'])
  result << (uids[2]['segment'] == uids[1]['next segment'])

  result.map(&:to_s).join('-')
end
