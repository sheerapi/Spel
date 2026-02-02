project(
    'spel',
    ['c', 'cpp'],
    version: '0.0.1',
    default_options: [
        'c_std=c17',
        'cpp_std=c++20',
        'default_library=static',
        'b_lto=true',
        'b_staticpic=true',
    ],
    meson_version: '>= 1.10.0',
)

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
    'src/utils/internal/execinfo.c',
    'src/utils/internal/stacktraverse.c',
    'src/utils/terminal.c',
    'src/core/panic.c',
    'src/utils/build_info.c',
    'src/core/env_info.c',

    'src/gfx/backends/gl/gfx_context_gl.c',
    'src/gfx/backends/gl/gfx_cmdlist_gl.c',
    'src/gfx/backends/gl/gfx_buffer_gl.c',
    'src/gfx/backends/gl/gfx_shader_gl.c',
    'src/gfx/backends/gl/gfx_pipeline_gl.c',
    'src/gfx/backends/gl/gfx_texture_gl.c',
]

shaders = ['shaders/spel_internal_2d.glsl', 'shaders/spel_internal_fullscreen.glsl']

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

compile_args = []
link_args = []

rel_compile_args = []
rel_link_args = []

if cc.get_id() != 'msvc'
    rel_compile_args = [
        '-Os',
        '-ffunction-sections',
        '-fdata-sections',
        '-fno-unwind-tables',
        '-fno-asynchronous-unwind-tables',
        '-fno-rtti',
    ]
    rel_link_args = [
        '-Wl,--gc-sections',
        '-Wl,--strip-all',
        '-Wl,--exclude-libs,ALL',
        '-Wl,--no-as-needed',
    ]
else
    rel_compile_args += [
        '/O1',
        '/Gy',
        '/Gw',
        '/GR-',
    ]

    rel_link_args += [
        '/OPT:REF',
        '/OPT:ICF',
    ]
endif

compile_args += ['-D_POSIX_C_SOURCE=200809L', '-D_XOPEN_SOURCE=700']

if get_option('debug')
    add_project_arguments(['-DDEBUG'], language: 'c')

    if cc.get_id() != 'msvc'
        compile_args += [
            '-fno-omit-frame-pointer',
            '-fasynchronous-unwind-tables',
            '-fno-optimize-sibling-calls',
            '-fno-lto',
            '-Wno-frame-address',
        ]
        link_args += ['-Wl,--export-dynamic']
    else
        compile_args += [
            '/Od',
            '/Oy-',
            '/Zi',
        ]
        link_args += [
            '/DEBUG',
        ]
    endif
else
    compile_args += rel_compile_args
    link_args += rel_link_args
endif

if cpp.get_id() != 'msvc'
    add_project_arguments(
        [
            '-fvisibility=hidden',
            '-fvisibility-inlines-hidden',
        ],
        language: 'cpp',
    )
endif

if get_option('sanitizers')
    if cc.get_id() != 'msvc'
        compile_args += [
            '-fsanitize=address,undefined',
            '-fno-omit-frame-pointer',
        ]

        link_args += [
            '-fsanitize=address,undefined',
        ]
    else
        warning('sanitizers not supported under msvc')
    endif
endif

add_project_arguments(compile_args, language: ['c', 'cpp'])
add_project_link_arguments(link_args, language: ['c', 'cpp'])

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

sources += [vcs, conf_file]

m_dep = cc.find_library('m', required: false)

cmake = import('cmake')
cmake_args = cmake.subproject_options()

cmake_args.append_compile_args('cpp', rel_compile_args)
cmake_args.append_link_args([rel_link_args])
cmake_args.add_cmake_defines(
    {
        'SPIRV_CROSS_ENABLE_TESTS': false,
        'SPIRV_CROSS_CLI': false,
        'ENABLE_GLSLANG_BINARIES': false,
    },
)

# tools
sh2spv_deps = []

if dependency('glslang', required: false).found()
    sh2spv_deps += dependency('glslang')
    rel_compile_args += '-DSPEL_SYSTEM_GLSLANG'
else
    glslang_args = cmake_args
    glslang_args.set_override_option('default_library', 'shared')
    sh2spv_deps += cmake.subproject('glslang', options: cmake_args).dependency('glslang')
endif

sh2spv = executable(
    'spel-sh2spv',
    sources: ['tools/sh2spv.cpp'],
    dependencies: sh2spv_deps,
    cpp_args: rel_compile_args,
    link_args: rel_link_args,
)

spv2h = executable(
    'spel-spv2h',
    sources: ['tools/spv2h.cpp'],
    dependencies: dependency('nlohmann_json'),
    cpp_args: rel_compile_args,
    link_args: rel_link_args,
)

compile_shaders = custom_target(
    'compile_shaders',
    input: shaders,
    output: 'shaders.json',
    command: [sh2spv, '@INPUT@'],
    console: false,
)

shader_header = custom_target(
    input: compile_shaders,
    command: [spv2h, '@INPUT@', '@OUTPUT@'],
    output: 'gfx_internal_shaders.h',
)

# Spël
dependencies = [
    dependency('sdl3'),
    m_dep,
    cmake.subproject('SPIRV-Cross', options: cmake_args).dependency('spirv-cross-core'),
]

spel_lib = library(
    'spel',
    sources: [sources, shader_header],
    include_directories: include_directories('spel'),
    dependencies: dependencies,
)

spel_dep = declare_dependency(
    link_with: spel_lib,
    dependencies: dependencies,
    include_directories: include_directories('spel'),
)

sandbox_src = ['sandbox/main.c']
executable(
    'sandbox',
    sources: sandbox_src,
    dependencies: spel_dep,
    link_args: link_args,
)
