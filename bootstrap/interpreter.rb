# encoding: UTF-8

class Interpreter
  attr_accessor :trace

  class Runtime
    def initialize parent=nil
      @parent = parent
      @this = {}
    end

    def spawn
      Runtime.new self
    end

    def [] k
      @this[k] or (@parent[k] if @parent)
    end

    # TODO handle nil
    def []= k, v
      if @parent
        if @parent[k].nil?
          @this[k] = v
        else
          @parent[k] = v
        end
      else
        @this[k] = v
      end
    end

    def merge ks, vs
      ks.zip vs do |k, v|
        @this[k] = v
      end
      self
    end
  end

  # interpreter main loop
  # -- we don't bother to generate fast code because this is only a bootstrap script
  def eval sexp, rt
    @stack << sexp
    eval_res =
    case sexp.first
    when :value
      sexp
    when :word
      _, word = sexp
      [:value, rt[word]]
    when :apply
      _, f, *params = sexp
      _, f = eval f, rt
      params.map! {|p| eval(p, rt)[1] }
      f[*params]
    when :assign
      _, value, vars = sexp
      _, value = eval value, rt
      vars.each do |v|
        rt[v] = value
      end
      [:value, value]
    when :def
      _, params, block = sexp
      lambda = proc do |*xs|
        rt = rt.spawn
        rt.merge(params, xs)
        eval block, rt
      end
      [:value, lambda]
    when Array # multi sexps
      r = nil
      sexp.each { |s| r = eval s, rt }
      r
    else
      raise "unknown sexp: #{sexp.inspect}"
    end
    if trace
      @stack.each { |tr| p tr }
      print '-> '
      p eval_res
      puts ''
    end
    @stack.pop
    eval_res
  end

  def eval! src
    rt = Runtime.new
    # TODO expose init binding to FFI
    rt['not'] = ->x{ [:value, (!x)] }
    rt['puts'] = ->x{ [:value, (puts x)] }
    %w[+ - * / % == != <= >= < > && ||].each do |op|
      rt[op] = Kernel.eval %Q| ->a, b{ [:value, (a #{op} b)] } |
    end

    @stack = []
    sexp = Parser.new.parse! src
    # p (eval sexp, rt)
    _, value = eval sexp, rt
    raise 'stack not empty after evaluation!' unless @stack.empty?
    value
  rescue
    @stack.each { |line| p line }
    raise
  end
end
