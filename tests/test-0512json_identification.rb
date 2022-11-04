#!/usr/bin/ruby -w

files = %w{
  data/aac/v.aac
  data/ac3/v.ac3
  data/subtitles/ssa-ass/fonts.ass
  data/vobsub/ally1-short.idx
  data/ts/timecode-overflow.m2ts
  data/mp4/aac_encoder_delay_sample.m4a
  data/mp4/o12-short.m4v
  data/mkv/complex.mkv
  data/pcm/big-endian.mka
  data/ogg/video-1.1.ogv
  data/vc1/MC.track_4113.vc1
  data/vob/pcm-48kHz-2ch-16bit.vob
  data/webm/yt3.webm
  data/wp/with-correction.wv
  data/wp/with-correction.wvc

  data/truehd/truehd-atmos+ac3.m2ts
  data/ts/two-teletext-pages-in-single-ts-track.ts
}

describe "mkvmerge / JSON identification format"

test "identification and validation" do
  hashes = []

  files.each do |file|
    output, _     = identify file, :format => :json
    output        = output.join ''
    json          = JSON.load(output)

    valid, errors = json_schema_validate(json)

    if !valid
      puts " JSON validation errors in #{file}:"
      puts errors.map { |err| "  #{err}" }.join("\n")
    end

    json.delete("identification_format_version")
    json_md5 = JSON.dump(json).md5

    hashes << "#{json_md5}+#{valid ? "ok" : "invalid"}"
  end

  hashes.join '-'
end
