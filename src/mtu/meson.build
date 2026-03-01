mtu_sources = [
  'mtu_bootstrap.cpp',
]

foreach s : mtu_sources
  if not fs.exists(s)
    error('MTU: Missing source file ' + s)
  endif
endforeach

mtu_src = files(mtu_sources)

mtu_lib = static_library('mtu',
  mtu_src,
  include_directories : [ dxvk_include_path ],
  dependencies        : [ dxvk_dep ],
)

mtu_dep = declare_dependency(
  link_with : [ mtu_lib ],
)