#!/usr/bin/ruby -w

# T_557dts_hd_ma_xll_extension
describe "mkvmerge / DTS-HD Master Audio with XLL extensions and sample rate > 48 kHz"

files = %w{96 192}.map { |r| "data/dts/dts-hd-ma-#{r}khz.dts" }

files.each do |file|
  test "detected sample rate of #{file}" do
    identify_json(file)["tracks"].
      map { |t| t["properties"]["audio_sampling_frequency"].to_s }.
      join("+")
  end
end

test_merge files.join(' '), :output => "#{tmp}-1", :keep_tmp => true
test_merge "#{tmp}-1"
