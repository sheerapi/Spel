project('spel', 'c', version: '0.0.1', default_options: ['c_std=c2x'])

if get_option('debug')
    add_project_arguments(['-DDEBUG'], language: 'c')
endif

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

sources = [
    'src/core/entry.c', 
    'src/core/log.c', 
    'src/core/window.c', 
    'src/core/event.c', 
    'src/utils/time.c', 
    'src/gfx/gfx.c', 
    'src/gfx/backends/gl/gfx_context_gl.c',
    'src/gfx/backends/gl/gfx_context_cmdlist_gl.c',
    'src/gfx/backends/gl/gfx_buffer_gl.c']
dependencies = [dependency('sdl3'), m_dep]

spel_lib = library('spel', sources: sources, include_directories: include_directories('spel'), dependencies: dependencies)

spel_dep = declare_dependency(link_with: spel_lib, dependencies: dependencies, include_directories: include_directories('spel'))

sandbox_src = ['sandbox/main.c']
executable('sandbox', sources: sandbox_src, dependencies: spel_dep)