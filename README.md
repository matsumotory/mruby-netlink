# Netlink class for mruby
mruby netlink class

## install by mrbgems
 - add conf.gem line to `build_config.rb`
```ruby
MRuby::Build.new do |conf|

    # ... (snip) ...

    conf.gem :git => 'https://github.com/matsumoto-r/mruby-netlink.git'
end
```

## example

```ruby
n = Netlink.new "eth1"

n.down
sleep 5
n.up

n.close
```

# License
under the MIT License:

* http://www.opensource.org/licenses/mit-license.php


