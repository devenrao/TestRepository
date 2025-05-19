cxx = meson.get_compiler('cpp')

sdbusplus = dependency(
    'sdbusplus',
    fallback: [
        'sdbusplus',
        'sdbusplus_dep'
    ],  
)

pdbg_deps = [ 
    cxx.find_library('pdbg'),
    cxx.find_library('fdt'),
    cxx.find_library('ekb'),
    cxx.find_library('dt-api'),
    cxx.find_library('phal'),
    cxx.find_library('ipl')
]

systemd = dependency('systemd')

# Hardcode or detect base path
libekb_base = '/home/devenrao/openbmc/build/p10bmc/workspace/sources/libekb'

# Create include directories
ekb_includes = include_directories(
  libekb_base + '/ekb/hwpf/fapi2/include',
)

phosphor_logging = dependency(
    'phosphor-logging',
    fallback: ['phosphor-logging', 'phosphor_logging_dep'],
    )

executable(
    'ffdcpozpel',
    'ffdcpozpel.cpp',
    dependencies: [ sdbusplus, pdbg_deps, systemd, phosphor_logging ],
    include_directories: ekb_includes,
	install:true,
)
executable(
    'ffdcpel',
    'ffdcpel.cpp',
    dependencies: [ sdbusplus, pdbg_deps, systemd, phosphor_logging ],
	install:true,
)
