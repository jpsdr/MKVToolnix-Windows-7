#!/usr/bin/ruby -w

# T_370propedit_attachments
describe "mkvpropedit / attachments"

sources = [
  { :file => "chap1.txt",      :name => "Dummy File.txt",           :mime_type => "text/plain",      :description => "Some funky description" },
  { :file => "chap2.txt",      :name => "Magic: The Gathering.ttf", :mime_type => "font/ttf",        :description => "This is a Font, mon!"   },
  { :file => "shortchaps.txt", :name => "Dummy File.txt",           :mime_type => "some:thing:else", :description => "Muh"                    },
  { :file => "tags.xml",       :name => "Hitme.xml",                :mime_type => "text/plain",      :description => "Gonzo"                  },
]

small_file = "data/text/tags.xml"

commands = [
  "--delete-attachment 2",
  "--delete-attachment =2",
  "--delete-attachment 'name:Magic: The Gathering.ttf'",
  "--delete-attachment mime-TYPE:text/plain",

  "--add-attachment #{small_file}",
  "--attachment-name 'Hallo! Die Welt....doc' --add-attachment #{small_file}",
  "--attachment-mime-type fun/go --add-attachment #{small_file}",
  "--attachment-description 'Alice and Bob see Charlie' --add-attachment #{small_file}",
  "--attachment-name 'Hallo! Die Welt....doc' --attachment-mime-type fun/go --attachment-description 'Alice and Bob see Charlie' --add-attachment #{small_file}",

  "--replace-attachment 3:#{small_file}",
  "--replace-attachment =3:#{small_file}",
  "--replace-attachment 'name:Magic\\c The Gathering.ttf:#{small_file}'",
  "--replace-attachment mime-type:text/plain:#{small_file}",

  "--attachment-name 'Hallo! Die Welt....doc' --attachment-mime-type fun/go --attachment-description 'Alice and Bob see Charlie' --replace-attachment 3:#{small_file}",
  "--attachment-name 'Hallo! Die Welt....doc' --attachment-mime-type fun/go --attachment-description 'Alice and Bob see Charlie' --replace-attachment =3:#{small_file}",
  "--attachment-name 'Hallo! Die Welt....doc' --attachment-mime-type fun/go --attachment-description 'Alice and Bob see Charlie' --replace-attachment 'name:Magic\\c The Gathering.ttf:#{small_file}'",
  "--attachment-name 'Hallo! Die Welt....doc' --attachment-mime-type fun/go --attachment-description 'Alice and Bob see Charlie' --replace-attachment mime-type:text/plain:#{small_file}",

  "--attachment-description 'Alice and Bob see Charlie' --replace-attachment mime-type:text/plain:#{small_file}",

  "--attachment-name 'bla blubb.txt' --attachment-description 'a real description' --attachment-mime-type 'gon/zo' --update-attachment 3",
  "--attachment-name 'bla blubb.txt' --attachment-description 'a real description' --attachment-mime-type 'gon/zo' --replace-attachment 3:#{small_file}",

  "--attachment-name 'bla blubb.txt' --attachment-description 'a real description' --attachment-mime-type 'gon/zo' --attachment-uid 47110815 --update-attachment 3",
  "--attachment-name 'bla blubb.txt' --attachment-description 'a real description' --attachment-mime-type 'gon/zo' --attachment-uid 47110815 --replace-attachment 3:#{small_file}",
  "--attachment-name 'bla blubb.txt' --attachment-description 'a real description' --attachment-mime-type 'gon/zo' --attachment-uid 47110815 --add-attachment #{small_file}",
]

test "several" do
  hashes          = []
  src             = "#{tmp}-src"
  work            = "#{tmp}-work"
  initial_command = "--sub-charset 0:iso-8859-15 data/subtitles/srt/vde.srt " +
    sources.collect { |s| "--attachment-name '#{s[:name]}' --attachment-mime-type '#{s[:mime_type]}' --attachment-description '#{s[:description]}' --attach-file 'data/text/#{s[:file]}'" }.
    join(' ')

  merge initial_command, :keep_tmp => true, :output => src
  hashes << hash_file(src)

  commands.each do |command|
    sys "cp #{src} #{work}"
    hashes << hash_file(work)
    propedit work, command
    hashes << hash_file(work)
  end

  hashes.join '-'
end
