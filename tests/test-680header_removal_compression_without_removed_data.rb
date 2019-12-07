#!/usr/bin/ruby -w

# T_680header_removal_compression_without_removed_data
describe "mkvmerge / header removal compression enabled but no removed bytes present in track headers"
test_merge "data/mkv/header_removal_compression_without_removed_data.mkv"
