poly enables and/or emulates polymorphic use of member variables and member
functions. It does so by providing the template `poly::basic_object`.
The polymorphic member variables of `poly::basic_object` are referred to as
**properties**. The polymorphic member functions of `poly::basic_object` are called
**methods**.

## Names

Properties and methods have a name associated with them, just like
any ordinary member variable or function has a name.

In poly, the name of a method or property is an empty, fully defined struct, i.e.
`struct any_name{};`.

poly also provides name injection via use of the POLY_METHOD and POLY_PROPERTY
macros. Defining method and property names with these macros will enable the
usage of said method or property directly by name in a `poly::basic_object`
featuring it.

```cpp
#include "poly.hpp"
// defines the method name my_method
POLY_METHOD(my_method)
// defines the propety name my_property
POLY_PROPERTY(my_property)

// obj refers to an instance of poly::basic_object with a method called my_method and
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
// not a MethodSpec
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
provides a few default implementations of the Storage concept.

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

`poly::is_storage_v` can be used to check if T syntactically satisfies the
storage concept (also works with C++17). In C++20 and above, the concept
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
    objects "stored"
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

## Type list

Type lists are used by poly to bundle method and property specs, as variadic
templates with multiple packs cannot be passed usually. A type list is any
template which can take an arbitrary amount of type parameters. poly has a type
list type, namely `poly::type_list`, but any template featuring a single
variadic type parameter pack works, for example `std::tuple`.

## poly::basic_object

`poly::basic_object` is one of two main class templates this library provides.
It contains a storage of type `StorageType`, a pointer to a "vtable", and
provides the properties and methods defined in the `PropertySpecs`
and`MethodSpecs` list.

`poly::basic_object` is defined in the following way (with C++20 concept notation):

```cpp
template <poly::Storage StorageType, 
          poly::PropertySpecList PropertySpecs,
          poly::MethodSpecList MethodSpecs>
class basic_object;
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

`method_specs = MethodSpecs;`

### Constructor

`basic_object` is constructible from other `basic_object`s with compatible storages
and the same MethodSpecs and PropertySpecs in the same order(i.e.
`std::is_same_v<typename basic_object::methodspecs,typename other_basic_object::method_specs>`
is true, ditto for `basic_object::property_specs`) and from types implementing the specified
properties and methods.

#### `basic_object(const basic_object&)`

Copy constructor. Enabled if `std::is_copy_constructible_v<StorageType>` is
true.

#### `basic_object(basic_object&&)`

Move constructor

#### `basic_object(const basic_object<S,PropertySpecs,MethodSpecs>&)`

Copy construct from `basic_object` with a different storage type. Enabled if
`std::is_constructible_v<StorageType,const S&>` is true.

#### `basic_object(basic_object<S,PropertySpecs,MethodSpecs>&&)`

Move construct from `basic_object` wtih a different storage type. Enabled if
`std::is_constructible_v<StorageType, S&&>` is true.

#### `basic_object(basic_object<S,PropertySpecs,MethodSpecs>&)`

Constructs a `basic_object` from an lvalue reference to another `basic_object`.
Enabled if `std::is_same_v<StorageType, poly::ref_storage>` is true.

#### `basic_object(T&&)`

Template Parameters | Description |
--------------------|-------------|
`T` | type of object to store |

Constructs a basic_object containing a `T`, i.e. stores the `T` in its
storage. `T` must implement the methods and properties given in the
`MethodSpecs` and `PropertySpecs` of the `basic_object`.

### Assignment

`basic_object` is assignable from other `basic_object`s with compatible storages
and from types implementing the specified properties and methods.

#### `basic_object::operator=(const basic_object&)`

Copy assignment operator. Enabled if
`std::is_copy_assignable_v<StorageType>` is true.

#### `basic_object::operator=(basic_object&&)`

Move assignment operator.

#### `basic_object::operator=(const basic_object<S,PropertySpecs,MethodSpecs>&)`

Copy assign from `basic_object` with a different storage type. Enabled if
`std::is_assignable_v<StorageType,const S&>` is true.

#### `basic_object::operator=(basic_object<S,PropertySpecs,MethodSpecs>&&)`

Move assign from `basic_object` wtih a different storage type. Enabled if
`std::is_assignable_v<StorageType, S&&>` is true.

#### basic_object::operator=(T&&)

Template Parameters | Description |
--------------------|-------------|
`T` | type of object to store |

