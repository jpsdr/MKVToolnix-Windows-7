#!/usr/bin/ruby -w

# T_646ogg_opus_gap_page_missing
describe "mkvmerge / Ogg Opus gap in stream due to missing page"
test_merge "data/opus/gap-page-missing.opus"
