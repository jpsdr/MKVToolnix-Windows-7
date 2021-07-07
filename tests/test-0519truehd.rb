#!/usr/bin/ruby -w

# T_519truehd
describe "mkvmerge / TrueHD from elementary streams and Matroska"

base = tmp
test_merge "data/truehd/blueplanet.thd", output: "#{base}-1", keep_tmp: true
test_merge "#{base}-1",                  output: "#{base}-2"
