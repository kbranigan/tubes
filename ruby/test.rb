#!/usr/bin/env ruby -w
#encoding:ASCII-8BIT

require File.join(File.dirname(File.expand_path(__FILE__)), 'Pipes') # tanks carsten
require 'rubygems'
require 'json'

Pipes.assert_stdin_is_piped
Pipes.assert_stdout_is_piped

Pipes.read_each(STDIN) do |shape|
  Pipes.write(STDOUT, shape)
end

STDERR.puts "awwwyeah"

=begin

Be aware, if you puts anything at all to STDOUT it'll mess up this script.
You need to puts to STDERR.

=end