#!/usr/bin/env ruby

require "json"
require "nokogiri"
require "pp"

fail "No file name" if ARGV.empty?

input  = Nokogiri::XML(`mkvextract tags #{ARGV[0]}`)

new_tags = input.css("Tags Tag").collect do |tag|
  uid = tag.css("Targets TrackUID").first
  next unless uid

  simple_tags = tag.css("Simple").collect do |simple|
    [ simple.css("Name").first.text, simple.css("String").first.text ]
  end

  { uid:  uid.text.to_i,
    tags: Hash[ *simple_tags.sort_by(&:first).flatten ],
  }
end

puts JSON.pretty_generate(new_tags)
