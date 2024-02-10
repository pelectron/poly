project('poly', 'cpp', 
        default_options:['cpp_std=c++20', 'warning_level=3'], 
        license:['Apache-2.0'], version:'0.1')

inc = include_directories('include')

# needed for testing
catch_dep = dependency( 'catch2',
                        default_options:['tests=false'],
                        fallback: ['catch2', 'catch2_with_main_dep'],
                        version:  '>=3.4.0',
                        required: false)

opts = ['macros', 
        'injection', 
        'method_injection', 
        'property_injection', 
        'default_property_access', 
        'default_extend']

args = []
foreach o: opts
  if get_option(o) == false
    o = o.to_upper()
    args += f'-DPOLY_DISABLE_@o@'
  endif
endforeach

extra_args = []

id = meson.get_compiler('cpp').get_id()

if 'gcc' == id
  extra_args += ['-Wno-self-move']
endif

if 'clang' == id
  extra_args += ['-Wno-self-assign-overloaded']
endif

if get_option('header_only')
  args += ['-DPOLY_HEADER_ONLY']
  poly_dep = declare_dependency(compile_args: args, 
                                include_directories: inc)
else
  poly_lib = library('poly',
                      include_directories: inc,
                      sources: ['include/poly/lib.cpp'],
                      gnu_symbol_visibility: 'hidden',
                      cpp_args: args + '-DPOLY_COMPILING_LIBRARY')
  poly_dep = declare_dependency(compile_args: args, 
                                include_directories: inc,
                                link_with: poly_lib)
endif

install_headers('include/poly.hpp')
install_headers('include/poly/alloc.hpp',
                'include/poly/always_false.hpp',
                'include/poly/config.hpp',
                'include/poly/function.hpp',
                'include/poly/fwd.hpp',
                'include/poly/interface.hpp',
                'include/poly/interface_method_entry.hpp',
                'include/poly/interface_property_entry.hpp',
                'include/poly/method.hpp',
                'include/poly/method_table.hpp',
                'include/poly/property.hpp',
                'include/poly/property_table.hpp',
                'include/poly/storage.hpp',
                'include/poly/struct.hpp',
                'include/poly/traits.hpp',
                'include/poly/type_list.hpp',
                subdir: 'poly')

if get_option('tests')
  if not catch_dep.found()
    error('Catch depdendency was not found, but the \'test\' option was',
      'specified. Set \'tests\' to false if Catch and/or Wrap DB is',
      'unavailable.')
  endif
  test_args = args+extra_args
  test_exe = executable('main', 
                        sources:[ 'tests/function.cpp',
                                  'tests/interface.cpp', 
                                  'tests/methods.cpp',
                                  'tests/properties.cpp',
                                  'tests/storage.cpp'],
                        include_directories:inc,
                        cpp_args:test_args,
                        dependencies:[poly_dep, catch_dep])
  size_exe = executable('size', 
                        sources:[ 'tests/size.cpp'],
                        include_directories:inc,
                        cpp_args:test_args,
                        dependencies:[poly_dep])
  ebo_exe = executable('ebo', 
                        sources:[ 'tests/ebo.cpp'],
                        include_directories:inc,
                        cpp_args:test_args,
                        dependencies:[poly_dep])
  test('poly unit tests', test_exe)
endif
