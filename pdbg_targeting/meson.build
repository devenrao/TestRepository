# Include libfdt
subdir('dtc/libfdt')

targeting_sources = files(
  'main.C',
  'target.C',
  'target_service.C',
  'dt_loader.C'
)

executable('targeting-app',
  targeting_sources,
  include_directories: [libfdt_inc],
  dependencies: libfdt_dep,
)
