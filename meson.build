project(
    'spel',
    ['c', 'cpp'],
    version: '0.0.1',
    default_options: [
        'c_std=c2x',
        'cpp_std=c++20',
        'default_library=static',
        'b_lto=true',
        'b_ndebug=true',
    ],
)

add_project_arguments(['-D_POSIX_C_SOURCE=200809L'], language: 'c')

if get_option('debug')
    add_project_arguments(['-DDEBUG'], language: 'c')
else
    add_project_arguments(
        [
            '-Os',
            '-ffunction-sections',
            '-fdata-sections',
            '-fno-unwind-tables',
            '-fno-asynchronous-unwind-tables',
            '-fno-exceptions',
            '-fno-rtti',
        ],
        language: ['c', 'cpp'],
    )

    add_project_link_arguments(
        [
            '-Wl,--gc-sections',
            '-Wl,--strip-all',
            '-Wl,--exclude-libs,ALL',
            '-Wl,--no-as-needed',
        ],
        language: ['c', 'cpp'],
    )
endif

add_project_arguments(
    [
        '-fno-threadsafe-statics',
        '-fno-unwind-tables',
        '-fno-asynchronous-unwind-tables',
    ],
    language: 'c',
)

add_project_arguments(
    [
        '-fvisibility=hidden',
        '-fvisibility-inlines-hidden',
    ],
    language: 'cpp',
)

if get_option('sanitizers')
    add_project_arguments(
        [
            '-fsanitize=address,undefined',
            '-fno-omit-frame-pointer',
        ],
        language: ['c', 'cpp'],
    )

    add_project_link_arguments(
        [
            '-fsanitize=address,undefined',
        ],
        language: ['c', 'cpp'],
    )
endif

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: false)

cmake = import('cmake')
cmake_args = cmake.subproject_options()

cmake_args.append_compile_args('cpp', ['-fPIC'])
cmake_args.add_cmake_defines({'SPIRV_CROSS_ENABLE_TESTS': false, 'SPIRV_CROSS_ENABLE_CLI': false})

sources = [
    'src/core/entry.c',
    'src/core/memory.c',
    'src/core/log.c',
    'src/core/window.c',
    'src/core/event.c',
    'src/utils/math.c',
    'src/utils/time.c',
    'src/gfx/gfx.c',
    'src/utils/path.c',
    'src/gfx/gfx_pipeline.c',
    'src/utils/internal/xxh_x86dispatch.c',
    'src/utils/internal/xxhash.c',
    'src/utils/terminal.c',

    'src/gfx/backends/gl/gfx_context_gl.c',
    'src/gfx/backends/gl/gfx_cmdlist_gl.c',
    'src/gfx/backends/gl/gfx_buffer_gl.c',
    'src/gfx/backends/gl/gfx_shader_gl.c',
    'src/gfx/backends/gl/gfx_pipeline_gl.c',
    'src/gfx/backends/gl/gfx_texture_gl.c',
]

dependencies = [
    dependency('sdl3'),
    m_dep,
    cmake.subproject('SPIRV-Cross').dependency('spirv-cross-core'),
]

spel_lib = library(
    'spel',
    sources: sources,
    include_directories: include_directories('spel'),
    dependencies: dependencies,
)

spel_dep = declare_dependency(
    link_with: spel_lib,
    dependencies: dependencies,
    include_directories: include_directories('spel'),
)

# tools
sh2spv = executable(
    'spel-sh2spv',
    sources: ['tools/sh2spv.cpp'],
    dependencies: [dependency('shaderc')],
)

sandbox_src = ['sandbox/main.c']
executable('sandbox', sources: sandbox_src, dependencies: spel_dep)
