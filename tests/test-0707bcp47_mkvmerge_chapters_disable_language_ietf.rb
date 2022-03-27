#!/usr/bin/ruby -w

# T_707bcp47_mkvmerge_chapters_disable_language_ietf
describe "mkvmerge / BCP 47/RFC 5646 language tags: chapters (--disable-language-ietf)"

src1   = "data/subtitles/srt/vde-utf-8-bom.srt"
src2   = "data/avi/v.avi"
chaps1 = "data/chapters/shortchaps-utf8.txt"
chaps2 = "data/chapters/uk-and-gb.xml"
chaps3 = "data/chapters/uk-and-gb-chaplanguageietf.xml"

# simple chapters
test_merge src1, :keep_tmp => true, :args => "--disable-language-ietf --chapter-charset utf-8 --chapters #{chaps1}"
compare_languages_chapters(*([ [ "eng", nil ] ] * 5))

test_merge src1, :keep_tmp => true, :args => "--disable-language-ietf --chapter-language de --chapter-charset utf-8 --chapters #{chaps1}"
compare_languages_chapters(*([ [ "ger", nil] ] * 5))

test_merge src1, :keep_tmp => true, :args => "--disable-language-ietf --chapter-language de-latn-ch --chapter-charset utf-8 --chapters #{chaps1}"
compare_languages_chapters(*([ [ "ger", nil ] ] * 5))

# XML chapters
test_merge src1, :keep_tmp => true, :args => "--disable-language-ietf --chapters #{chaps2}"
compare_languages_chapters(*([ [ "eng", nil ] ] * 3 + [ [ "ger", nil ] ] * 3))

test_merge src1, :keep_tmp => true, :args => "--disable-language-ietf --chapter-language de --chapters #{chaps2}"
compare_languages_chapters(*([ [ "eng", nil ] ] * 3 + [ [ "ger", nil ] ] * 3))

test_merge src1, :keep_tmp => true, :args => "--disable-language-ietf --chapter-language de-latn-ch --chapters #{chaps2}"
compare_languages_chapters(*([ [ "eng", nil ] ] * 3 + [ [ "ger", nil ] ] * 3))

# chapter generation
test_merge src2, :keep_tmp => true, :args => "--disable-language-ietf --no-audio --generate-chapters interval:10s"
compare_languages_chapters(*([ [ "eng", nil ] ] * 7))

test_merge src2, :keep_tmp => true, :args => "--disable-language-ietf --no-audio --chapter-language de --generate-chapters interval:10s"
compare_languages_chapters(*([ [ "ger", nil ] ] * 7))

test_merge src2, :keep_tmp => true, :args => "--disable-language-ietf --no-audio --chapter-language de-latn-ch --generate-chapters interval:10s"
compare_languages_chapters(*([ [ "ger", nil ] ] * 7))

test_merge src1, :keep_tmp => true, :args => "--disable-language-ietf --chapters #{chaps3}"
compare_languages_chapters(*([ [ "eng", nil ] ] * 3 + [ [ "ger", nil ] ] * 3))


# simple chapters
test_merge src1, :keep_tmp => true, :args => "--chapter-charset utf-8 --chapters #{chaps1}", :post_args => "--disable-language-ietf"
compare_languages_chapters(*([ [ "eng", nil ] ] * 5))

test_merge src1, :keep_tmp => true, :args => "--chapter-language de --chapter-charset utf-8 --chapters #{chaps1}", :post_args => "--disable-language-ietf"
compare_languages_chapters(*([ [ "ger", nil] ] * 5))

test_merge src1, :keep_tmp => true, :args => "--chapter-language de-latn-ch --chapter-charset utf-8 --chapters #{chaps1}", :post_args => "--disable-language-ietf"
compare_languages_chapters(*([ [ "ger", nil ] ] * 5))

# XML chapters
test_merge src1, :keep_tmp => true, :args => "--chapters #{chaps2}", :post_args => "--disable-language-ietf"
compare_languages_chapters(*([ [ "eng", nil ] ] * 3 + [ [ "ger", nil ] ] * 3))

test_merge src1, :keep_tmp => true, :args => "--chapter-language de --chapters #{chaps2}", :post_args => "--disable-language-ietf"
compare_languages_chapters(*([ [ "eng", nil ] ] * 3 + [ [ "ger", nil ] ] * 3))

test_merge src1, :keep_tmp => true, :args => "--chapter-language de-latn-ch --chapters #{chaps2}", :post_args => "--disable-language-ietf"
compare_languages_chapters(*([ [ "eng", nil ] ] * 3 + [ [ "ger", nil ] ] * 3))

# chapter generation
test_merge src2, :keep_tmp => true, :args => "--no-audio --generate-chapters interval:10s", :post_args => "--disable-language-ietf"
compare_languages_chapters(*([ [ "eng", nil ] ] * 7))

test_merge src2, :keep_tmp => true, :args => "--no-audio --chapter-language de --generate-chapters interval:10s", :post_args => "--disable-language-ietf"
compare_languages_chapters(*([ [ "ger", nil ] ] * 7))

test_merge src2, :keep_tmp => true, :args => "--no-audio --chapter-language de-latn-ch --generate-chapters interval:10s", :post_args => "--disable-language-ietf"
compare_languages_chapters(*([ [ "ger", nil ] ] * 7))

test_merge src1, :keep_tmp => true, :args => "--chapters #{chaps3}", :post_args => "--disable-language-ietf"
compare_languages_chapters(*([ [ "eng", nil ] ] * 3 + [ [ "ger", nil ] ] * 3))
