#!/usr/bin/ruby -w

# T_444pcm_statistics_from_packaged_sources
describe "mkvmerge / PCM track statistics when reading from packaged sources"

src_file = "data/wav/v.wav"

base = tmp

test_merge src_file,    :output => "#{base}-1", :keep_tmp => true, :args => "--disable-lacing"
test_merge "#{base}-1", :output => "#{base}-2", :keep_tmp => true, :args => "--disable-lacing"
test_merge src_file,    :output => "#{base}-3", :keep_tmp => true
test_merge "#{base}-3", :output => "#{base}-4", :keep_tmp => true

test "tag_bps" do
  (1..4).collect do |idx|
    identify("#{base}-#{idx}")[0].
      select  { |line| /tag_bps:/.match(line) }.
      collect { |line| line.chomp.gsub(/.*tag_bps:(\d+).*/, '\1') }.
      join('')
  end.join('+')
end
