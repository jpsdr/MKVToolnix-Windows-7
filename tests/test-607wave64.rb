#!/usr/bin/ruby -w

files = %w{w64-pcm16.w64 w64-pcm32.w64}

# T_607wave64
describe "mkvmerge / Wave64"

files.each { |file| test_merge "data/wav/#{file}" }
