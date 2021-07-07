#!/usr/bin/ruby -w

# T_473quicktime_cinepak_pcm
describe "mkvmerge / QuickTime with Cinepak video and PCM audio"

(1..2).each { |idx| test_merge "data/mp4/cinepak-pcm-#{idx}.mov" }
