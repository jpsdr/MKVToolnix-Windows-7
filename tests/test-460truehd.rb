#!/usr/bin/ruby -w

# T_460truehd
describe "mkvmerge / TrueHD"

test_merge "data/truehd/blueplanet.thd"
test_merge "data/truehd/blueplanet.thd", :args => "-a 0"

test_merge "data/truehd/blueplanet.thd+ac3"
test_merge "data/truehd/blueplanet.thd+ac3", :args => "-a 0"
test_merge "data/truehd/blueplanet.thd+ac3", :args => "-a 1"
test_merge "data/truehd/blueplanet.thd+ac3", :args => "-a 0,1"
