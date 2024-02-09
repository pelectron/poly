poly enables and/or emulates polymorphic use of member variables and member
functions. It does so by providing the template `poly::Struct` and
`poly::Interface`. The polymorphic member variables poly provides are referred
to as **properties**. The polymorphic member functions are called **methods**.

## Names

Properties and methods have a name associated with them, just like
any ordinary member variable or function has a name.

In poly, the name of a method or property is an empty, fully defined struct/class, i.e.
`struct any_name{};`.

poly also provides name injection via use of the `POLY_METHOD` and
`POLY_PROPERTY` macros. Defining method and property names with these macros
will enable the usage of said method or property directly by name in a
`poly::Struct` featuring it.

```cpp
#include "poly.hpp"
// defines the method name my_method
POLY_METHOD(my_method)
// defines the propety name my_property
POLY_PROPERTY(my_property)

// obj refers to an instance of poly::Struct with a method called my_method and
// property called my_property.
// with name injection, the names can be used directly:
auto ret_val = obj.my_method(...);
auto prop_val = obj.my_property;
obj.my_property = ...;

// without name injection:
auto ret_val = obj.call<my_method>(...);
auto prop_val = obj.get<my_property>();
obj.set<my_property>(...);
```

## Specs

In addition to a name, native c++ member functions and variables have additional
attributes. For functions, this would be the return type, the argument types,
const/mutable access specifier, and noexcept specification. For member
variables, the modeled attributes are of course the type, and const/mutable
access. Without these attributes, a function or variable definition is not
complete and illegal syntax.

The same goes for poly, i.e. more than a name is needed to generate a method or
property. The thing used by poly to specify a method or property fully is called
a MethodSpec or PropertySpec respectively.

### MethodSpecs

A MethodSpec is a function signature type of the following form:

- `ReturnType(MethodName, ArgumentTypes...)` : a non const method that can throw
- `ReturnType(MethodName, ArgumentTypes...) noexcept` : a non const method that
  can't throw
- `ReturnType(MethodName, ArgumentTypes...) const` : a const method that can
  throw
- `ReturnType(MethodName, ArgumentTypes...) const noexcept` : a const method
  that can't throw

`ReturnType` is the return type of the fully specified method.

