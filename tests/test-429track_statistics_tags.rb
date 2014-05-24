#!/usr/bin/ruby -w

describe "mkvmerge / track tag statistics"
['', '--tags 0:data/text/tags-as-tags.xml'].each do |track_tags|
  ['', '--disable-track-statistics-tags'].each do |statistics|
    test_merge "data/avi/v.avi", :args => "#{track_tags} #{statistics}"
  end
end
