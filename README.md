poly: external polymorphism (WIP)

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
- easy value semantics with configurable storage mechanisms
  - copyable or move only
  - local, small buffer optimized, strictly heap allocated (TBD) options
  - nothrow or throwing move (TBD)
- optional name "injection" (requires minimal usage of macros)
  - methods and properties can be injected with an actual name, i.e. legal
    C++ identifier, instead of needing to be referenced by template
    parameters or writing a wrapper
- as constexpr as possible
- only basic components of standard library as dependency
- low overhead: implementing the same feature set by hand should lead to identical
  performance and memory usage as using poly

## How to include

Get the source code, add the include folder to your toolchains include path, and add

```
#include "poly.hpp"
```

Alternatively, if building with [meson](https://mesonbuild.com/), this project
can be added as a subproject.

## A small sample

Imagine an graphics application or library. One feature of such software could
be to draw geometric shapes on a canvas. A simple approach would be to
create a base class with all operations defined as virtual functions, and make
every concrete shape inherit from the shape base. The application will also
need to store multiple shapes in a container, iterate over them, and draw them.
A pointer to the base shape must be used, which is probably further wrapped in
a smart pointer for RAII.

```{cpp}
// The shape base
class Shape{
public:
    // ctors etc.
    Shape(...);

    // dont forget
    virtual ~Shape()=0;

    // the operatio
    virtual void draw(Canvas&, Vec2d pos) const=0;

    // needed to copy Shapes
    virtual Shape* clone()=0;
};

class Circle : publice Shape {
    // ctors etc.
    Circle(...): Shape(...){}

    ~Circle()override;

    // the operation
    void draw(Canvas&, Vec2d pos) const overrride;

    // needed to copy Shapes
    Shape* clone() override;

private:
    double radius_;
};

class Rectangle : public Shape {
    // ctors etc.
    Rectangle(...): Shape(...){}

    ~Rectangle()override;

    // the operation
    void draw(Canvas&, Vec2d pos) const overrride;

    // needed to copy Shapes
    Shape* clone() override;

private:
    double width_,height_;
};

#include <vector>
#include <memory>

using BasicShapeContainer = std::vector<Shape*>;
using SharedShapeContainer = std::vector<std::shared_ptr<Shape>>;
using UniqueShapeContainer = std::vector<std::unique_ptr<Shape>>;

// works for any of the three container aliases defined above
template<typename ShapeContainer>
void draw_shapes_at_random_position(const ShapeContainer& shapes, Canvas& canvas){
    for(const auto& shape : shapes){
        shape->draw(canvas, get_random_pos());
    }
}

// creating a vector of shapes is also pretty ugly, either 
// with naked new or littered with make_shared/make_unique
BasicShapeContainer get_shapes(){
    return {new Rectanlge(...), new Circle(...), ...};
}
```

### What are the concrete problems with this approach?

- every type to draw MUST inherit from Shape. Third party types cannot be added
  directly. A wrapper would have to be created.
- value semantics are not given, at least with the BasicShapeContainer.
- every instance of a shape must be heap allocated, which leads fragmentation and hinders cache locality.
- for every draw call, at least two indirections are made
  - 1. following pointer to base
  - 2. following vptr to vtable

### What is the apparent solution

Manual type erasure. Type erasure removes, i.e. erases, the run time type from a
variable. However, the naive approach to type erasure only solves the first two problems of
the ones mentioned above. Heap allocation still occurs, inheritance with
virtual function is used. Furthermore, every type erased interface must be
defined at least three times (the type erased wrapper to use, the internal
model type, and the actual implementation derived from the model), all of which
is manual and error prone work.

More on type erasure basics can be found here:

- [Breaking Dependencies: Type Erasure - A Design Analysis - Klaus Iglberger -
  CppCon 2021](https://www.youtube.com/watch?v=qn6OqefuH08&t=660s)
- [Back to Basics: Type Erasure - Arthur O'Dwyer - CppCon
  2019](https://www.youtube.com/watch?v=tbUCHifyT24&t=652s)
- [C++ Weekly - Ep 343 - Digging Into Type
  Erasure](https://www.youtube.com/watch?v=iMzEUdacznQ&t=299s)

### What is needed for real solution

Generic and descriptive type erasure. It offers the same benefits as the manual
version, and some additional ones depending on the implementation, while
reducing manual work substantially.

For one, no more repeating yourself three times for one interface as in the
manual case.

Secondly, the allocation/storage system can be made generic. This enables
further fine grained control by simply specifying a different storage
mechanisms. This brings value semantics, guarantees about dynamic
allocation (or lack thereof) and copyability control, which makes the system
useable even in constrained systems.

Thirdly, virtual functions can be completely removed, even from the
implementation. This requires creating a vtable outside the objects emplaced in
the type erased wrapper and outside of the storage location of these objects.
The latter point is the problem with the naive type erasure, because the vptr
still resides inside the model base class. The purpose of all this work is
moving the vptr onto the stack, which removes one indirection from a member
function call.

### Why poly

poly provides the necessary features described above. But the libraries
mentioned in the introduction also do this, so why use poly?

Simply put, poly is designed to be a middle ground between metaprogrammed
polymorphism and dyno in terms of complexity, configurability and type safety,
while aiming to be more user friendly than both.

poly has a more complex implementation than metaprogrammed polymorphism, but at
the same time offers more flexibility when it comes to specifying stack
consumption and storage, is type safer, and constexpr friendly.

dyno provides much more configuration than poly when it comes to the storage of
objects, vptrs, and individual vtable entries. However, this comes at the cost
of a very complex metaprogramming implementation, which further depends on
boost::hana and pretty much requires macro usage by the library user.

One feature missing from the mentioned libraries is the ability to inject
method names into the main type erasure template class provided by them, such
that working with fully specialized instances of the type erasure template is
as natural as working with manual type erasure without manually creating a
wrapper. poly just requires the usage of one macro, and does not cause storage
overhead for providing name injection (\* see [Considerations](#considerations)).

Let's take another look at the original sample, but with the requirement of
adding a small buffer optimization of 16 bytes to our type erased wrapper,
using poly.

```{cpp}
#include "poly.hpp"
// external, third party c library header
#include "fancy_shape.h" // provides struct FancyShape_t and
                         // void draw_fancy_shape(const struct FancyShape_t* s, Canvas* c, int x, int y)

// defining the method name
POLY_METHOD(draw)

// alternative method name definition without macro:
// struct draw{};
// note: 
// - does not offer any serious advantage to using the macro
// - name injection for methods does not add any real overhead, except for an
//   additional symbol for each MethodSpec in the executable.
// main drawbacks are
// - name injection cannot be used
// - the generic version of extend() must also be defined manually, if a geneic version is desired
//

// generic version of extend() 
// template<typename T>
// void extend(draw, const T& t, Canvas&c, Vec2d pos){
//     t.draw(c, pos);
// }
// note: 
// - should not be defined when using POLY_METHOD and using poly in the
//   standard configuration.
// - enable_if_t should be used  to check for the validity of the expression
//   t.draw(c,pos), but is left out for simplicity

// extending draw for struct FancyShape_t
void extend(draw, const struct FancyShape_t& s, Canvas& c, Vec2d pos){
    draw_fancy_shape(&s, &c, pos.x, pos.y);
}

struct Circle {
    void draw(Canvas&, Vec2d) const;
    double radius{0};
};

class Rectangle{
    void draw(Canvas&, Vec2d) const;
    double width_
    double height_;
};

// all methods of a type erased interface are collected in a type list.
// The elements of the list are called MethodSpecs, and have the form of:
// ReturnType(MethodName, Args...), const and noexcept can also be specified at
// the end. 
// 
// In this case, only one const method with the name draw, return type void,
// and arguments Canvas& and Vec2d is specified.
using methods = poly::type_list<void(draw, Canvas&, Vec2d) const>;
// for now, not using any properties -> empty list
using properties = poly::type_list<>;
// a small buffer optimized storage type with a 16 byte buffer, 
// poly::move_only_sbo_storage can be used to make Drawable move only
using storage = poly::sbo_storage<16>;

// Drawable can now be used to store any object with extends draw with the
// corresponding signature, and stores objects with a size of 16 bytes or less inline.
// basic_object is the main class this library exports.
using Drawable = poly::basic_object<storage, properties, methods>;
// or with additional helper macros in (possibly) one line
using Drawable = 
    poly::basic_object< poly::sbo_storage<16>, 
                        POLY_PROPERTIES(), /*same as properties above*/
                        POLY_METHODS(void(draw, Canvas&, Vec2d) const)/*same as methods above*/>;

// the shape container as before
using ShapeContainer = std::vector<Drawable>;

// can now be separately compiled
void draw_shapes_at_random_position(const ShapeContainer& shapes, Canvas& canvas){
    for(const auto& shape : shapes){
        // with name injection: requires POLY_METHOD usage
        shape.draw(canvas, get_random_pos());
        // without name injection: need to provide method name as template
        // parameter
        shape.call<draw>(canvas, get_random_pos());
    }
}

// simple, value semantics, no syntax clutter due to actual storage used,
// no extra dynamic allocations other than what std::vector allocates 
ShapeContainer get_shapes(){
    return {Rectangle(...), Circle(...), struct FancyShape_t{...}...};
}
```

With poly, Circle and Rectangle don't depend on Shape anymore. The external 3rd
party C struct can easily be adapted with one free function. Types already
satisfying the interface, i.e. featuring a void draw(Canvas&, Vec2d) member
function, can be stored in a Drawable instance without having to manually write
the extend() function, given that the methods name, i.e. draw, was defined with
the POLY_METHOD macro.

Adapting storage requirements to have a guarantee to never dynamically allocate
can be achieved by simply changing the storage type, i.e change the storage
definition to

```{cpp}
using storage = poly::local_storage<16>;
```

Factory functions which need to return a different type depending on runtime
parameters can also be easily implemented, without exposing any implementation
details in a header. A good example could be a communication driver which has a
standard high level interface but several underlying implementations depending
on the actual protocol used, i.e. Ethernet, USB, serial etc.

```{cpp}
// com_driver.hpp
#include "poly.hpp"
#include <optional>
#include <string>

POLY_METHOD(write)
POLY_METHOD(read)

// interface to write and read strings
using ComDriver = 
    poly::basic_object< poly::move_only_sbo_storage<32>, 
                        POLY_PROPERTIES(),
                        POLY_METHODS(void(write, const std::string& msg),
                                     std::string(read))>;

// parses the address, and returns a ComDriver if the address is valid, 
// else returns std::nullopt
std::optional<ComDriver> create_com_driver(const std::string& address);

// com_driver.cpp
#include "com_driver.hpp"
#include "com_usb_driver.hpp" // defines UsbDriver and UsbEndpoint
#include "com_eth_driver.hpp" // defines EthDriver and EthEndpoint
#include <utility>
#include <variant>

enum ComType{
    INVALID,
    USB,
    ETH
};

std::variant<std::monostate, UsbEndpoint, EthEndpoint> 
parse_address(const std::string& addr){
// ... (possibly gory) details of string parsing
}

std::optional<ComDriver> create_com_driver(const std::string& address){
    auto addr = parse_address(address);
    switch(addr.index()){
        case ComType::USB:
            return UsbDriver{std::get<UsbEndpoint>(addr)};
        case ComType::EthDriver:
            return EthDriver{std::get<EthEndpoint>(addr)};
    }
    return std::nullopt;// invalid address
}
```

Another feature of poly is the property system. Properties model public member
variables. Without name injection, they are accessed with get and set member
function of the poly::basic_object. With name injection, properties are accessed
in the same way as public member variables.

A simple example would be a non owning buffer type, which has a read only
property "size" and "data", and adapts any container.

```{cpp}
#include "poly.hpp"

POLY_PROPERTY(size) // alternative: struct size{};

// getter for size needs to be defined, as default implementation checks for
// t.size instead of t.size()
template<typename Container>
size_t get(size, const Container& c) noexcept{
    return c.size();
}

POLY_PROPERTY(data)

// getter for size needs to be defined, as default implementation checks for
// t.data of type void*, instead of t.data()
template<typename Container>
const void* get(data, const Container& c) noexcept{
    return static_cast<const void*>(c.data());
}

// non owing storage
using storage = poly::ref_storage;
// list of properties
using properties = POLY_PROPERTIES(const size(size_t), const data(const void*));
// no methods
using methods = POLY_METHODS();
using Buffer = poly::basic_object<storage, properties, methods>;

Buffer print_half(Buffer b){
    // with name injection
    const size_t to_consume = b.size / 2;
    for(size_t i = 0; i < to_consume; ++i){
        int c = *reinterpret_cast<const uint8_t*>(b.data);
        putchar(c);
    }

    // without name injection
    const size_t to_consume = b.get<size>() / 2;
    for(size_t i = 0; i < b.get<size>(); ++i){
        int c = *reinterpret_cast<const uint8_t*>(b.get<data>());
        putchar(c);
    }

    return b;
}
```

As in the previous example, print_half() can be defined in a separate
translation unit and does not need to be defined in a header file.

## Considerations

This library relies on empty base class optimizations (EBCO) to get the smallest
size possible for the poly::basic_object template. If EBCO is applied correctly,
the size of basic_object will always be the size of the storage used plus the
size of a pointer.

This works well on linux with GCC and clang, as well as GCC on windows. In this
combination of compiler and platform, name injection will never incur any
overhead.

On windows, clang and msvc do generate some overhead here (clang aims to be
compatible with msvc), but only when using name injection with properties.
Concretely, if exactly one property with name injection is specified in a
basic_object, the size of that basic_object specialization will be one pointer
size greater than an object with a property without name injection. This size
overhead will stay constant, i.e. stay at the size of one pointer, until the
number of properties with name injection is greater than the size of a pointer.
Then the overhead will increase by an additional pointer size.

If this is unclear, some concrete math could help: Let N be the size of one
pointer, i.e. sizeof(void\*). Let n be the number of properties with name
injection specified in a basic_object. The overhead O in bytes of that
basic_object can be calculated as

O = ceil(n/N) * N

On a 64 bit platform, this would mean an overhead of 8 bytes for the first 8
properties with name injection added to a basic_object, an overhead of 16 bytes
for n between 9 and 16, and so on.

Name injection with methods should never incur any space overhead.

macOS is not tested, as I do not have a mac.

## Testing

Unit testing can be done by building the project with meson. The resulting
executable "main" runs the unit tests.
