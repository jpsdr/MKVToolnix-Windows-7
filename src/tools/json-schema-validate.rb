#!/usr/bin/env ruby

require 'json'
require 'jschema'

fail "Required parameters: <schema> <file to validate>" if ARGV.count != 2

content     = JSON.parse File.read(ARGV[1])
schema_data = JSON.parse File.read(ARGV[0])
schema      = JSchema.build(schema_data)
errors      = schema.validate content

exit 0 if errors.empty?

puts errors.join("\n")

exit 1
