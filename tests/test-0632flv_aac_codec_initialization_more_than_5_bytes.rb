#!/usr/bin/ruby -w

# T_632flv_aac_codec_initialization_more_than_5_bytes
describe "mkvmerge / FLV reader: AAC codec initialization with more than 5 bytes"
test_merge "data/flv/aac-codec-initialization-more-than-5-bytes.flv"
