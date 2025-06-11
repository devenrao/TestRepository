# Include libfdt
project(
  'pdbg_targeting',
  'cpp',
  'c',
  version: '0.1',
  meson_version: '>=1.0.1',
  default_options: [
    'warning_level=3',
    'cpp_std=c++23',
  ])

cpp = meson.get_compiler('cpp')

subdir('dtc/libfdt')

targeting_sources = files(
  'main.C',
  'target.C',
  'target_service.C',
  'dtree_loader.C',
  'targeting/common/entitypath.C',
)

targeting_inc = include_directories(
  'targeting', 
  'targeting/common',
  'targeting/adapters',
)

executable('targeting-app',
  targeting_sources,
  include_directories: [libfdt_inc, targeting_inc],
  dependencies: libfdt_dep,
)
