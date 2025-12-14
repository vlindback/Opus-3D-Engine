# foundation/meson.build

# --- Global Setup ---

# List to hold the libraries compiled in subdirectories (vulkan, fibers, etc.)
# This list will be linked into the main foundation_lib.
foundation_modules = []

# --- Subdirectories and Dependencies ---

subdir('vulkan')
subdir('fibers')
# subdir('window') # Future module
# subdir('audio')  # Future module

# --- Define the Core Foundation Library ---

foundation_sources = files(
    'foundation_placeholder.cpp'
    # Add any other source files specific to the base foundation layer
)

foundation_lib = static_library(
    'foundation',
    foundation_sources,
    link_whole: foundation_modules,
    include_directories: include_directories('.'), # Include foundation headers
    cpp_args: '-D_OPUS_FOUNDATION_BUILD',
    install: true,
)

# --- Foundation as a dependency ---

# Export the compiled foundation library as a dependency object.
# This allows other parts of the project (like the 'engine' or 'game')
# to easily link against the entire Foundation layer using:
# foundation_dep = dependency('foundation')
foundation_dep = declare_dependency(
    link_with: foundation_lib,
    include_directories: include_directories('.'),
)

# Makes this available to the root meson.build (which includes this subdir):
set_variable('foundation_dep', foundation_dep)