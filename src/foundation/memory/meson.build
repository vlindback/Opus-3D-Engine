# foundation/memory/meson.build

memory_sources = files(
    'src/heap_allocator.cpp',
    'src/linear_allocator.cpp',
    'src/memory_error.cpp',
    'src/virtual_range.cpp',
)

local_inc_paths = ['include']

if host_machine.system() == 'windows'
    local_inc_paths += 'src/platform/windows'
    memory_sources += [
        'src/platform/windows/pages_win32.cpp'
    ]
elif host_machine.system() == 'linux'
    local_inc_paths += 'src/platform/linux'
     memory_sources += [
        'src/platform/linux/pages_linux.cpp'
    ]
endif

local_inc = include_directories(local_inc_paths)

memory_lib = static_library(
  'memory',
  [memory_sources], # Meson flattens nested lists automatically
  include_directories: [local_inc, opus_src_root],
)

# Export to the foundation module list
foundation_modules += memory_lib