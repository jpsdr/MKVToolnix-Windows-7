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

    self.reject! { |f| %r{(?:/|^)linux[_.]}.match f }   unless $building_for[:linux]
    self.reject! { |f| %r{(?:/|^)macos[_.]}.match f }   unless $building_for[:macos]
    self.reject! { |f| %r{(?:/|^)unix[_.]}.match f }    if     $building_for[:windows]
    self.reject! { |f| %r{(?:/|^)windows[_.]}.match f } unless $building_for[:windows]

    return self
  end
end
