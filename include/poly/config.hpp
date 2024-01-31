#ifndef POLY_CONFIG_HPP
#define POLY_CONFIG_HPP

#ifdef POLY_DISABLE_MACROS
#if POLY_DISABLE_MACROS == 1
#define POLY_USE_MACROS 0
#else
#define POLY_USE_MACROS 1
#endif
#else
#define POLY_USE_MACROS 1
#endif

#ifdef POLY_DISABLE_INJECTION
#if POLY_DISABLE_INJECT == 1
#define POLY_USE_INJECTOR 0
#else
#define POLY_USE_INJECTOR 1
#endif
#else
#define POLY_USE_INJECTOR 1
#endif

#ifdef POLY_DISABLE_METHOD_INJECTION
#if (POLY_DISABLE_METHOD_INJECTION == 1) || (POLY_USE_INJECTOR == 0)
#define POLY_USE_METHOD_INJECTOR 0
#else
#define POLY_USE_METHOD_INJECTOR 1
#endif
#else
#define POLY_USE_METHOD_INJECTOR POLY_USE_INJECTOR
#endif

#ifdef POLY_DISABLE_PROPERTY_INJECTION
#if (POLY_DISABLE_PROPERTY_INJECTION == 1) || (POLY_USE_INJECTOR == 0)
#define POLY_USE_PROPERTY_INJECTOR 0
#else
#define POLY_USE_PROPERTY_INJECTOR 1
#endif
#else
#define POLY_USE_PROPERTY_INJECTOR POLY_USE_INJECTOR
#endif

#ifdef POLY_DISABLE_DEFAULT_PROPERTY_ACCESS
#if POLY_DISABLE_DEFAULT_PROPERTY_ACCESS == 1
#define POLY_USE_DEFAULT_PROPERTY_ACCESS 0
#else
#define POLY_USE_DEFAULT_PROPERTY_ACCESS 1
#endif
#else
#define POLY_USE_DEFAULT_PROPERTY_ACCESS 1
#endif

#ifdef POLY_DISABLE_DEFAULT_EXTEND
#if POLY_DISABLE_DEFAULT_EXTEND == 1
#define POLY_USE_DEFAULT_EXTEND 0
#else
#define POLY_USE_DEFAULT_EXTEND 1
#endif
#else
#define POLY_USE_DEFAULT_EXTEND 1
#endif

#ifndef POLY_MAX_METHOD_COUNT
#define POLY_MAX_METHOD_COUNT 256
#endif
#ifndef POLY_MAX_PROPERTY_COUNT
#define POLY_MAX_PROPERTY_COUNT 256
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1900
#define POLY_EMPTY_BASE __declspec(empty_bases)
#else
#define POLY_EMPTY_BASE
#endif

#if defined(_MSC_VER)
#define POLY_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define POLY_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

#if __cplusplus > 201703L
// enabled if c++ std >= c++20
#define POLY_STORAGE ::poly::Storage
#define POLY_METHOD_SPEC ::poly::MethodSpecification
#define POLY_PROP_SPEC ::poly::PropertySpecification
#else
#define POLY_STORAGE typename
#define POLY_METHOD_SPEC typename
#define POLY_PROP_SPEC typename
#endif

#endif
