$message_mutex = Mutex.new
def show_message message
  $message_mutex.lock
  puts message
  $message_mutex.unlock
end

def error_and_exit text, exit_code = 2
  show_message text
  exit exit_code
end

def constantize camel_cased_word
  names = camel_cased_word.split('::')
  names.shift if names.empty? || names.first.empty?

  constant = Object
  names.each do |name|
    constant = constant.const_defined?(name) ? constant.const_get(name) : constant.const_missing(name)
  end
  constant
end

def file_name_to_class_name file_name
  "T_" + file_name.gsub(/^test-/, "").gsub(/\.rb$/, "")
end

def class_name_to_file_name class_name
  class_name.gsub(/^T_/, "test-") + ".rb"
end

class Array
  def extract_options!
    last.is_a?(::Hash) ? pop : {}
  end
end

class String
  def md5
    Digest::MD5.hexdigest self
  end
end

def md5 name
  case
  when $is_macos   then `/sbin/md5 #{name}`.chomp.gsub(/.*=\s*/, "")
  else             `md5sum #{name}`.chomp.gsub(/\s+.*/, "")
  end
end

def run_bash command
  return system command if !$is_windows

  cmd_file = Tempfile.new
  cmd_file.write command
  cmd_file.close

  FileUtils.chmod 0700, cmd_file.path

  system "bash -c #{cmd_file.path}"
end

def capture_bash command
  return `#{command}` if !$is_windows

  cmd_file = Tempfile.new
  cmd_file.write command
  cmd_file.close

  FileUtils.chmod 0700, cmd_file.path

  `bash -c #{cmd_file.path}`
end
