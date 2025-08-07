
sources = [
  'civ.c',
  'civ_config.c',
  'gl_box.c',
  'gl_image.c',
  'gl_shader.c',
  'gl_text.c',
  'gl_uniform.c',
  'glad.c',
  'main.c',
  'timer.c',
  ]

cc = meson.get_compiler('c')

m_dep = cc.find_library('m', required: false)

rlc_dep = dependency('rlc', fallback : ['rlc', 'rlc_dep'], default_options: ['default_library=static'])
rlso_dep = dependency('rlso', fallback : ['rlso', 'rlso_dep'], default_options: ['default_library=static'])
rlarg_dep = dependency('rlarg', fallback : ['rlarg', 'rlarg_dep'], default_options: ['default_library=static'])

ft2_dep = dependency('freetype2', required: true)
glfw3_dep = dependency('glfw3', required: true)

civ = executable('civ',
  sources,
  dependencies: [ft2_dep, rlc_dep, rlso_dep, rlarg_dep, glfw3_dep, m_dep],
  install: true,
  )

