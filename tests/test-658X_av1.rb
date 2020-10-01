#!/usr/bin/ruby -w

# T_658X_av1
describe "mkvextract / AV1 to IVF"

test "extraction to IVF" do
  extract "data/av1/av1.webm", 0 => "#{tmp}-1"
  merge "#{tmp}-1", :output => "#{tmp}-2"

  (1..2).map { |idx| hash_file("#{tmp}-#{idx}") }.join('-')
end