Assigns a `T` to the `basic_object`, i.e. stores the `T` in its storage. `T`
must implement the methods and properties given in the `MethodSpecs` and
`PropertySpecs`.

### Member Functions

#### `Ret basic_object::call<Name>(Args&&...args) noexcept(/*as specified*/)`

Template Parameters | Description |
--------------------|-------------|
`Name` | name of the method to invoke. Must be explicitly provided as the first and only template parameter. |
`Args` | argument types of the method. These are deduced and should never be provided explicitly. |
`Ret` | return type of the method. Deduced and the exact same type as specified in the corresponding `MethodSpec`. |

This is the member function to use to invoke poly methods. It invokes the
method with the name `Name` and arguments `args` on the value stored in the
`basic_object`. This requires that a method invocable with `args` is specified in
the `basic_object`.

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
using Object = poly::basic_object<Storage,Properties,Methods>;

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

#### `Ret basic_object::call<Name>(Args&&...args) const noexcept(/*as specified*/)`

Same as
[`basic_object::call<Name>(args...)`](<#ret-basic_object%3A%3Acall%3Cname%3E(args%26%26...args)-noexcept(%2F*as-specified*%2F)>),
but only for const methods.

#### `value_type_for<Name> basic_object::get<Name>() const noexcept(/*as specified*/)`

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
using Object = poly::basic_object<Storage,Properties,Methods>;

float use_object(const Object& obj){
    // without name injection
    return obj.get<p1>() + obj.get<p2>();

    // with name injection
    return obj.p1 + obj.p2;
}
```

#### `bool basic_object::set<Name>(const value_type_for<Name>& new_value)`

Template Parameters | Description |
--------------------|-------------|
`Name` | name of the property to set. |

Sets the value of the property with name `Name` to `new_value`.

Returns whether `new_value` was successfully set or not.

`new_value` is not set if a `check()` function is defined for the object `t` of
type `T` currently stored in basic_object, and `check(t, new_value)` returns
false. If `check()` is not defined, `set()` will always return true.

### Injected Methods

Methods specified with a name that was created with the `POLY_METHOD` macro are
available in the public interface of basic_object as a standard c++ member
function with that name.

#### Example

`method1` is an injectable method name (if name injection is enabled).
`method2` cannot be injected by name, but can still used with
[`call`](<#ret-basic_object%3A%3Acall%3Cname%3E(args%26%26...args)-noexcept(%2F*as-specified*%2F)>).

```cpp
#include "poly.hpp"

POLY_METHOD(my_method1)
struct my_method2{};

using Storage = ...;
using methods = poly::type_list<void(my_method1),void(my_method2)>;

using Object = poly::basic_object<Storage,type_list<>,methods>;

void use_object(Object obj){
   obj.my_method1(); // injected method is a normal c++ member function
   obj.call<my_method1>(); // Also still works

   obj.my_method2(); // Compile error: no member function called my_method2 in Object.
   obj.call<my_method2>(); // Ok
}
```

### Injected Properties

Properties specified with a name that was created with the `POLY_PROPERTY`
macro are available in the public interface of basic_object as a standard c++
member variable with that name.

#### Example

`property1` is an injectable method name (if name injection is enabled).
`property2` cannot be injected by name, but can still used with
[`set`](<#bool-basic_object%3A%3Aset%3Cname%3E(const-value_type_for%3Cname%3E%26-new_value)>)
and
[`get`](<#value_type_for%3Cname%3E-basic_object%3A%3Aget%3Cname%3E()-const-noexcept(%2F*as-specified*%2F)>).

```cpp
#include "poly.hpp"

POLY_PROPERTY(property1)
struct property2{};

using Storage = ...;
using properties = poly::type_list<property1(float),const property2(float)>;

using Object = poly::basic_object<Storage, properties, type_list<>>;

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

using Device = poly::basic_object<poly::sbo_storage<32>,
                                  poly::type_list<timeout(std::chrono::milliseconds)>,
                                  poly::type_list<void(write, const std::string&), 
                                                  std::string(read)>>;

Device create_device(ProtocolAgnosticAddress a);
```

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
- `POLY_MAX_METHOD_COUNT`: defines the maximum number of methods an object can have.
  Defaults to 256. A static assertion will be triggered if this value is too low.
- `POLY_MAX_PROPERTY_COUNT`: defines the maximum number of properties an object can have.
  Defaults to 256. A static assertion will be triggered if this value is too low.