`MethodName` is the [name](#name) of the method, and is the first parameter of
the function signature type.

`ArgumentTypes` are the arguments the method should accept.

poly provides the type trait `poly::is_method_spec_v` to check if a type is a
valid MethodSpec. When compiling with C++20 or above, poly also provides the
concept `poly::MethodSpecification`, which can be used to check if a type is a
MethodSpec.

#### Example

```cpp
#include "poly.hpp"
// the name
POLY_METHOD(calculate)

// calc1 specifies a const method with the name calculate, taking two 
// arguments of type double and returning a double
using calc1 = double(calculate, double,double) const;
// calc2 specifies a non throwing const method with the name calculate, 
// taking one argument of type double and returning a double
using calc2 = double(calculate, double) const noexcept;
// calc3 specifies a throwing non const method with the name calculate, 
// taking no arguments and returning nothing
using calc3 = void(calculate);
// not a MethodSpec, double is nopt a valid method name
using calc4 = void(double, double);

static_assert(poly::is_method_spec_v<calc1>);
static_assert(poly::is_method_spec_v<calc2>);
static_assert(poly::is_method_spec_v<calc3>);
static_assert(not poly::is_method_spec_v<calc4>);
```

### PropertySpecs

A PropertySpec is a function signature type of the following form:

- `PropertyName(ValueType)` : a mutable property
- `PropertyName(ValueType) noexcept` : a non throwing mutable property
- `const PropertyName(ValueType)` : an immutable property
- `const PropertyName(ValueType) noexcept` : an non throwing immutable property

`PropertyName` is the [name](#name) of the fully specified property.

`ValueType` is the type of the emulated member variable.

poly provides the type trait `poly::is_property_spec_v` to check if a type is a
valid PropertySpec. When compiling with C++20 or above, poly also provides the
concept `poly::PropertySpecification`, which can be used to check if a type is
a PropertySpec.

```cpp
#include "poly.hpp"
#include <chrono>
// the name
POLY_PROPERTY(timeout)

// specifies readable and writeable property with the name timeout, using
// std::chrono::milliseconds as its value type
using rw_timeout = timeout(std::chrono::milliseconds) noexcept;
// specifies read only property with the name timeout, using
// std::chrono::milliseconds as its value type
using ro_timeout = const timeout(std::chrono::milliseconds) noexcept;
```

## Storage

poly uses the concept of a Storage to store data in a type erased manner, and
provides a few implementations of the Storage concept.

For a type `T` to satisfy the Storage concept, the following must hold with `t`
and `ct` denoting an object of type `T` and `const T` respectively, `U` denoting
any arbitrary type constructible with arguments `args...`, and `u` denoting an
object of type `U&`:

- `T a;`: constructs the object a of type `T`. a is empty, i.e. `a.data()`
  returns nullptr
- `T a(std::move(t))`: move constructability is required
- `T a; a = std::move(t);`: move assignability is required
- `t.data()`: returns a `void*` to stored data, or nullptr if empty
- `ct.data()`: returns a const `void*` to stored data, or nullptr if empty
- one of the following emplace member functions, or both. Owning Storages
  usually provide the first option, non owning Storages the second:
  - `t.emplace<U>(args...)`: returns U&, i.e. reference to the value emplaced.
  - `t.emplace<U>(u)`: returns U&, typically u itself if non owning Storage.

`poly::is_storage_v<T>` can be used to check if  a `T` syntactically satisfies
the storage concept (also works with C++17). In C++20 and above, the concept
poly::Storage can be used.

A minimal class satisfying the Storage requirements is depicted below

```cpp
struct minimal_storage{
  minimal_storage();
  minimal_storage(minimal_storage&&);
  minimal_storage& operator=(minimal_storage&&);
                                                
  void* data();
  const void* data() const;
                                                
  // typical owning storage emplace
  template<typename T,typename...Args>
  T& emplace(Args&&args);
  // or below for non owning storage
  template<typename T>
  T& emplace(T& t);
};
```

poly provides the following storage classes and class templates:

- Non owing Storages:
  - `poly::ref_storage`:  Only contains `void*` to
    objects emplaced. Does not own the emplaced value, but merely references it.
- Storages without dynamic allocation
  - `poly::local_storage<Size,Align>`: stores any type without allocating
    memory, if `sizeof(type) <= Size` and `alignof(type) <= Align`. type must
    be move and copy constructible.
  - `poly::move_only_local_storage<Size,Align>`: same as above, but stored types
    only need to be move constructible. Cannot be copied.
  - `poly::varaint_storage<Ts...>`: Storage for any of the types in
    `Ts...`. Requires that every type in `Ts...` is at least move constructible.
    Is copyable if every type in `Ts...` is copy constructible.
- Storages with small buffer optimization.
  - `poly::sbo_storage<Size,Align>`: stores any type without allocating, if
    `sizeof(type) <= Size` and `alignof(type) <= Align`, else it heap allocates.
    type must be move and copy constructible.
  - `poly::move_only_storage<Size,Align>`: same as above, but types only need to
    be move constructible. Cannot be copied.
- Storages which always heap allocate.
  - `poly::heap_storage`: stores any move and copy constructible type on the
    heap. Can be used accross DLL boundaries if poly is compiled as a DLL.
  - `poly::move_only_heap_storage`: move only version of `poly::heap_storage`

## Type list

Type lists are used by poly to bundle method and property specs, as variadic
templates with multiple packs cannot be passed usually. A type list is any
template which can take an arbitrary amount of type parameters. poly has a type
list type, namely `poly::type_list`, but any template featuring a single
variadic type parameter pack works, for example `std::tuple`.

## poly::Struct

`poly::Struct` is one of two main class templates this library provides.
It contains a [storage](#storage) of type `StorageType` and a pointer to a "vtable", and
provides the properties and methods defined in the `PropertySpecs`
and`MethodSpecs` list. It is constexpr friendly, i.e. if the `StorageType` is
constexpr then `Struct` will also be constexpr constructible and methods
are callable at compile time, given that the underlying extension function is
constexpr. Accessing injected properties by name is the only thing not not
available in a constexpr context.

`poly::Struct` is defined in the following way (with C++20 concept notation):

```cpp
template <poly::Storage StorageType, 
          poly::PropertySpecList PropertySpecs,
          poly::MethodSpecList MethodSpecs>
class Struct;
```

### Template Parameters

`StorageType`
: the [storage](#storage) used to store arbitrary objects.

`PropertySpecs`
: a [type list](#type-list) of [PropertySpecs](#propertyspecs).

`MethodSpecs`
: a [type list](#type-list) of [MethodSpecs](#methodspecs).

### Inner typenames

`using property_specs = PropertySpecs;`

`using method_specs = MethodSpecs;`

### Constructors

`Struct` is constructible from other `Struct`s with compatible storages
and the same MethodSpecs and PropertySpecs in the same order(i.e.
`std::is_same_v<typename Struct::methodspecs,typename other_Struct::method_specs>`
is true, ditto for `Struct::property_specs`), and from types implementing the specified
properties and methods. The constructors are constexpr if the `StoraqgeType` is
constexpr capable.

#### `Struct(const Struct&)`

Copy constructor. Enabled if `std::is_copy_constructible_v<StorageType>` is
true.

#### `Struct(Struct&&)`

Move constructor

#### `Struct(const Struct<S,PropertySpecs,MethodSpecs>&)`

Copy construct from `Struct` with a different (but compatible) storage type. Enabled if
`std::is_constructible_v<StorageType,const S&>` is true.

#### `Struct(Struct<S,PropertySpecs,MethodSpecs>&&)`

Move construct from `Struct` with a different (but compatible) storage type. Enabled if
`std::is_constructible_v<StorageType, S&&>` is true.

#### `Struct(Struct<S,PropertySpecs,MethodSpecs>&)`

Constructs a `Struct` from an lvalue reference to another `Struct`.
Enabled if `std::is_same_v<StorageType, poly::ref_storage>` is true.

#### `Struct(T&&)`

Template Parameters | Description |
--------------------|-------------|
`T` | type of object to store |

Constructs a Struct containing a `std::decay_t<T>`, i.e. stores the decayed
version of `T` in its storage. `T` must implement the methods and properties
given in the `MethodSpecs` and `PropertySpecs` of the `Struct`.

### Assignment

`Struct` is assignable from other `Struct`s with compatible storages
and from types implementing the specified properties and methods.

#### `Struct::operator=(const Struct&)`

Copy assignment operator. Enabled if
`std::is_copy_assignable_v<StorageType>` is true.

#### `Struct::operator=(Struct&&)`

Move assignment operator.

#### `Struct::operator=(const Struct<S,PropertySpecs,MethodSpecs>&)`

Copy assign from `Struct` with a different storage type. Enabled if
`std::is_assignable_v<StorageType,const S&>` is true.

#### `Struct::operator=(Struct<S,PropertySpecs,MethodSpecs>&&)`

Move assign from `Struct` wtih a different storage type. Enabled if
`std::is_assignable_v<StorageType, S&&>` is true.

#### Struct::operator=(T&&)

Template Parameters | Description |
--------------------|-------------|
`T` | type of object to store |

Assigns a `T` to the `Struct`, i.e. stores the `T` in its storage. `T`
must implement the methods and properties given in the `MethodSpecs` and
`PropertySpecs`.

### <a name="struct-mem-funcs"></a>Member Functions

#### <a name="call-func"></a>`Ret Struct::call<Name>(Args&&...args) noexcept(/*as specified*/)`

Template Parameters | Description |
--------------------|-------------|
`Name` | name of the method to invoke. Must be explicitly provided as the first and only template parameter. |
`Args` | argument types of the method. These are deduced and should never be provided explicitly. |
`Ret` | return type of the method. Deduced and the exact same type as specified in the corresponding `MethodSpec`. |

This is the member function to use to invoke poly methods. It invokes the
method with the name `Name` and arguments `args` on the value stored in the
`Struct`. This requires that a method invocable with `args` is specified in
the `Struct`.

The combination of `Name` and `Args` does not have to match exactly to any of
the `MethodSpecs`. Standard C++ overload resolution and argument conversion
will select the overload called (if there are any overloaded methods).

If the `MethodSpec` of the selected overload is specified `noexcept`, `call`
will also be noexcept.

##### Example

```cpp
#include "poly.hpp"
POlY_METHOD(m1)
POLY_METHOD(m2)
// some arbitrary Storage type
using Storage = ...; 
// some arbitrary properties
using Properties = poly::type_list<...>; 
// a list of MethodSpecs consisting of the method with name m1 and the overloaded methods with name m2.
using Methods = poly::type_list<int  (m1)const
                                float(m2, int)const noexcept,
                                int  (m2, float)const>; 

// Objects C++ equivalent (with name injection):
// class Object{
// public:
//   int m1() const;
//   float m2(int) const;
//   int m2(float) const;
// };
using Object = poly::Struct<Storage,Properties,Methods>;

int use_object(const Object& obj, int i){
    // without name injection
    float calc1 = obj.call<m2>(i); // selects the integer parameter overload, just as expected
    int calc2 = obj.call<m2>(calc1); // selects the float parameter overload
    return calc2 + obj.call<m1>();

    // with name injection
    float calc1 = obj.m2(i);
    int calc2 = obj.m2(calc1);
    return calc2 + obj.m1();
}

```

#### `Ret Struct::call<Name>(Args&&...args) const noexcept(/*as specified*/)`

Same as [`Struct::call<Name>(args...)`](#call-func), but only for const
methods.

#### <a name="get-func"></a>`value_type_for<Name> Struct::get<Name>() const noexcept(/*as specified*/)`

Template Parameters | Description |
--------------------|-------------|
`Name` | name of the property to access. |

`get()` retrieves the value of a property. `get()` is `noexcept` if the
`PropertySpec` corresponding to `Name` is specified noexcept.

`get()` will always return a value, and never a reference.

##### Example

```cpp
#include "poly.hpp"

POLY_PROPERTY(p1)

// some arbitrary Storage type
using Storage = ...; 
// two properties: mutable p1 of type int, immutbale p2 of type float
using Properties = poly::type_list<p1(int), const p2(float)>; 
// some arbitrary methods
using Methods = poly::type_list<...>; 

// Objects C++ equivalent (with name injection):
// struct Object{
//   int p1;
//   const float p2;
// };
using Object = poly::Struct<Storage,Properties,Methods>;

float use_object(const Object& obj){
    // without name injection
    return obj.get<p1>() + obj.get<p2>();

    // with name injection
    return obj.p1 + obj.p2;
}
```

#### <a name="set-func"></a>`bool Struct::set<Name>(const value_type_for<Name>& new_value)`

Template Parameters | Description |
--------------------|-------------|
`Name` | name of the property to set. |

Sets the value of the property with name `Name` to `new_value`.

Returns whether `new_value` was successfully set or not.

`new_value` is not set if a `check()` function is defined for the object `t` of
type `T` currently stored in Struct, and `check(t, new_value)` returns
false. If `check()` is not defined, `set()` will always return true.

### <a name="injected-methods"></a>Injected Methods

Methods specified with a name that was created with the `POLY_METHOD` macro are
available in the public interface of Struct as a standard c++ member
function with that name.

#### Example

`method1` is an injectable method name (if name injection is enabled).
`method2` cannot be injected by name, but can still used with
[`call`](#call-func).

```cpp
#include "poly.hpp"

POLY_METHOD(my_method1)
struct my_method2{};

using Storage = ...;
using methods = poly::type_list<void(my_method1),void(my_method2)>;

using Object = poly::Struct<Storage,type_list<>,methods>;

void use_object(Object obj){
   obj.my_method1(); // injected method is a normal c++ member function
   obj.call<my_method1>(); // Also still works

   obj.my_method2(); // Compile error: no member function called my_method2 in Object.
   obj.call<my_method2>(); // Ok
}
```

### <a name="injected-properties"></a>Injected Properties

Properties specified with a name that was created with the `POLY_PROPERTY`
macro are available in the public interface of Struct as a standard c++
member variable with that name.

#### Example

`property1` is an injectable method name (if name injection is enabled).
`property2` cannot be injected by name, but can still used with
[`set`](#set-func) and [`get`](#get-func).

```cpp
#include "poly.hpp"

POLY_PROPERTY(property1)
struct property2{};

using Storage = ...;
using properties = poly::type_list<property1(float),const property2(float)>;

using Object = poly::Struct<Storage, properties, type_list<>>;

void use_object(Object obj){
   // injected members act like a normal c++ member variables
   float f = obj.property1; 
   obj.property1 = 2*f;

   obj.property2; // Compile error: no member called property2 in Object.
   obj.call<my_method2>(); // Ok
}
```

### Example

Everything together, a virtual interface which used to look like this:

```cpp
class DeviceBase{
public:
    virtual void write(const std::string&)=0;
    virtual std::string read()=0;
    virtual void timeout(std::chrono::milliseconds)=0;
    virtual std::chrono::milliseconds timeout()const=0;
    virtual ~DeviceBase()=0; // don't forget ;)
};

DeviceBase* create_device(ProtocolAgnosticAddress a);
```

is transformed into

```cpp
#include "poly.hpp"

POLY_METHOD(write)
POLY_METHOD(read)
POLY_PROPERTY(timeout)

using Device = poly::Struct<poly::sbo_storage<32>,
                                  poly::type_list<timeout(std::chrono::milliseconds)>,
                                  poly::type_list<void(write, const std::string&), 
                                                  std::string(read)>>;

Device create_device(ProtocolAgnosticAddress a);
```

## poly::Interface

`poly::Interface` is the other "polymorphic" class template poly provides. Just
like `poly::Struct`, it stores any type in its Storage and provides the
properties and methods specified. The main difference to `poly::Struct` is the
constructability/assignability restrictions. `Struct` instances can only
be constructed/assigned from instances of types implementing the specified methods and
properties, and other `Struct` instances featuring the exact same method
and property list (i.e. same order, same list type) and compatible storages.
`poly::Interface` is much more flexible. `Interfaces` can be
constructed/assigned from any `Struct` or `Interface` featuring the same or a
super set of the specified methods and properties. This does however necessitate
some additional stack space inside the `Interface` compared to `Struct`.

`poly::Interface` is defined in the following way (with C++20 concept notation):

```cpp
template <poly::Storage StorageType, 
          poly::PropertySpecList PropertySpecs,
          poly::MethodSpecList MethodSpecs>
class Interface;
```

### Template Parameters

`StorageType`
: the [storage](#storage) used to store arbitrary objects.

`PropertySpecs`
: a [type list](#type-list) of [PropertySpecs](#propertyspecs).

`MethodSpecs`
: a [type list](#type-list) of [MethodSpecs](#methodspecs).

### Inner typenames

`using property_specs = PropertySpecs;`

`using method_specs = MethodSpecs;`

### Constructors

#### `Interface(const Interface&)`

Copy constructor. Enabled if `std::is_copy_constructible_v<StorageType>` is
true.

#### `Interface(Interface&&)`

Move constructor

#### `Interface(const Interface<S, OtherPropertySpecs, OtherMethodSpecs>&)`

Copy construct from `Interface` with a different (but compatible) storage type.
Enabled if `std::is_constructible_v<StorageType,const S&>` is true.
`OtherPropertySpecs` and `OtherMethodSpecs` must be a super set of
`propertyspecs` and `method_specs`.

#### `Interface(Interface<S, OtherPropertySpecs, OtherMethodSpecs>&)`

Same as const version. Used for `poly::ref_storage`.

#### `Interface(Interface<S, OtherPropertySpecs, OtherMethodSpecs>&&)`

Move construct from `Interface` with a different (but compatible) storage type.
Enabled if `std::is_constructible_v<StorageType,S&&>` is true.
`OtherPropertySpecs` and `OtherMethodSpecs` must be a super set of
`propertyspecs` and `method_specs`.

#### `Interface(const Struct<S, OtherPropertySpecs, OtherMethodSpecs>&)`

Copy construct from `Struct` with a different (but compatible) storage type.
Enabled if `std::is_constructible_v<StorageType,const S&>` is true.
`OtherPropertySpecs` and `OtherMethodSpecs` must be a super set of
`propertyspecs` and `method_specs`.

#### `Interface(const Struct<S, OtherPropertySpecs, OtherMethodSpecs>&)`

Same as const version. Used for `poly::ref_storage`.

#### `Interface(Struct<S, OtherPropertySpecs, OtherMethodSpecs>&&)`

Move construct from `Struct` with a different (but compatible) storage type.
Enabled if `std::is_constructible_v<StorageType,S&&>` is true.
`OtherPropertySpecs` and `OtherMethodSpecs` must be a super set of
`propertyspecs` and `method_specs`.

### Assignment

Assignment versions of the constructors.

### Member functions

See `poly::Struct`s [member functions](#struct-mem-funcs). Ditto for [injected
methods](#injected-methods) and [properties](#injected-properties).

## Configuration

poly has a few configuration macros, which can be used to disable certain
aspects of the library. These must be defined BEFORE including any poly header,
i.e. using `#define POLY_XXX` before including poly headers.

- `POLY_DISABLE_MACROS`: disables the definition of function like macros.
- `POLY_DISABLE_INJECTION`: disables name injection for methods and properties.
- `POLY_DISABLE_METHOD_INJECTION`: disables name injection for methods.
- `POLY_DISABLE_PROPERTY_INJECTION`: disables name injection for properties.
- `POLY_DISABLE_DEFAULT_PROPERTY_ACCESS`: disables generation of default get()
  and set() functions when using the `POLY_PROPERTY` macro to define property names
- `POLY_DISABLE_DEFAULT_EXTEND`: disables generation of default extend()
  function when using the `POLY_METHOD` macro to define method names
- `POLY_MAX_METHOD_COUNT`: defines the maximum number of methods a
  `poly::Struct` can have when used in combination with `poly::Interface`.
  Defaults to 256. A static assertion will be triggered if this value is too
  low.
- `POLY_MAX_PROPERTY_COUNT`: defines the maximum number of properties a
  `poly::Struct` can have when used in combination with `poly::Interface`.
  Defaults to 256. A static assertion will be triggered if this value is too
  low.
- `POLY_HEADER_ONLY`: must be defined if poly is used as a header only library
- `POLY_COMPILING_LIBRARY`: must be defined when compiling the poly library (but
  not when using the library)

## Method Extension

To implement a method with the name `Name`, return type `Ret` and arguments
`Args...` for a type `T`, the following free function must be defined:

```cpp
// for non const methods, add noexcept if MethodSpec is noexcept
Ret extend(Name, T&t, Args... args){
    // ... implementation stuff
}

// for const methods, add noexcept if MethodSpec is noexcept
Ret extend(Name, const T&, Args... args){
    // ... implementation stuff
}
```

`extend()` is located through ADL, so it should be defined in the same namespace
as `Name` or `T`.

## Property Extension

Properties with name `Name` and value type `V` are
implemented for a type `T` by minimally providing the free function `get()`.
Both const and non const properties use `get()` to retrieve a property value.

```cpp
// add noexcept for noexcept properties
V get(Name, const T& t){
    // ... implementation stuff
    return V(...);
}
```

Mutable, i.e. non const, properties can also be modified. The `set()` function
must be provided to implement this behaviour.

```cpp
// add noexcept for noexcept properties
void set(Name, T& t, const V& new_value){
    // ... implementation stuff
}
```

In addition to `set()`, a `check()` function can optionally be provided.
`check()` checks if the new value to be set is valid. When `check()` exists,
then its return value is used to determine if `set()` is called (`check() ==
true`) or not.

```cpp
bool check(Name, const T&, const V& new_value){
    // determine if new_value is a valid value for an instance of T.
    // return true if new_value is valid, false otherwise
}
```

`get`, `set` and `check` are located through ADL, and as such should be defined
in the same namespace as `Name` or `T`.

## Considerations

This library relies on empty base class optimizations (EBCO) to get the smallest
size possible for the poly::Struct template. If EBCO is applied correctly,
the size of Struct will always be the size of the storage used plus the
size of a pointer.

This works well on linux with GCC and clang, as well as GCC on windows. In this
combination of compiler and platform, name injection will never incur any
overhead.

On windows, clang and msvc do generate some overhead here (clang aims to be
compatible with msvc), but only when using name injection with properties.
Concretely, if exactly one property with name injection is specified in a
Struct, the size of that Struct specialization will be one pointer
size greater than an object with a property without name injection. This size
overhead will stay constant, i.e. stay at the size of one pointer, until the
number of properties with name injection is greater than the size of a pointer.
Then the overhead will increase by an additional pointer size.

If this is unclear, some concrete math could help: Let N be the size of one
pointer, i.e. sizeof(void\*). Let n be the number of properties with name
injection specified in a Struct. The overhead O in bytes of that
Struct can be calculated as

O = ceil(n/N) * N

On a 64 bit platform, this would mean an overhead of 8 bytes for the first 8
properties with name injection added to a Struct, an overhead of 16 bytes
for n between 9 and 16, and so on.

Name injection with methods should never incur any space overhead.

macOS is not tested, as I do not have a mac.

The overhead of poly::Interface can be calculated similarily. In addition to
the overhead mentioned above, each property and method, regardless whether it
is named or not, will occupy one byte (or more, depending on
`POLY_MAX_METHOD/PROPERTY_COUNT`) of additional space. The size overhead O for an
Interface with M methods and P properties, of which p are named, can be calculated as follows:

O = (ceil(p/N) + ceil((M + P)/N)) * N

where N = sizeof(void\*).

## Building the library without meson

The step to produce a static/shared library of poly is quite simple.
Compile `poly/lib.cpp` with `POLY_COMPILING_LIBRARY` defined. Done.
