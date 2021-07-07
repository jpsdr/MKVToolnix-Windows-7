#!/usr/bin/ruby -w

# T_514remove_track_statistics_tags_during_remux
describe "mkvmerge / removing track statistics tags during remux"

base = tmp

test_merge "data/avi/v.avi", :output => "#{base}-1", :keep_tmp => true
test_merge "#{base}-1",      :output => "#{base}-2", :keep_tmp => true
test_merge "#{base}-1",      :output => "#{base}-3", :keep_tmp => true, :args => '--disable-track-statistics-tags'
