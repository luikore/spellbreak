# encoding: UTF-8

# parse -> sexp (can't execute while parsing, because languages are delayed)

# a line is an expression.

# expresstion of terms combined by binary operators or spaces
#   a + b
#   a = b
# like Haskell, space has higher precedence than other binary operators
#   a b c + e f

# term is the basic unit, which can be:
# - variable or function call ( `(f)` is applying `f` )
#     a
#     (f)
# - literals
#     3
#     true
#     false
#     nil
#     -1.2  # not in bootstrap
#     'asd' # not in bootstrap
# - parens around an expression
#     (a + b)
# - domain specific languages
#     %array
#       1 2 3
#       4 5 6
#     %hash
#       a: 12
#       b: 3
#     %s
#       hello world
#     %r
#       \d\w
#     (%x '#! /usr/bin/ash')
#       rm -rf /
#     (\ a b c)
#       content

# NOTE about the apply syntax:
# if there's only 1 term inside `()` pair, it's interpreted as apply. for example:
#   f a       # apply f a
#   (f a)     # apply f a
#   ((f a))   # apply f a then apply the result
#   (a + b)   # not apply
#   ((a + b)) # a + b then apply the result

require "rsec"

# helpers
module Rsec::Parser
  SPACE = /[\ \t]*/.r

  # wrap result with [:node, ]
  def as node
    map { |s| [node, s] }
  end

  # left assoc op, turned into nested apply
  def bin_op op
    join(SPACE >> op << SPACE).map do |(head, *rest)|
      if rest.empty?
        head
      else
        rest.each_slice(2).inject head do |lhs, (op, rhs)|
          [:apply, [:word, op], lhs, rhs]
        end
      end
    end
  end

  # assignment is right-assoc and has an extra limit:
  #   all left terms should be words
  def assign_op op
    join(SPACE >> op << SPACE).even.map do |xs|
      if xs.size == 1
        xs.first
      else
        value = xs.pop
        vars = xs.reverse.map do |(ty, v)|
          ty == :word ? v : raise('expect word in assign')
        end
        [:assign, value, vars]
      end
    end
  end

  # (a) is apply
  def paren_apply
    map do |sexp|
      ty, head, *rest = sexp
      rest.empty? ? [:apply, sexp] : sexp
    end
  end

  # differ macro and apply, and map macro to lang
  # XXX: the block is for setting parser internal states
  def macro_apply
    map do |(head, *rest)|
      # TODO number / string / collection as function extension
      ty, lang = head
      if ty == :macro
        rest.map! { |r| r[0] == :word ? r[1] : raise("invalid def: #{[head, *rest].inspect}") }
        yield [:lang, lang, *rest] # spat rest: less token if rest empty
      else
        rest.empty? ? head : [:apply, head, *rest]
      end
    end
  end

end

class Parser
  include Rsec::Helper

  # word and literals
  WORD  = /[a-zA-Z_]\w*/
  TRUE  = /\btrue\b/
  FALSE = /\bfalse\b/
  NIL   = /\bnil\b/
  INT   = /\b\d+\b/

  # bin operators
  # TODO more inteligent ambiguous rule for % and %lang ?
  SP   = /[\ \t]+/
  MUL  = /[*\/]|%(?=[\ \t])/
  ADD  = /[+-]/
  COMP = /==|!=|<=|>=|<|>/
  AND  = '&&'
  OR   = '||'
  EQ   = '='

  def initialize
    @line_parser = make_line_parser
  end

  def make_line_parser
    word = WORD.r.as :word
    macro = ('%'.r >> WORD | '\\').as :macro

    int = INT.r &:to_i
    bool = TRUE.r{true} | FALSE.r{false}
    literal = (int | bool | NIL.r{nil}).as :value

    paren = ('('.r >> lazy{expr} << ')').paren_apply

    unit_term = word | literal | macro | paren
    term = unit_term.join(SP).even.macro_apply do |lang|
      if @lang
        raise "already defined lang: #{@lang.inspect}"
      else
        @lang = lang
      end
    end

    # NOTE 'not' is function.
    #       as in other languages, 'and' is higher than 'or'.
    expr = term
      .bin_op(MUL)
      .bin_op(ADD)
      .bin_op(COMP)
      .bin_op(AND)
      .bin_op(OR)
      .assign_op(EQ)
  end

  # return sexp, lang
  def parse_line l
    [@line_parser.parse!(l.rstrip), @lang].tap{ @lang = nil }
  end

  # TODO make it extensible
  def parse_lang lang, block
    case lang[1]
    when '\\'
      # [:lang, '\\', *params]
      _, _, *params = lang
      lang.clear
      lang << :def << params << parse(block)
    when 's'
      lang.clear
      s = block.join.chomp
      lang << :value << s
    when 'array'
      # TODO
    when 'hash'
      # TODO
    end
  end

  def parse lines
    r = []
    while !lines.empty?
      line, *lines = lines
      next if line =~ /^\s*$/
      sexp, lang = parse_line line
      r << sexp
      if lang and lang[1]
        raise "empty lang at: #{line}" unless lines.first
        block, lines = lines.partition{|line| line.sub!(/^  /, '') or line =~ /^\s*$/ }
        parse_lang lang, block
      end
    end
    r
  end

  def parse! src
    parse src.lines.to_a
  end

end
