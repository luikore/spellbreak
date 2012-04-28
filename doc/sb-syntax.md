## literals

    0
    -3
    'ab'
    "ab"
    1.2
    4.5

## naming

`-` can be put inside words

## comments

    # inline comment

    #
      multiline
      comment

it is slim object at the same time

## class

`@` is "self"

    class A
      extend B
    
      def init a b c
        z = 3

        # previously defined same name functions
        super z
        
        # the following 2 are equivalent
        @b = z
        @['b'] = z

      def b, @b

      def b= x, @b = x

      def c e f
        e + f
        @

## new

    newed = A.new a b c

## send message

    a.x y z

another way to send message

    v = 'x'
    a.[v].call y z

## ivar

    v = 'x'
    a[v]

for less confusion, sugars like `a@v` is not added

## object representation

    VALUE klass
    map ivs

objects are in fact maps with special bindings, the binding depends on klass

## lambda

    l = \e f, a b

get method as lambda

    l = a.['x']

lambda as method params

    array.each \x
      print x

apply lambda

    l.call a b

quick define lambdas

    \( print _ )
    \( _.hello 3 )
    \( _ + _ )

## mixin

    class A
      extend B # methods are overridden

## hierachy

    class A::B::C, ... ;

top level

    ::func
    ::@ ## top level object

other separators doesn't work well

## meta class

singleton method

    def A.meth a b: ...

set ivar on class

    A['abc'] = 3

in implementation, the klass field of A will be pointed to the meta class

## env vars

    $abc

it's case sensitive, including `$1, ...` for params

## array and hash literals

    [a, b, c]
    {"a":12, "b":3}

true form

    [
      a
      b
      c
    ]
    {
      "a": 12
      "b": 3
    }

## one-liners , and ;

- `,` is same dent or indent, depending on context
- `;` is dedent

    if a1, if b1, b2, b3; else a2

note if `:` is can make hash literal difficult, so not considering it.

## while

    while a < 3
      ...
    
## macros

single

    %a
      ...
      ...

macro dominates till the end of line, if must have multiple in one line, should make the first inline

    %r(...) %json
      ["a", 123]

whether a macro can be posed inline depends on the compositivity of the target language

all macro return objects

## default macros

- `%r` regexp
- `%s` string
- `%q` quoted string, can interpolate with #{}
- `%w` words array
- `%ld` return an ffi object linking to a dylib

    ffi = %ld
      #pragma comment "stdc"
      // the following captures stdc
      #pragma comment stdc
      #include <stdio.h>

- `%sh`

    %sh
      #!/usr/bin/ruby
      ...

how macro captures depends on the definition: the definition can use spellbreak's built-in syntax or interpretations

## defining rules

default rule

    %%
      repeat : a*  # first one is start rule
      a      : 'a'

variant semantics are of no difference, just use the existing rules and vary them

    %%
      repeat2 : a*
      a       : repeat.a

## would-be-useful macros

- %csv
- %json
- %yaml
