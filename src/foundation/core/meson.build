# foundation/core/meson.build

# Local headers (e.g., stuff in src/foundation/core/include)
local_inc_paths = ['include', 'src']

core_sources = files(
    'src/panic.cpp',
    'src/system_error.cpp'
)

if host_machine.system() == 'windows'
    local_inc_paths += 'src/platform/windows'
    core_sources += files('src/platform/windows/win32_helpers.cpp')
endif

local_inc = include_directories(local_inc_paths)

core_lib = static_library(
  'core',
  [core_sources], # Meson flattens nested lists automatically
  include_directories: [local_inc, opus_src_root],
)

core_dep = declare_dependency(
    link_with: core_lib,
    include_directories: [local_inc, opus_src_root]
)

# Export to the foundation module list
foundation_modules += core_lib