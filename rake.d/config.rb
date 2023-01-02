def read_config_file file_name
  result = Hash[ *IO.readlines(file_name).collect { |line| line.chomp.gsub(/#.*/, "") }.select { |line| !line.empty? }.collect do |line|
                   parts = line.split(/\s*=\s*/, 2).collect { |part| part.gsub(/^\s+/, '').gsub(/\s+$/, '') }
                   key   = parts[0].gsub(%r{^export\s+}, '').to_sym
                   value = (parts[1] || '').gsub(%r{^"|"$}, '').gsub(%r{\\"}, '"')
                   [ key, value ]
                 end.flatten ]
  result.default = ''

  result
end

def read_build_config
  dir = File.dirname(__FILE__) + '/..'

  fail "build-config not found: please run ./configure" unless FileTest.exist?("#{dir}/build-config")

  config = read_config_file("#{dir}/build-config")
  config = config.merge(read_config_file("#{dir}/build-config.local")) if FileTest.exist?("#{dir}/build-config.local")

  config
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
