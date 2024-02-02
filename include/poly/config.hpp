/**
 *  Copyright 2024 Pelé Constam
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#ifndef POLY_CONFIG_HPP
#define POLY_CONFIG_HPP

#ifdef POLY_DISABLE_MACROS
#define POLY_USE_MACROS 0
#else
#define POLY_USE_MACROS 1
#endif

#ifdef POLY_DISABLE_INJECTION
#define POLY_USE_INJECTOR 0
#else
#define POLY_USE_INJECTOR 1
#endif

#ifdef POLY_DISABLE_METHOD_INJECTION
#define POLY_USE_METHOD_INJECTOR 0
#else
#define POLY_USE_METHOD_INJECTOR POLY_USE_INJECTOR
#endif

#ifdef POLY_DISABLE_PROPERTY_INJECTION
#define POLY_USE_PROPERTY_INJECTOR 0
#else
#define POLY_USE_PROPERTY_INJECTOR POLY_USE_INJECTOR
#endif

#ifdef POLY_DISABLE_DEFAULT_PROPERTY_ACCESS
#define POLY_USE_DEFAULT_PROPERTY_ACCESS 0
#else
#define POLY_USE_DEFAULT_PROPERTY_ACCESS 1
#endif

#ifdef POLY_DISABLE_DEFAULT_EXTEND
#define POLY_USE_DEFAULT_EXTEND 0
#else
#define POLY_USE_DEFAULT_EXTEND 1
#endif

#ifndef POLY_MAX_METHOD_COUNT
#define POLY_MAX_METHOD_COUNT 256
#endif

#ifndef POLY_MAX_PROPERTY_COUNT
#define POLY_MAX_PROPERTY_COUNT 256
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1900) 
// needed for msvc to get EBCO right
#define POLY_EMPTY_BASE __declspec(empty_bases)
#else
#define POLY_EMPTY_BASE
#endif

#if __has_cpp_attribute(no_unique_address)
#define POLY_NO_UNIQUE_ADDRESS [[no_unique_address]]
#elif defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(__clang__) && (__cplusplus >= 202002L)
#define POLY_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define POLY_NO_UNIQUE_ADDRESS
#endif

#if __cplusplus >= 202002L
// enabled if c++ std >= c++20
#define POLY_STORAGE ::poly::Storage
#define POLY_METHOD_SPEC ::poly::MethodSpecification
#define POLY_PROP_SPEC ::poly::PropertySpecification
#define POLY_TYPE_LIST ::poly::TypeList
#define POLY_CONSTEXPR constexpr
#else
#define POLY_STORAGE typename
#define POLY_METHOD_SPEC typename
#define POLY_PROP_SPEC typename
#define POLY_TYPE_LIST typename
#define POLY_CONSTEXPR
#endif

#endif
