def no_conflict(lines)
  addr = lines.map { |line|
    line =~ /(?<address>\h+)$/
    $~[:address] if $~
  }.select{|x| ! x.nil?}
  if addr.uniq.length == addr.length
    exit(0)
  else
    exit(-1)
  end
end

if ARGV[0] then
  no_conflict File.readlines(ARGV[0])
else
  no_conflict STDIN.readlines
end
