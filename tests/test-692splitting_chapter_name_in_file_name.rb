#!/usr/bin/ruby -w

# T_692splitting_chapter_name_in_file_name
describe "mkvmerge / splitting & using %c for chapter names in destination file names"

test "splitting" do
  merge "--chapter-charset UTF-8 --chapters data/chapters/shortchaps-utf8.txt data/avi/v.avi --split chapters:all", :output => "#{tmp}-zZzZz-%03d-%c.mkv"

  Dir.glob("#{tmp}-*mkv").
    map { |name| name.encode("UTF-8").gsub(%r{.*-zZzZz-}, '') }.
    sort.
    join('+').
    md5
end
