#!/usr/bin/env ruby

class FormatStringVerifier
  def verify file_name
    entries = read_po(file_name).
      reject { |e| e[:obsolete] }.
      select { |e| e[:msgid] && e[:msgstr] && e[:flags] }.
      reject { |e| e[:msgid].empty? || e[:msgstr].empty? }.
      reject { |e| e[:msgstr].all?(&:empty?) }.
      reject { |e| e[:flags].include?("fuzzy") }.
      select { |e| e[:flags].include?("c-format") || e[:flags].include?("boost-format") }

    errors = []

    matchers = {
      :boost => /%(?:
                     %
                   | \|[0-9$a-zA-Z\.\-]+\|
                   | [0-9]+%?
                   )/ix,

      :c      => /%(?:
                      %
                    | -?\.?\s?[0-9]*l?l?[a-zA-Z]
                    )/ix,
    }

    format_types = matchers.keys

    entries.each do |e|
      formats = {
        :c     => { :id => [], :str => [] },
        :boost => { :id => [], :str => [] },
      }

      has_errors = false

      e[:msgstr].each_with_index do |msgstr, idx|
        ( (e[:msgid] || []) + (e[:msgid_plural] || []) ).
          reject(&:nil?).
          each do |msgid|
            format_types.select { |type| e[:flags].include?("#{type}-format") }.each do |type|
              store = formats[type]

              store[:id]  << msgid .scan(matchers[type]).uniq.sort
              store[:str] << msgstr.scan(matchers[type]).uniq.sort

              missing = store[:id]  - store[:str]
              added   = store[:str] - store[:id]

              has_errors = true if !missing.empty? || !added.empty?
            end
        end
      end

      errors << e[:line] if has_errors
    end

    # pp errors

    errors.each do |error|
      puts "#{file_name}:#{error}: error: format string differences"
    end

    return errors.empty?
  end
end
