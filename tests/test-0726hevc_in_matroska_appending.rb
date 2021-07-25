#!/usr/bin/ruby -w

# T_726hevc_in_matroska_appending
describe "appending HEVC/H.265 in Matroska, ensuring all data is passed to the correct parser instance"

files = Dir.glob("data/h265/issue-3170-sample-source-file-*").sort

test_merge "#{files.join(' + ')}"
