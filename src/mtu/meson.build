mtu_src = files([
  'mtu_overlay.cpp',
  'imgui/imgui.cpp',
  'imgui/imgui_draw.cpp',
  'imgui/imgui_tables.cpp',
  'imgui/imgui_widgets.cpp',
  'imgui/backends/imgui_impl_win32.cpp',
  'imgui/backends/imgui_impl_vulkan.cpp',
])

mtu_lib = static_library('mtu', [ mtu_src ],
  include_directories : [ dxvk_include_path, 'imgui' ],
  dependencies        : [ dxvk_dep ],
)

mtu_dep = declare_dependency(
  link_with           : [ mtu_lib ],
  include_directories : [ '.', 'imgui' ],
)
