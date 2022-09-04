#!/usr/bin/ruby -w

# T_746ogg_unknown_track_and_kate_missing_headers
describe "mkvmerge / Ogg: stream of unknown/unsupported type and a Kate stream missing its headers"

test_merge "data/ogg/unknown_stream_and_kate_without_headers.ogv"
