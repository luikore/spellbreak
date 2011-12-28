# encoding: UTF-8
require_relative "parser"
require_relative "interpreter"
require "test/unit"
require "pry"

class InterpreterTest < Test::Unit::TestCase
  def setup
    @i = Interpreter.new
  end

  def test_basic_expr
    assert_eval 13, '13'
    assert_eval 13, '1+12'
  end

  def test_interpret
    pr = @i.eval! "\\ x\n  x + 3"
    assert_equal [:value, 13], pr[10]

    assert_eval 13, 'a = 13'

    assert_eval 13, "(\\ x) 3\n  x + 10"
  end

  def assert_eval r, src
    assert_equal r, (@i.eval! src)
  end
end
