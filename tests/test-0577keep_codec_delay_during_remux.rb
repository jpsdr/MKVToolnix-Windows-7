#!/usr/bin/ruby -w

# T_577keep_codec_delay_during_remux
describe "mkvmerge / keep codec delay when remuxing"

test_merge "data/aac/v.aac", :keep_tmp => true

test "keep codec delay during remux" do
  propedit tmp, "--edit track:a1 --set codec-delay=123000456"
  result = [ identify_json(tmp)["tracks"][0]["properties"]["codec_delay"] ]

  merge tmp, :output => "#{tmp}-2"
  result << identify_json("#{tmp}-2")["tracks"][0]["properties"]["codec_delay"]

  result.join('+')
end

test_merge "data/opus/v-opus.ogg", :keep_tmp => true

test "let Opus overwrite even if present" do
  propedit tmp, "--edit track:a1 --set codec-delay=123000456"
  result = [ identify_json(tmp)["tracks"][0]["properties"]["codec_delay"] ]

  merge tmp, :output => "#{tmp}-2"
  result << identify_json("#{tmp}-2")["tracks"][0]["properties"]["codec_delay"]

  result.join('+')
end
