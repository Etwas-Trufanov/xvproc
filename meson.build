project('xvproc', 'cpp',
  default_options: ['cpp_std=c++17'])

# Указываем, что исходные файлы находятся в папке src
src_files = files('source/main.cpp')

# Создаём исполняемый файл
exe = executable('xvprocexe',
                 src_files,
                 install: false)  # Не устанавливаем в систему
