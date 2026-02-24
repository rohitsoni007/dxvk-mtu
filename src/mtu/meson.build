mtu_sources = [
  'mtu_overlay.cpp',
  'imgui/imgui.cpp',
  'imgui/imgui_draw.cpp',
  'imgui/imgui_tables.cpp',
  'imgui/imgui_widgets.cpp',
  'imgui/backends/imgui_impl_win32.cpp',
  'imgui/backends/imgui_impl_vulkan.cpp',
]

foreach s : mtu_sources
  if not fs.exists(s)
    error('MTU: Missing source file ' + s + '. Did you forget to initialize submodules?')
  endif
endforeach

mtu_src = files(mtu_sources)

mtu_lib = static_library('mtu', [ mtu_src ],
  include_directories : [ dxvk_include_path, 'imgui' ],
  dependencies        : [ dxvk_dep ],
)

mtu_dep = declare_dependency(
  link_with           : [ mtu_lib ],
  include_directories : [ '.', 'imgui' ],
)
