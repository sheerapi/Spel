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
    meson_version: '1.10.0',
)

compile_args = []
link_args = []

compile_args += ['-D_POSIX_C_SOURCE=200809L', '-D_XOPEN_SOURCE=700']

if get_option('debug')
    add_project_arguments(['-DDEBUG'], language: 'c')
    compile_args += ['-fno-omit-frame-pointer', '-fasynchronous-unwind-tables']
else
    compile_args += [
        '-Os',
        '-ffunction-sections',
        '-fdata-sections',
        '-fno-unwind-tables',
        '-fno-asynchronous-unwind-tables',
        '-fno-exceptions',
        '-fno-rtti',
    ]

    link_args += [
        '-Wl,--gc-sections',
        '-Wl,--strip-all',
        '-Wl,--exclude-libs,ALL',
        '-Wl,--no-as-needed',
    ]
endif

add_project_arguments(
    [
        '-fvisibility=hidden',
        '-fvisibility-inlines-hidden',
    ],
    language: 'cpp',
)

if get_option('sanitizers')
    compile_args += [
        '-fsanitize=address,undefined',
        '-fno-omit-frame-pointer',
    ]

    link_args += [
        '-fsanitize=address,undefined',
    ]
endif

add_project_arguments(compile_args, language: ['c', 'cpp'])
add_project_link_arguments(link_args, language: ['c', 'cpp'])

cc = meson.get_compiler('c')

conf = configuration_data()
conf.set('SPEL_COMPILER_ID', cc.get_id())
conf.set('SPEL_COMPILER_VERSION', cc.version())
conf.set(
    'SPEL_TARGET_TRIPLE',
    host_machine.system() + '-' + host_machine.cpu_family(),
)

conf.set('SPEL_BUILD_TYPE', get_option('buildtype'))
conf.set('SPEL_COMPILER_ARGS', ' '.join(compile_args))
conf.set('SPEL_LINKER_ARGS', ' '.join(link_args))
conf.set('SPEL_MESON_BACKEND', meson.backend())
conf.set('SPEL_LINKER_ID', cc.get_linker_id())

vcs = custom_target(
    input: 'build-aux/build_info_vcs.in.h',
    output: 'build_info_vcs.generated.h',
    command: ['tools/git-commit.sh', '@INPUT@', '@OUTPUT@'],
)

conf_file = configure_file(
    input: 'build-aux/build_info.in.h',
    output: 'build_info.generated.h',
    configuration: conf,
)

m_dep = cc.find_library('m', required: false)

cmake = import('cmake')
cmake_args = cmake.subproject_options()

cmake_args.append_compile_args('cpp', ['-fPIC'])
cmake_args.add_cmake_defines({'SPIRV_CROSS_ENABLE_TESTS': false, 'SPIRV_CROSS_CLI': false})

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
    'src/core/panic.c',
    'src/core/build_info.c',
    'src/core/env_info.c',

    'src/gfx/backends/gl/gfx_context_gl.c',
    'src/gfx/backends/gl/gfx_cmdlist_gl.c',
    'src/gfx/backends/gl/gfx_buffer_gl.c',
    'src/gfx/backends/gl/gfx_shader_gl.c',
    'src/gfx/backends/gl/gfx_pipeline_gl.c',
    'src/gfx/backends/gl/gfx_texture_gl.c',

    vcs,
    conf_file,
]

dependencies = [
    dependency('sdl3'),
    m_dep,
    cmake.subproject('SPIRV-Cross', options: cmake_args).dependency('spirv-cross-core'),
    cmake.subproject('libunwind', options: cmake_args).dependency('libunwind'),
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
