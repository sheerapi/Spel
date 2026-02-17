project(
    'imgui',
    ['c', 'cpp'],
    version: '1.92.6',
    default_options: [
        'c_std=c11',
        'cpp_std=c++17',
        'default_library=static',
        'b_lto=true',
        'b_staticpic=true',
    ],
    meson_version: '>= 1.10.0',
)

sources = [
    'src/dcimgui.cpp',
    'src/dcimgui_internal.cpp',
    'src/imgui_draw.cpp',
    'src/imgui_tables.cpp',
    'src/imgui_widgets.cpp',
    'src/imgui_demo.cpp',
    'src/imgui.cpp',
]

imgui_lib = library(
    'imgui',
    sources: sources,
    include_directories: include_directories('imgui'),
)

imgui_dep = declare_dependency(
    link_with: imgui_lib,
    include_directories: include_directories('imgui'),
)

meson.override_dependency('imgui', imgui_dep)