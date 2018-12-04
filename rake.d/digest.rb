require "digest"

$digest_mutex = Mutex.new

def sha1_hexdigest data
  $digest_mutex.synchronize { Digest::SHA1.hexdigest(data) }
end
