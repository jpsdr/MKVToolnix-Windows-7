#!/usr/bin/ruby -w

# T_734legacy_language_code_fallback_via_extlang_prefix
describe "mkvmerge, mkvpropedit / legacy language codes derived via falling back on extlang prefix"

test_merge "data/avi/v.avi", :args => "--language 0:bfi --language 1:yue", :keep_tmp => true
test "tracks actual language codes" do
  languages = identify_json(tmp)["tracks"].map { |track| track["properties"]["language"] }
  fail "unexpected legacy language codes" if languages != ["sgn", "chi"]
  languages.join('+')
end

test_merge "data/avi/v.avi", :args => "--chapter-language yue --generate-chapters interval:10s", :keep_tmp => true
test "chapters actual language codes 1" do
  language = extract(tmp, :mode => :chapters).
    join('').
    gsub(%r{[\r\n]+}, '').
    gsub(%r{.*<ChapterLanguage>}, '').
    gsub(%r{<.*}, '')

  fail "unexpected legacy language code" if language != "chi"
  "chi"
end

test_merge "data/avi/v.avi", :args => "--chapters data/chapters/chapters-yue.xml", :keep_tmp => true
test "chapters actual language codes 2" do
  language = extract(tmp, :mode => :chapters).
    join('').
    gsub(%r{[\r\n]+}, '').
    gsub(%r{.*<ChapterLanguage>}, '').
    gsub(%r{<.*}, '')

  fail "unexpected legacy language code" if language != "chi"
  "chi"
end

test_merge "data/avi/v.avi", :keep_tmp => true
test "propedit actual language code" do
  propedit tmp, "--edit track:1 --set language=yue"
  languages = identify_json(tmp)["tracks"].map { |track| track["properties"]["language"] }
  fail "unexpected legacy language codes" if languages != ["chi", "und"]
  languages.join('+')
end
