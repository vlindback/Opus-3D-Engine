# foundation/meson.build

# --- 1. Global Setup ---

# List to hold the libraries compiled in subdirectories (vulkan, fibers, etc.)
# This list will be linked into the main foundation_lib.
foundation_modules = []

# --- 2. Include Subdirectories and Dependencies ---

# Include the build files for each module.
# The meson.build file in each subdir is responsible for:
# a) Finding its external dependencies (Vulkan, Boost.Context).
# b) Compiling its own static library (e.g., vulkan_lib, fibers_lib).
# c) Appending its compiled library to the 'foundation_modules' list.

subdir('vulkan')
subdir('fibers')
# subdir('window') # Future module
# subdir('audio')  # Future module

# --- 3. Define the Core Foundation Library ---

# Collect source files that belong directly to the foundation/ directory
foundation_sources = files(
    'foundation_placeholder.cpp'
    #'memory.cpp',
    #'platform_stuff.cpp',
    # Add any other source files specific to the base foundation layer
)

# Define the main foundation library.
# It compiles the core sources AND links the compiled module libraries.
foundation_lib = static_library(
    'foundation',
    foundation_sources,
    link_whole: foundation_modules,
    include_directories: include_directories('.'), # Include foundation headers
    # Add any common C/C++ arguments here
    cpp_args: '-D_OPUS_FOUNDATION_BUILD',
)

# --- 4. Export the Library ---

# Export the compiled foundation library as a dependency object.
# This allows other parts of your project (like the 'engine' or 'game')
# to easily link against the entire Foundation layer using:
# foundation_dep = dependency('foundation')
foundation_dep = declare_dependency(
    link_with: foundation_lib,
    include_directories: include_directories('.'),
)

# To make this available to the root meson.build (which includes this subdir):
set_variable('foundation_dep', foundation_dep)