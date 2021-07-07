#!/usr/bin/ruby -w

# T_492truehd_ac3_setting_track_properties
describe "mkvmerge / setting track properties for AC-3 cores embedded in TrueHD tracks"

[
  "--language 0:fre --language 1:ger",
  "--language 0:ger --language 1:fre",
  "--track-name 0:tricky --track-name 1:tracky",
  "--track-name 0:tracky --track-name 1:tricky",
  "--track-order 0:0,0:1",
  "--track-order 0:1,0:0",
].each { |args| test_merge "data/truehd/blueplanet.thd+ac3", :args => args }
