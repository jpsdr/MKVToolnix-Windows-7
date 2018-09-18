#!/usr/bin/ruby -w

# T_651sync_chapter_timestamps
describe "mkvmerge / syncing chapter timestamps"

sources = [
  [ "data/ogg/with_chapters.ogm", "--chapter-charset ISO-8859-15" ],
  "data/mp4/o12-short.m4v",
  "data/mkv/chapters-with-ebmlvoid.mkv",
]

sources.each do |source|
  source = [ source, "" ] unless source.is_a? Array

  test_merge source[0], :args => "#{source[1]}"

  [ "", "-" ].each do |sign|
    [ -1, -2 ].each { |id| test_merge source[0], :args => "#{source[1]} --sync #{id}:#{sign}1000,3/2" }
    test_merge source[0], :args => "#{source[1]} --chapter-sync #{sign}1000,3/2"
  end
end

[ "",
  "--chapter-sync 1000,3/2",  "--sync -1:1000,3/2",  "--sync -2:1000,3/2",
  "--chapter-sync -1000,3/2", "--sync -1:-1000,3/2", "--sync -2:-1000,3/2",
].each do |args|
  test_merge "data/subtitles/srt/ven.srt", :args => "#{args} --chapters data/chapters/uk-and-gb.xml"
end
