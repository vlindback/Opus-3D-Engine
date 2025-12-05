# foundation/vulkan/meson.build

# Find the Vulkan SDK dependency
vulkan_dep = dependency('vulkan', required: true)

# Define the Vulkan module library
vulkan_lib = static_library(
    'vulkan',
    'vulkan_stub.cpp',
    #'vulkan_source.cpp',  # Add your source files here
    dependencies: vulkan_dep,
    # Add other configuration options
)

# Export the library for use by the 'engine' or other parts of 'foundation'
foundation_modules += vulkan_lib