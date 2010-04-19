#!/usr/bin/env ruby
require 'ostruct'
m = []
File.open 'test' do |f|
  f.each_line do |l|
    tmp = l.split(/\s+/)
    m << OpenStruct.new(:n => tmp[0], :tag => tmp[1])
  end
end


ll = []
File.open 'beagle.h' do |f|
  f.each_line {|l| ll << l }
end


m.each do |mm|
  x = ll.find {|l| l.match mm.tag}
  puts '%s %s: %s' % [mm.n, mm.tag, x]
end
