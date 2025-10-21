def format_string_for_po str
  return '"' + str.gsub(/"/, '\"') + '"' unless /\\n./.match(str)

  ([ '""' ] + str.split(/(?<=\\n)/).map { |part| '"' + part.gsub(/"/, '\"') + '"' }).join("\n")
end

def unformat_string_for_po str
  str.gsub(/^"|"$/, '').gsub(/\\"/, '"')
end

def fix_po_msgstr_plurals items
  return items if (items.size == 0) || !items.first.key?(:comments)

  matches = %r{nplurals=(\d+)}.match(items.first[:comments].join(""))

  return items if !matches

  num_plurals = matches[1].to_i

  return items if num_plurals <= 0

  items.each do |item|
    next unless item.key?(:msgstr)
    item[:msgstr] = item[:msgstr][0..num_plurals - 1] if item[:msgstr].size > num_plurals
  end

  return items
end

def read_po file_name
  items   = [ { comments: [] } ]
  msgtype = nil
  line_no = 0
  started = false

  add_line = lambda do |type, to_add|
    items.last[type] ||= []
    items.last[type]  += to_add if to_add.is_a?(Array)
    items.last[type]  << to_add if to_add.is_a?(String)
  end

  IO.readlines(file_name).each do |line|
    line_no += 1
    line     = line.force_encoding("UTF-8").chomp

    if !line.empty? && !started
      items.last[:line] = line_no
      started           = true
    end

    if line.empty?
      items << {} unless items.last.keys.empty?
      msgtype = nil
      started = false

    elsif items.size == 1
      add_line.call :comments, line

    elsif /^#:\s*(.+)/.match(line)
      add_line.call :sources, $1.split(/\s+/)

    elsif /^#,\s*(.+)/.match(line)
      add_line.call :flags, $1.split(/,\s*/)

    elsif /^#\./.match(line)
      add_line.call :instructions, line

    elsif /^#~/.match(line)
      add_line.call :obsolete, line

    elsif /^#\|/.match(line)
      add_line.call :suggestions, line

    elsif /^#\s/.match(line)
      add_line.call :comments, line

    elsif /^ ( msgid(?:_plural)? | msgstr (?: \[ \d+ \])? ) \s* (.+)/x.match(line)
      type, string = $1, $2
      msgtype      = type.gsub(/\[.*/, '').to_sym

      items.last[msgtype] ||= []
      items.last[msgtype]  << unformat_string_for_po(string)

    elsif /^"/.match(line)
      fail "read_po: #{file_name}:#{line_no}: string entry without prior msgid/msgstr for »#{line}«" unless msgtype
      items.last[msgtype].last << unformat_string_for_po(line)

    else
      fail "read_po: #{file_name}:#{line_no}: unrecognized line type for »#{line}«"

    end
  end

  items.pop if items.last.keys.empty?

  return fix_po_msgstr_plurals items
end

def write_po file_name, items
  File.open(file_name, "w") do |file|

    items.each do |item|
      if item[:obsolete]
        file.puts(item[:obsolete].join("\n"))
        file.puts
        next
      end

      if item[:comments] && !item[:comments].empty?
        file.puts(item[:comments].join("\n"))
      end

      if item[:instructions] && !item[:instructions].empty?
        file.puts(item[:instructions].join("\n"))
      end

      if item[:sources] && !item[:sources].empty?
        file.puts(item[:sources].map { |source| "#: #{source}" }.join("\n").gsub(/,$/, ''))
      end

      if item[:flags] && !item[:flags].empty?
        file.puts("#, " + item[:flags].join(", "))
      end

      if item[:suggestions] && !item[:suggestions].empty?
        file.puts(item[:suggestions].join("\n"))
      end

      if item[:msgid]
        file.puts("msgid " + format_string_for_po(item[:msgid].first))
      end

      if item[:msgid_plural]
        file.puts("msgid_plural " + format_string_for_po(item[:msgid_plural].first))
      end

      if item[:msgstr]
        idx = 0

        item[:msgstr].each do |msgstr|
          suffix  = item[:msgid_plural] ? "[#{idx}]" : ""
          idx    += 1
          file.puts("msgstr#{suffix} " + format_string_for_po(msgstr))
        end
      end

      file.puts
    end
  end
end

def normalize_po file
  runq_code "NORMALIZE-PO", :target => file do
    write_po file, read_po(file)
  end
end

def replace_po_meta_info orig_metas, transifex_meta, key
  new_value = /"#{key}: \s+ (.+?) \\n"/x.match(transifex_meta)[1]
  # puts "looking for #{key} in #{transifex_meta}"
  # puts "  new val #{new_value}"
  return unless new_value

  orig_metas.each { |meta| meta.gsub!(/"#{key}: \s+ .+? \\n"/x, "\"#{key}: #{new_value}\\n\"") }
end

def merge_po orig_items, updated_items, options = {}
  translated = Hash[ *updated_items.
    select { |item| item[:msgid] && item[:msgid].first && !item[:msgid].first.empty? && item[:msgstr] && !item[:msgstr].empty? && !item[:msgstr].first.empty? }.
    map    { |item| [ item[:msgid].first, item ] }.
    flatten(1)
  ]

  update_meta_info = false

  orig_items.each do |orig_item|
    next if !orig_item[:msgid] || orig_item[:msgid].empty? || orig_item[:msgid].first.empty?

    updated_item = translated[ orig_item[:msgid].first ]

    next if !updated_item

    next if (orig_item[:msgstr] == updated_item[:msgstr]) && !(orig_item[:flags] || []).include?("fuzzy")

    # puts "UPDATE of msgid " + orig_item[:msgid].first
    # puts "  old " + orig_item[:msgstr].first
    # puts "  new " + updated_item[:msgstr].first

    update_meta_info   = true
    orig_item[:msgstr] = updated_item[:msgstr]

    next if (updated_item[:flags] || []).include?("fuzzy")

    orig_item[:flags].reject! { |flag| flag == "fuzzy" } if orig_item[:flags]
    orig_item.delete(:suggestions)
  end

  # update_meta_info = true

  return orig_items unless update_meta_info

  orig_meta          = orig_items.first[:comments]
  updated_meta       = updated_items.first[:comments].join("")
  headers_to_update  = %w{PO-Revision-Date Last-Translator Language-Team Plural-Forms}
  headers_to_update += options[:headers_to_update] || []

  headers_to_update.each { |key| replace_po_meta_info orig_meta, updated_meta, key }

  orig_items
end

def transifex_pull_and_merge resource, language
  po_file = resource == "programs" ? "po/#{language}.po" : "doc/man/po4a/po/#{language}.po"

  runq_git po_file, "checkout HEAD -- #{po_file}"

  orig_items = read_po(po_file)

  runq "tx pull", po_file, "tx pull -f -r mkvtoolnix.#{resource} -l #{language} > /dev/null"

  runq_code "merge", :target => po_file do
    transifex_items = read_po(po_file)
    merged_items    = merge_po orig_items, transifex_items
    fixed_items     = fix_po_msgstr_plurals merged_items

    write_po po_file, fixed_items
  end
end

def transifex_remove_fuzzy_and_push resource, language
  po_file          = resource == "programs" ? "po/#{language}.po" : "doc/man/po4a/po/#{language}.po"
  po_file_no_fuzzy = Tempfile.new("mkvtoolnix-rake-po-no-fuzzy")

  runq_git po_file, "checkout HEAD -- #{po_file}"

  runq "msgattrib", po_file, "msgattrib --no-fuzzy --output=#{po_file_no_fuzzy.path} #{po_file}"

  IO.write(po_file, IO.read(po_file_no_fuzzy))

  normalize_po po_file

  runq "tx push",  po_file, "tx push -t -f -r mkvtoolnix.#{resource} -l #{language} > /dev/null"

  runq_git po_file, "checkout HEAD -- #{po_file}"
end

def create_new_po dir
  %w{LANGUAGE EMAIL}.each { |e| fail "Variable '#{e}' is not set" if ENV[e].blank? }

  language = ENV['LANGUAGE']
  locale   = look_up_iso_639_1 language

  puts_action "create", :target => "#{dir}/#{locale}.po"
  File.open "#{dir}/#{locale}.po", "w" do |out|
    now      = Time.now
    email    = ENV['EMAIL']
    email    = "YOUR NAME <#{email}>" unless /</.match(email)
    header   = <<EOT
# translation of mkvtoolnix.pot to #{language}
# Copyright (C) #{now.year} Moritz Bunkus
# This file is distributed under the same license as the MKVToolNix package.
#
msgid ""
EOT

    content = IO.
      readlines("#{dir}/mkvtoolnix.pot").
      join("").
      gsub(/\A.*?msgid ""\n/m, header).
      gsub(/^"PO-Revision-Date:.*?$/m, %{"PO-Revision-Date: #{now.strftime('%Y-%m-%d %H:%M%z')}\\n"}).
      gsub(/^"Last-Translator:.*?$/m,  %{"Last-Translator: #{email}\\n"}).
      gsub(/^"Language-Team:.*?$/m,    %{"Language-Team: #{language} <mo@bunkus.online>\\n"}).
      gsub(/^"Language: \\n"$/,        %{"Language: #{locale}\\n"})

    out.puts content
  end

  puts "Remember to look up the plural forms in this document:\nhttp://docs.translatehouse.org/projects/localization-guide/en/latest/l10n/pluralforms.html"
end
