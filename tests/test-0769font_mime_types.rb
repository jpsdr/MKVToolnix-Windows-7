#!/usr/bin/ruby -w

# T_769font_mime_types
describe "mkvmerge / font MIME types"

def do_test title, options, expected
  file_name = tmp + ".mkv"

  test_merge "data/subtitles/srt/ven.srt", :args => "#{options} --attach-file data/fonts/AndaleMo.TTF --attach-file data/fonts/latinmodern-math.otf", :keep_tmp => true, :output => file_name

  test "#{title} font MIME types" do
    # |  + MIME type: application/vnd.ms-opentype

    all_ok            = true
    result            = [ ]
    actual_mime_types = info(file_name, :output => :return).
      first.
      select { |line| %r{MIME type:}i.match(line) }.
      map    { |line| line.chomp.gsub(%r{.*: *}, '') }

    expected.each_with_index do |expected_mime_type, idx|
      actual_mime_type  = actual_mime_types[idx]
      result           += [ actual_mime_type, actual_mime_type == expected_mime_type ? "OK" : "bad" ]
      all_ok            = false if actual_mime_type != expected_mime_type
    end

    fail "not all OK" if !all_ok

    result.join('+')
  end
end

do_test "current", "", [ "font/ttf", "font/otf" ]
do_test "legacy",  "--enable-legacy-font-mime-types", [ "application/x-truetype-font", "application/vnd.ms-opentype" ]
