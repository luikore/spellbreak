## inside def and include

`extend` is mere kernel method

class is translated to
    
    push_class 'X'

def is translated to

    define_method 'X', 'x', ...

## calling conventions

for targets:

ms         x86_64
system V   x86_64
javascript

building a C function

params:

    RDI, RSI, RDX, RCX, R8, R9, then stack
    self
    
    the number of args is pushed into XMM0 ? or determined by RBP ?
    
    return: RAX

in generated function code, first validate if param_size matches, then mov default params into registers after param_size

## bootstraping

the circular: rule is an embeding macro of sb, and sb is defined by rule.

so which one is more fundamental?

**the rule runtime** and the **sb runtime**

when runtimes are defined, we can put them together:

- syntax for sb, and ffi

then syntax for rules (it's quoting sb) and interpretation with sb

## vm

only assemble jump / call , and easy-to-inline instructions

instruction set:

- push_class A  ; add scope
- pop_class
- set_const
- get_const
- set_var
- get_var
- set_ivar
- get_ivar
- def

for def, new iseq and use def to add method to class

for lambda, new iseq and use set_ivar or whatever

thoughts: can we just validate syntax, but not-assemble the method before it is called?

macro related instruction set:

- parse
- rule
