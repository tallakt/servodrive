#!usr/bin/env ruby

$:.unshift File.dirname(__FILE__)

require 'servoctl'


sc = ServoControl.new


puts 'running servos, press ctrl+c to exit...'
loop do
  [-30.0, 0.0, 30.0, 10.0].each |speed|
    sc.speed 0 => speed, 1 => speed
    sleep 1000
    end
end

