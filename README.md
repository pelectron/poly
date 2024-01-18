# poly: external polymorphism (WIP)

This header-only C++17 library provides polymorphism without inheritance. It is inspired
by cpp-from-the-sky-downs
[metaprogrammed polymorphism](https://github.com/google/cpp-from-the-sky-down/tree/master/metaprogrammed_polymorphism)
(see [John Bandela “Polymorphism != Virtual: Easy, Flexible Runtime Polymorphism
Without Inheritance”](https://www.youtube.com/watch?v=PSxo85L2lC0) for the
corresponding talk) and [dyno](https://github.com/ldionne/dyno/tree/master) (see
[CppCon 2017: Louis Dionne “Runtime Polymorphism: Back to the
Basics”](https://www.youtube.com/watch?v=gVGtNFg4ay0&t=1063s) and [C++Now 2018:
Louis Dionne “Runtime Polymorphism: Back to the
Basics”](https://www.youtube.com/watch?v=OtU51Ytfe04)).

External polymorphism  is a non invasive technique to achieve polymorphic
behaviour for an open set of types, bypassing the inheritance system offered by C++. 

The main reasons one might want such a feature are flexibility, reducing
coupling, and increasing testability. The reference semantics introduced by
traditional C++ OOP are a pain to deal when it comes to lifetimes. The double
indirection for every virtual function call caused by the pointer-to-base can
also reduce performance, compared to the approach taken here. Additionally,
types which one cannot modify or add member functions to, for example types
from third parties, a C library, or built in types, can be extended.

## Features

- external polymorphism with simple implementation
- value semantics 
- configurable storage mechanism
    - local
    - small buffer optimized
    - heap allocated (TBD), 
    - copyable or move only
- name injection (requires usage of macros)
    - methods and properties can be injected with an actual name, i.e. legal
      C++ identifier, instead of needing to be referenced by template
      parameters.
- as constexpr as possible (fully constexpr when compiling with C++20)
- only standard library as dependency 
    - <type_traits>
    - <utility> (std::declvcal)
    - <new> (placement new, operator new for SBO storage)
    - <cstddef>

## How to include

## Tests

