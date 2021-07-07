#!/usr/bin/ruby -w

# T_267mkvextract_clusters_with_unknown_size
describe "mkvextract / Clusters with an unknown size"

test "extraction" do
  extract "data/webm/live-stream.webm", 1 => tmp, :exit_code => :warning
  hash_tmp
end
