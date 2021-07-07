#!/usr/bin/ruby -w

# T_461truehd_from_mpeg_ts
describe "mkvmerge / TrueHD+AC-3 from MPEG transport streams"

test_merge "data/ts/blue_planet.ts", :args => "-D -a 2,3"
test_merge "data/ts/blue_planet.ts", :args => "-D -a 2"
test_merge "data/ts/blue_planet.ts", :args => "-D -a 3"
