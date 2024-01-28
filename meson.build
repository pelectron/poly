project('poly', 'cpp', default_options:['cpp_std=c++20'])
inc = include_directories('include')
catch_dep = dependency('catch2',
  default_options:['tests=false'],
  fallback: ['catch2', 'catch2_with_main_dep'],
  version:  '>=3.4.0')
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

test_exe = executable('main', 
                      sources:[ 'tests/function.cpp',
                                'tests/interface.cpp', 
                                'tests/methods.cpp',
                                'tests/properties.cpp',
                                'tests/storage.cpp'],
                      include_directories:inc,
                      cpp_args:args+['-ftemplate-backtrace-limit=0', '-ferror-limit=1'],
                      dependencies:catch_dep)
test('poly unit tests', test_exe)
