require "digest"
require "erb"
require "fileutils"
require "json"
require "pathname"
require "tempfile"
require "tmpdir"
require "zlib"

$pty_module_available = true
begin
  require "pty"
rescue LoadError
  $pty_module_available = false
end
