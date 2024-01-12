project('properties_test', 'cpp', default_options:['cpp_std=c++20'])
inc = include_directories('include')
catch_dep = dependency('catch2',
  default_options:['tests=false'],
  fallback: ['catch2', 'catch2_with_main_dep'],
  version:  '>=3.4.0')
executable('main', 
  'tests/interface.cpp', 
  'tests/methods.cpp',
  'tests/properties.cpp',
  'tests/storage.cpp',
  include_directories:inc,
  dependencies:catch_dep)

