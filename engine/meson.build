engine_inc = include_directories('include')

engine_lib = library(
    'engine',
    sources: files('src/**.cpp'),
    include_directories: engine_inc,
    dependencies: foundation_dep,
)

engine_dep = declare_dependency(
    link_with: engine_lib,
    include_directories: engine_inc,
    dependencies: foundation_dep
)