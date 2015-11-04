#!/usr/bin/env ruby

def read_config
  IO.readlines("#{File.dirname(__FILE__)}/conf.sh").
    map    { |line| line.chomp.gsub(%r{\$HOME|\$\{HOME\}}, ENV['HOME']) }.
    reject { |line| %r{^#|^\s*$}.match line }.
    select { |line| %r{=}.match line }.
    map    { |line| line.split %r{=} }.
    to_h
end
