sources_shaders = [
  'box.frag',
  'box.vert',
  'rectangle.frag',
  'rectangle.vert',
  'text.frag',
  'text.vert',
  ]

target_shaders = custom_target('shaders',
  output: 'shaders.o',
  input: sources_shaders,
  command: ['ld', '-r', '-b', 'binary', '-o', '@OUTPUT@', '@INPUT@'],
  )


