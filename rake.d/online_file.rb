module Mtx::OnlineFile
  @@to_unlink = []
  @@mutex     = Mutex.new

  def self.download url, file_name = nil
    @@mutex.synchronize do
      FileUtils.mkdir_p "tmp"

      file_name ||= url.gsub(%r{.*/}, '')
      file_name   = "tmp/#{file_name}"

      if !FileTest.exist?(file_name)
        @@to_unlink << file_name

        runq "wget", url, "wget --quiet -O #{file_name} #{url}"
      end

      if %r{\.zip$}.match(file_name)
        require "zip"
        Zip::File.open(file_name, :create => false) do |zip_file|
          return zip_file.entries.reject { |entry| %r{/$}.match entry.name }.first.get_input_stream.read
        end
      end

      return IO.read(file_name)
    end
  end

  def self.cleanup
    return if c?(:KEEP_DOWNLOADED_FILES)

    @@to_unlink.
      select { |fn| FileTest.exist? fn }.
      each   { |fn| File.unlink      fn }
  end
end

END {
  Mtx::OnlineFile.cleanup
}
