# coding: utf-8

module Mtx::Installer
  def self.read_translation_file file_name
    # Local Variables:
    # mode: nsi
    # coding: windows-1252-unix
    # End:

    all_lines    = IO.readlines(file_name, $/, :encoding => "ASCII-8BIT").map(&:chomp)
    language     = all_lines.first  { |line| %r{^!define \s+ LANG \s+}x.match line }
    emacs        = all_lines.select { |line| %r{^\# \s+ (Local \s+ Variables|mode|coding|End) :}ix.match line }
    translations = Hash[
      *all_lines.
      map { |line| %r{^!insertmacro \s+ LANG_STRING \s+ ([^\s]+) \s+ "(.+)" $}x.match(line) ? [ $1, $2 ] : nil }.
      reject(&:nil?).
      flatten
    ]

    return {
      :language     => language,
      :translations => translations,
      :emacs        => emacs,
    }
  end

  def self.update_one_translation_file file_name, english
    puts_qaction "nsh_update", file_name

    language        = self.read_translation_file file_name
    translation_lines = []
    prefix            = "!insertmacro LANG_STRING "

    english[:translations].keys.sort.each do |name|
      english_text    = english[:translations][name]
      language_text = language[:translations][name].to_s

      if language_text.empty? || (language_text == english_text)
        translation_lines += [
          "# The next entry needs translations:",
          "#{prefix}#{name} \"#{english_text}\""
        ]
      else
        translation_lines << "#{prefix}#{name} \"#{language_text}\""
      end
    end

    all_lines = [ language[:language] ] +
      [ "" ] +
      translation_lines +
      [ "" ] +
      language[:emacs]

    IO.write(file_name, all_lines.join("\n") + "\n")
  end

  def self.update_all_translation_files
    english = self.read_translation_file "installer/translations/English.nsh"

    FileList["installer/translations/*.nsh"].to_a.sort.each do |file_name|
      next if %r{/English\.nsh$}.match file_name
      # next unless %r{German}.match file_name
      self.update_one_translation_file file_name, english
    end
  end
end
