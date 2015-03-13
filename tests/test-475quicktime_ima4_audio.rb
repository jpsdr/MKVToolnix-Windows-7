#!/usr/bin/ruby -w

# T_475quicktime_ima4_audio
describe "mkvmerge / QuickTime with ima4 audio"

(1..3).each { |idx| test_merge "data/mp4/cinepak-ima4-#{idx}.mov" }
