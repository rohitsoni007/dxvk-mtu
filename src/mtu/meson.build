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

mtu_dep = [ dxvk_dep ]

if host_machine.system() == 'windows'
  lib_dwmapi = cpp.find_library('dwmapi', required: true)
  mtu_dep += lib_dwmapi
endif

mtu_lib = static_library('mtu', [ mtu_src ],
  include_directories : [ dxvk_include_path, 'imgui' ],
  dependencies        : mtu_dep,
  cpp_args            : [ '-DIMGUI_IMPL_VULKAN_NO_PROTOTYPES' ],
)