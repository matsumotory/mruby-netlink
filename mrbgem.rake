MRuby::Gem::Specification.new('mruby-netlink') do |spec|
  spec.license = 'MIT'
  spec.authors = 'MATSUMOTO Ryosuke'
  spec.linker.libraries << 'netlink'
end
