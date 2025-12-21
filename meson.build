project('civ', 'c', meson_version: '>=1.1', version: '9999')
add_project_arguments('-DVERSION="'+meson.project_version()+'"', language : 'c')
subdir('src')

install_data('desktop/civ.desktop', install_dir: get_option('desktop_dir'))
install_data('bash/civ', install_dir: get_option('completion_dir'))

