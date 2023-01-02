module Mtx
  module CompilationDatabase
    @file_name            = "#{$build_dir}/compile_commands.json"
    @compilation_commands = {}

    def self.database_file_name
      @file_name
    end

    def self.read
      return {} unless FileTest.exist?(@file_name)

      Hash[
        *JSON.parse(IO.readlines(@file_name).join("")).
          map { |entry| [ entry["file"], entry ] }.
          flatten
      ]
    end

    def self.write
      return if @compilation_commands.empty?
      return if !FileTest.exist?(@file_name) && !c?(:BUILD_COMPILATION_DATABASE)

      entries = self.read.merge(@compilation_commands).values.sort_by { |e| e["file"] }
      File.open(@file_name, "w") do |f|
        f.write(JSON.generate(entries, :indent => "  ", :object_nl => "\n", :array_nl => "\n"))
      end
    end

    def self.add entry
      @compilation_commands[entry["file"]] = entry
    end
  end
end

at_exit { Mtx::CompilationDatabase.write }
