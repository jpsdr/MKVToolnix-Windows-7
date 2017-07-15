# coding: utf-8

module Mtx::SourceTests
  def self.collect_header_files
    ui_file_names        = FileList["src/**/*.ui"]
    ui_header_file_names = ui_file_names.collect { |file_name| file_name.ext 'h' }.to_a
    header_file_names    = FileList["src/**/*.h"].to_a.sort

    header_file_names - ui_header_file_names
  end

  def self.expected_guard_name file_name
    "MTX_" + file_name.
      gsub(%r{.*src/}, '').
      gsub(%r{[/.]}, '_').
      upcase
  end

  def self.test_include_guard_for_file file_name, lines
    guard_line_idx = (0...lines.size).select { |idx| %r{^#ifndef +MTX_}.match(lines[idx]) }[0]

    if !guard_line_idx
      puts "#{file_name}:1: error: no include guard line found"
      return false
    end

    ok            = true
    guard_line    = lines[guard_line_idx].chomp
    guard_name    = guard_line.gsub(%r{^#ifndef +}, '').gsub(%r{ .*}, '')
    expected_name = self.expected_guard_name file_name

    if guard_name != expected_name
      puts "#{file_name}:#{guard_line_idx + 1}: error: wrong guard name: #{guard_name}; should be: #{expected_name}"
      ok = false
    end

    guard_def_line    = lines[guard_line_idx + 1].chomp
    expected_def_line = "#define #{expected_name}"

    if guard_def_line != expected_def_line
      puts "#{file_name}:#{guard_line_idx + 2}: error: guard definition line wrong or not found; should be: #{expected_def_line}"
      ok = false
    end

    guard_end_line  = lines.last
    expected_end_re = "^\\#endif \\s+ // \\s+ #{expected_name} $"

    if !%r(#{expected_end_re})x.match(guard_end_line)
      puts "#{file_name}:#{lines.size}: error: guard end line wrong or not found; should match RE: #{expected_end_re}"
      ok = false
    end

    ok
  end

  def self.test_include_guards
    file_names = self.collect_header_files
    content    = read_files file_names
    ok         = true

    file_names.each do |file_name|
      if !self.test_include_guard_for_file(file_name, content[file_name])
        ok = false
      end
    end

    exit 1 if !ok
  end
end
