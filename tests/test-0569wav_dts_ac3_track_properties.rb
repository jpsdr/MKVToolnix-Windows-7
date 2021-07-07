#!/usr/bin/ruby -w

# T_569wav_dts_ac3_track_properties
describe "mkvmerge / track properties for DTS & AC-3 in WAV"

%w{ac3/shortsample.ac3.wav wav/05-labyrinth-path-13.wav wav/06-labyrinth-path-14.wav wav/dts-sample.dts.wav}.each do |file_name|
  test_identify "data/#{file_name}"
end
