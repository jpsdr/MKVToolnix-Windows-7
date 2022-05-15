#!/usr/bin/ruby -w

# T_0217file_identification

describe "mkvmerge / file identification / in(*)"

test "identification" do
  checksum = [
    "data/avi/v.avi",
    "data/bugs/From_Nero_AVC_Muxer.mp4",
    "data/mkv/complex.mkv",
    "data/mkv/vobsubs.mks",
    "data/mp4/test_2000_inloop.mp4",
    "data/mp4/test_mp2.mp4",
    "data/ogg/v.flac.ogg",
    "data/ogg/v.ogg",
    "data/rm/rv3.rm",
    "data/rm/rv4.rm",
    "data/ac3/misdetected_as_mp2.ac3",
    "data/ac3/misdetected_as_mpeges.ac3",
    "data/aac/v.aac",
    "data/ac3/v.ac3",
    "data/flac/v.flac",
    "data/simple/v.mp3",
    "data/wav/v.wav",
    "data/subtitles/ssa-ass/fe.ssa",
    "data/subtitles/srt/vde.srt",
    "data/vobsub/ally1-short.sub",
    "data/wp/with-correction.wv",
    "data/wp/without-correction.wv"
  ].collect do |file|
    json = identify_json(file)
    json.delete("identification_format_version")
    json.to_json.md5
  end

  checksum.join '-'
end
