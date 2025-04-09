
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

dep_ft2 = dependency('freetype2', required: true)
dep_rphiic = dependency('rphiic', required: true)
dep_glfw3 = dependency('glfw3', required: true)

civ = executable('civ',
  sources,
  dependencies: [dep_ft2, dep_rphiic, dep_glfw3],
  link_args: '-lm',
  install: true,
  )

