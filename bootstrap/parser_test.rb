# encoding: UTF-8
require_relative "parser"
require "test/unit"

class ParserTest < Test::Unit::TestCase
  def setup
    @sb = Parser.new
  end

  def test_bin_op
    sexp, lang = @sb.parse_line '1+12'
    r = bin_op [:value, 1], '+', [:value, 12]
    assert_equal r, sexp
    sexp = @sb.parse! '1+12'
    assert_equal [r], sexp

    sexp = @sb.parse! '1'
    assert_equal [[:value, 1]], sexp

    sexp = @sb.parse! '1 + 3*2'
    mul = bin_op [:value, 3], '*', [:value, 2]
    add = bin_op [:value, 1], '+', mul
    assert_equal [add], sexp
  end

  def test_line
    sexp, lang = @sb.parse_line 'a = b f + 3'
    assert_equal nil, lang
    _, value, vars = sexp
    assert_equal ['a'], vars
    assert_equal bin_op([:apply, [:word, "b"], [:word, "f"]], '+', [:value, 3]), value

    sexp, lang = @sb.parse_line 'b (\ f as) + (b = 33)'
    add, plus, part1, part2 = sexp
    assert_equal [:apply, [:word, "b"], [:lang, "\\", 'f', 'as']], part1
    assert_equal [:assign, [:value, 33], ["b"]], part2
    assert_equal [:lang, '\\', 'f', 'as'], lang
  end

  def test_src
    l1, l2, l3 = @sb.parse! '
a = 3
\ f a
  a + 1
f a
'
    assert_equal [:assign, [:value, 3], ['a']], l1
    block_line = bin_op [:word, "a"], '+', [:value, 1]
    assert_equal [:def, ['f', 'a'], [block_line]], l2
    assert_equal [:apply, [:word, "f"], [:word, "a"]], l3
  end

  def bin_op a, op, b
    [:apply, [:word, op], a, b]
  end
end
