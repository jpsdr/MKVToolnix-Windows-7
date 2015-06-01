#!/usr/bin/ruby -w

# T_497crash_in_base64_decoder
describe "mkvmerge / crash in the Base 64 decoder"
test_merge "--tags 0:data/text/tags-crash-invalid-memory-access.xml data/subtitles/srt/ven.srt"
