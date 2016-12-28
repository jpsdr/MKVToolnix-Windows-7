def parse_changelog file_name
  changelog    = []
  current_line = []
  info         = nil
  version      = 'HEAD'

  flush        = lambda do
    if !current_line.empty?
      version = $1.gsub(/\.+$/, '') if / ^ released? \s+ v? ( [\d\.]+ )/ix.match(current_line[0])

      changelog << info.dup.merge({ :content => current_line.join(' '), :version => version })
      current_line = []
    end
  end

  IO.readlines(file_name).each do |line|
    line = line.chomp.gsub(/^\s+/, '').gsub(/\s+/, ' ')

    if / ^ (\d+)-(\d+)-(\d+) \s+ (.+) /x.match(line)
      y, m, d, author = $1, $2, $3, $4

      flush.call

      info = { :date => sprintf("%04d-%02d-%02d", y.to_i, m.to_i, d.to_i) }
      author.gsub!(/\s+/, ' ')
      if /(.+?) \s+ < (.+) >$/x.match(author)
        info[:author_name]  = $1
        info[:author_email] = $2
      else
        info[:author_name]  = author
      end

    else
      flush.call                              if     /^\*/.match(line)
      current_line << line.gsub(/^\*\s*/, '') unless line.empty?

    end
  end

  flush.call

  changelog.chunk { |e| e[:version] }
end
