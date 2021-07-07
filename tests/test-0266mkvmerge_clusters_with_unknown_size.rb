#!/usr/bin/ruby -w

describe "mkvmerge / Clusters with an unknown size"

test_merge "data/webm/live-stream.webm", :exit_code => :warning
