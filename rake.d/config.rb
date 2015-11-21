def read_config
  fail "build-config not found: please run ./configure" unless File.exists?("build-config")

  $config = Hash[ *IO.readlines("build-config").collect { |line| line.chomp.gsub(/#.*/, "") }.select { |line| !line.empty? }.collect do |line|
                     parts = line.split(/\s*=\s*/, 2).collect { |part| part.gsub(/^\s+/, '').gsub(/\s+$/, '') }
                     [ parts[0].to_sym, parts[1] || '' ]
                   end.flatten ]
  $config.default = ''
end

def c(idx, default = '')
  idx_s = idx.to_s
  var   = (ENV[idx_s].nil? ? $config[idx.to_sym] : ENV[idx_s]).to_s
  var   = var.gsub(/\$[\({](.*?)[\)}]/) { c($1) }.gsub(/^\s+/, '').gsub(/\s+$/, '')

  var.empty? ? default : var
end

def c?(idx)
  c(idx).to_bool
end
