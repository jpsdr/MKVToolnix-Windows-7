#!/usr/bin/env ruby

require 'rubygems'
require 'pp'
require 'builder'
require 'rexml/document'
require 'trollop'

require_relative 'modules/changelog'

def rewrap text
  lines = []
  line  = []
  len   = 0
  parts = text.split(%r{\s+})

  while !parts.empty?
    part = parts.shift

    if !line.empty? && ((len + 1 + part.length) > 72)
      lines << line.join(' ')
      line   = []
      len    = 0
    end

    len  += part.length
    line << part
  end

  lines << line.join(' ')

  ([ "* #{lines.shift}" ] + lines.map { |l| "  #{l}"}).join("\n")
end

def create_news changelog
  releases_xml = REXML::Document.new File.new($opts[:releases])

  node_to_hash = lambda { |xpath|
    begin
      Hash[ *REXML::XPath.first(releases_xml, xpath).elements.collect { |e| [ e.name, e.text ] }.flatten ]
    rescue
      Hash.new
    end
  }

  category_labels = [
    "Important notes",
    "New features and enhancements",
    "Bug fixes",
    "Build system changes",
    "Other changes",
  ]

  changelog.each do |cl_release|
    categories = {}
    attributes = { "version" => cl_release[0] }.merge node_to_hash.call("/mkvtoolnix-releases/release[version='#{cl_release[0]}']")

    line  = "# Version #{attributes['version']}"
    line += %Q{ "#{attributes['codename']}"} if attributes['codename'] && !attributes['codename'].empty?
    line += " #{attributes['date']}"

    puts "#{line}\n\n"

    cl_release[1].each do |entry|
      content = entry[:content]

      next if /^Release/.match(content)

      category = case
                 when %r{deprecat|note:}i.match(content)                                       then 0
                 when %r{bug\s*fix|fixe[ds]}i.match(content)                                   then 2
                 when %r{build\s*system|build.*:}i.match(content)                              then 3
                 when %r{new.*feature|enhancement|improvement|implement(s|ed)}i.match(content) then 1
                 when %r{add(ed)?.*(support|option|translation)}i.match(content)               then 1
                 when %r{fix}i.match(content)                                                  then 2
                 else                                                                               4
                 end

      categories[category] ||= []
      categories[category]  << rewrap(content)
    end

    category_labels.each_with_index do |label, idx|
      next unless categories[idx]

      puts "## #{label}\n\n"
      puts categories[idx].join("\n") + "\n\n"
    end

    puts
  end

end

def parse_opts
  opts = Trollop::options do
    opt :changelog, "ChangeLog source file",    :type => String, :default => 'ChangeLog'
    opt :releases,  "releases.xml source file", :type => String, :default => 'releases.xml'
  end

  Trollop::die :changelog, "must be given and exist" if !opts[:changelog] || !File.exist?(opts[:changelog])
  Trollop::die :releases,  "must be given and exist" if !opts[:releases]  || !File.exist?(opts[:releases])

  opts
end

def main
  $opts = parse_opts
  create_news parse_changelog($opts[:changelog])
end

main
