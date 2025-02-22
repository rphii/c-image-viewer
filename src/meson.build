
#subdir('shaders')

#run_target('shaders_generated',
#  command: ['shaders/generate.sh'])

sources = [
  'arg.c',
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
dep_c_str = dependency('c-str', required: true)
dep_c_file = dependency('c-file', required: true)
dep_c_utf8 = dependency('c-utf8', required: true)
dep_glfw3 = dependency('glfw3', required: true)

civ = executable('civ',
  sources,
  dependencies: [dep_ft2, dep_c_str, dep_c_file, dep_c_utf8, dep_glfw3],
  link_args: '-lm',
  )

