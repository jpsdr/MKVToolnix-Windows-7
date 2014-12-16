#!/usr/bin/ruby -w

describe "mkvinfo / Clusters with an unknown size"
test_info "data/webm/live-stream.webm", :args => "-v -v -z"
