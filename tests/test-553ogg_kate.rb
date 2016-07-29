#!/usr/bin/ruby -w

# T_553ogg_kate
describe "mkvmerge / Kate in Ogg"

%w{bom bspline counter demo font granule karaoke kate markup minimal path periodic style unicode utf8test z}.each do |file|
  test_merge "data/subtitles/kate/#{file}.kate.ogg"
end
