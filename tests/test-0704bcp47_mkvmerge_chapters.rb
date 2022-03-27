#!/usr/bin/ruby -w

# T_704bcp47_mkvmerge_chapters
describe "mkvmerge / BCP 47/RFC 5646 language tags: chapters"

src1   = "data/subtitles/srt/vde-utf-8-bom.srt"
src2   = "data/avi/v.avi"
chaps1 = "data/chapters/shortchaps-utf8.txt"
chaps2 = "data/chapters/uk-and-gb.xml"
chaps3 = "data/chapters/uk-and-gb-chaplanguageietf.xml"

# simple chapters
test_merge src1, :keep_tmp => true, :args => "--chapter-charset utf-8 --chapters #{chaps1}"
compare_languages_chapters(*([ %w{eng en} ] * 5))

test_merge src1, :keep_tmp => true, :args => "--chapter-language de --chapter-charset utf-8 --chapters #{chaps1}"
compare_languages_chapters(*([ %w{ger de} ] * 5))

test_merge src1, :keep_tmp => true, :args => "--chapter-language de-latn-ch --chapter-charset utf-8 --chapters #{chaps1}"
compare_languages_chapters(*([ %w{ger de-Latn-CH} ] * 5))

# XML chapters
test_merge src1, :keep_tmp => true, :args => "--chapters #{chaps2}"
compare_languages_chapters(*([ [ "eng", "en-GB" ], [ "eng", "en-GB", ], [ "eng", "en-US" ] ] + [ [ "ger", "de-DE" ] ] * 3))

test_merge src1, :keep_tmp => true, :args => "--chapter-language de --chapters #{chaps2}"
compare_languages_chapters(*([ [ "eng", "en-GB" ], [ "eng", "en-GB", ], [ "eng", "en-US" ] ] + [ [ "ger", "de-DE" ] ] * 3))

test_merge src1, :keep_tmp => true, :args => "--chapter-language de-latn-ch --chapters #{chaps2}"
compare_languages_chapters(*([ [ "eng", "en-GB" ], [ "eng", "en-GB", ], [ "eng", "en-US" ] ] + [ [ "ger", "de-DE" ] ] * 3))

# chapter generation
test_merge src2, :keep_tmp => true, :args => "--no-audio --generate-chapters interval:10s"
compare_languages_chapters(*([ %w{eng en} ] * 7))

test_merge src2, :keep_tmp => true, :args => "--no-audio --chapter-language de --generate-chapters interval:10s"
compare_languages_chapters(*([ %w{ger de} ] * 7))

test_merge src2, :keep_tmp => true, :args => "--no-audio --chapter-language de-latn-ch --generate-chapters interval:10s"
compare_languages_chapters(*([ %w{ger de-Latn-CH} ] * 7))

# XML chapters with ChapLanguageIETF
test_merge src1, :keep_tmp => true, :args => "--chapters #{chaps3}"
compare_languages_chapters(*([ [ "eng", "en-GB" ], [ "eng", "en-GB" ], [ "eng", "en-US" ] ]  + [ [ "ger", "de-DE" ] ] * 3))

test_merge src1, :keep_tmp => true, :args => "--chapter-language de --chapters #{chaps3}"
compare_languages_chapters(*([ [ "eng", "en-GB" ], [ "eng", "en-GB" ], [ "eng", "en-US" ] ]  + [ [ "ger", "de-DE" ] ] * 3))

test_merge src1, :keep_tmp => true, :args => "--chapter-language de-latn-ch --chapters #{chaps3}"
compare_languages_chapters(*([ [ "eng", "en-GB" ], [ "eng", "en-GB" ], [ "eng", "en-US" ] ]  + [ [ "ger", "de-DE" ] ] * 3))
