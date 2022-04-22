class NilClass
  def to_bool
    false
  end

  def blank?
    true
  end
end

class String
  def to_bool
    %w{1 true yes}.include? self.downcase
  end

  def blank?
    empty?
  end

  def to_c_string
  '"' + self.gsub(%r{"}, '\"') + '"'
  end

  def to_u8_c_string
  'u8' + self.to_c_string
  end

  def to_cpp_string
    self.to_c_string + "s"
  end

  def to_u8_cpp_string
    "u8" + self.to_c_string + "s"
  end
end

class TrueClass
  def to_bool
    self
  end
end

class FalseClass
  def to_bool
    self
  end
end

class Array
  def to_hash_by key = nil, value = nil, &block
    elements = case
               when block_given? then self.collect(&block)
               when key.nil? then     self.collect { |e| [ e, true ] }
               else                   self.collect { |e| e.is_a?(Hash) ? [ e[key], value.nil? ? e : e[value] ] : [ e.send(key), value.nil? ? e : e.send(value) ] }
               end

    return Hash[ *elements.flatten ]
  end

  def for_target!
    self.flatten!

    reject = {
      :linux   => %w{      macos      windows},
      :macos   => %w{linux            windows x11},
      :windows => %w{linux macos unix         x11},
    }

    # Treat other OS (e.g. FreeBSD) the same as Linux wrt. which files to compile
    os    = $building_for.keys.select { |key| $building_for[key] }.first
    types = reject[os || :linux]

    re    = '(?:' + types.join('|') + ')'
    re    = %r{(?:/|^)#{re}[_.]}

    self.reject! { |f| re.match f }

    return self
  end
end
