#!/usr/bin/ruby -w

# T_688opus_single_page
describe "mkvmerge / Opus with a single page"
test_merge "data/opus/single_page.opus", :args => "--disable-lacing"
