#!/usr/bin/ruby -w

# T_493truehd_ac3_setting_track_properties_mpeg_ts
describe "mkvmerge / setting track properties for AC-3 cores embedded in TrueHD tracks from MPEG TS"

[
  "--language 2:fre --language 3:ger",
  "--language 2:ger --language 3:fre",
  "--track-name 2:tricky --track-name 3:tracky",
  "--track-name 2:tracky --track-name 3:tricky",
  "--track-order 0:2,0:3",
  "--track-order 0:3,0:2",
].each { |args| test_merge "data/ts/blue_planet_2mb.ts", :args => "--no-video --atracks 2,3 #{args}" }
