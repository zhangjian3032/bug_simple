project('bug-sample', 'cpp',
    default_options: [
      'buildtype=debugoptimized',
      'cpp_std=c++20',
      'warning_level=3',
      'werror=true',
    ],
    version: '1.0.0',
    meson_version: '>=0.57.0',
)

sdbusplus_dep = dependency('sdbusplus')
phosphor_logging_dep = dependency('phosphor-logging')

subdir('case1')
subdir('match')
