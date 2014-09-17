#!/usr/bin/ruby -w

# T_439pcm_in_m2ts
describe "mkvmerge / PCM in M2TS"

%w{pcm_48khz_2ch_16bit pcm_48khz_5.1ch_32bit}.each do |file|
  test_merge "data/ts/#{file}.m2ts"
end
