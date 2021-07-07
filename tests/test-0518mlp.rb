#!/usr/bin/ruby -w

# T_518mlp
describe "mkvmerge / MLP from elementary streams and Matroska"

base = tmp
test_merge "data/truehd/god-save-the-queen.mlp", output: "#{base}-1", keep_tmp: true
test_merge "#{base}-1",                          output: "#{base}-2"
