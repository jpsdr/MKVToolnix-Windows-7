#!/usr/bin/ruby -w

# T_474quicktime_rpza
describe "mkvmerge / QuickTime with rpza (Road Pizza) video and PCM audio"

(1..2).each { |idx| test_merge "data/mp4/rpza-pcm-#{idx}.mov" }
