class Library < Target
  def initialize(name)
    super name
    @build_dll = false
  end

  def build_dll(build_dll_as_well = true)
    return self if !@only_if || (options.include?(:if) && !options[:if])
    @build_dll = build_dll_as_well
    self
  end

  def create_specific_static
    file "#{@target}.a" => @objects do |t|
      FileUtils.rm_f t.name
      runq "ar", t.name, "#{c(:AR)} crS #{t.name} #{@objects.join(" ")}"
      runq "ranlib", t.name, "#{c(:RANLIB)} #{$flags[:ranlib]} #{t.name}"
    end
  end

  def create_specific_dll
    file "#{@target}.dll" => @objects do |t|
      runq "link", t.name, <<-COMMAND
        #{c(:CXX)} #{$flags[:ldflags]} #{$system_libdirs} -shared -Wl,--export-all -Wl,--out-implib=#{t.name}.a -o #{t.name} #{@objects.join(" ")} #{@libraries.join(" ")}
      COMMAND
    end
  end

  def create_specific
    if @build_dll
      create_specific_dll
    else
      create_specific_static
    end

    self
  end
end
