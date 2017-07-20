#!/usr/bin/ruby -w

# T_606aac_960_samples_per_frame
files = %w{aac_isombff_960.m4a  aac_latm_960.aac}

describe "mkvmerge / AAC with 960 samples per frame"

files.each { |file| test_merge "data/aac/#{file}" }
