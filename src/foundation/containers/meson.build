# foundation/containers/meson.build

containers_sources = files(
    'src/string.cpp',
    'src/container_error.cpp'
)

local_inc = include_directories('include')

containers_lib = static_library(
  'containers',
  [containers_sources], # Meson flattens nested lists automatically
  include_directories: [local_inc, opus_src_root],
)

# Export to the foundation module list
foundation_modules += containers_lib