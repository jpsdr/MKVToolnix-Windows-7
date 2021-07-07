#!/usr/bin/ruby -w

# T_630propedit_add_statistics_tags_with_compression
describe "mkvpropedit / adding track statistics tags for compressed tracks (content encodings)"

test_merge "data/subtitles/srt/ven.srt", :args => "--compression 0:zlib --disable-track-statistics-tags", :keep_tmp => true
test "adding track statistics tags" do
  propedit tmp, "--add-track-statistics-tags"
  sys("../src/mkvextract #{tmp} tags -")[0].
    select { |line| %r{<String>[0-9]+</String>}.match(line) }.
    map { |line| line.chomp.gsub(%r{.*<String>(.*)</String>.*}, '\1').to_i }.
    sort.
    join('+')
end
