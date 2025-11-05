#!/usr/bin/env ruby

class FormatStringVerifier
  def verify file_name
    language  = file_name.gsub(%r{.*/|\.po$}, '')
    to_ignore = {
      'eu' => [ '74f8165349457fb0d89a559ffa83c4b3b3c1993b', 'aa8f04e33094d67c75256a298364323f38967abf' ],
      'tr' => [ '1934766bc7b88d6824e95d50e313ca3e129e62e7', '62fc63d5ae795e24fcdbdfd5ec300c7bf5e46b76' ],
    }

    entries = read_po(file_name).
      reject { |e| e[:obsolete] }.
      select { |e| e[:msgid] && e[:msgstr] && e[:flags] }.
      reject { |e| e[:msgid].empty? || e[:msgstr].empty? }.
      reject { |e| e[:msgstr].all?(&:empty?) }.
      reject { |e| e[:flags].include?("fuzzy") }

    errors = []

    matchers = {
      :fmt => /( \{ \d+ (?: : [^\}]+ )? \} )/ix,
      :qt  => /( (?<! %) % \d+ (?! [a-z]) )/ix,
    }

    format_types = matchers.keys

    entries.each do |e|
      formats       = Hash[ *matchers.keys.map { |key| [ key, { :id => [], :str => [] } ] }.flatten ]
      error_digests = []

      e[:msgstr].each_with_index do |msgstr, idx|
        ( (e[:msgid] || []) + (e[:msgid_plural] || []) ).
          reject(&:nil?).
          each do |msgid|
            format_types.each do |type|
              sha1 = sha1_hexdigest "#{type}:#{msgid}:#{msgstr}"

              next if to_ignore.key?(language) && to_ignore[language].include?(sha1)

              store = formats[type]

              store[:id]  << msgid .scan(matchers[type]).uniq.sort
              store[:str] << msgstr.scan(matchers[type]).uniq.sort

              missing = store[:id]  - store[:str]
              added   = store[:str] - store[:id]

              next if missing.empty? && added.empty?
              next if (missing == [[]]) && (added == [[["%100"]]])  # 100% → %100 in languages such as Turkish

              pp({ missing: missing, added: added })

              error_digests << sha1
            end
        end
      end

      errors << { :line => e[:line], :digests => error_digests } if !error_digests.empty?
    end

    # pp errors

    errors.each do |error|
      puts "#{file_name}:#{error[:line]}: error: format string differences (IDs: #{error[:digests].sort.join(' ')})"
    end

    return errors.empty?
  end
end
