#!/usr/bin/ruby -w

# T_730track_selection_by_language_matching
describe "mkvmerge / track selection by language matching"

def track_languages file
  identify_json(file)["tracks"].map { |track| track["properties"]["language_ietf"] }
end

def test_remux src, languages_args, languages_result = nil
  out  = "#{tmp}-2"
  args = languages_args.join(',')

  test_merge "--stracks #{args} #{src}", :output => out, :keep_tmp => true
  test("languages #{args}") { track_languages(out) == (languages_result || languages_args) }
end

src1          = "data/subtitles/srt/ven.srt"
src2          = "#{tmp}-1"
all_languages = %w{de de-CH es es-ES es-MX es-US}
args          = all_languages.
  map { |language| "--language 0:#{language} #{src1}" }.
  join(" ")

test_merge args, :output => src2, :keep_tmp => true
test("languages orig") { track_languages(src2) == all_languages }

test_remux src2, all_languages
test_remux src2, %w{es-ES es-US}
test_remux src2, %w{es-MX}
test_remux src2, %w{!es-MX}, %w{de de-CH es es-ES es-US}
test_remux src2, %w{es}, %w{es es-ES es-MX es-US}
test_remux src2, %w{de}, %w{de de-CH}
test_remux src2, %w{es de}, all_languages
test_remux src2, %w{!de}, %w{es es-ES es-MX es-US}
