project('burglar_fucker', 'cpp',  default_options : ['cpp_std=c++2a', 'warning_level=3'])

static_linking = get_option('static_linking')
opencv = dependency('opencv4',  modules : ['core', 'videoio', 'imgproc', 'imgcodecs'], static: static_linking)
sfml = dependency('sfml-audio', static: static_linking)

boost_proj = subproject('boost')
boost = boost_proj.get_variable('boost_dep').as_system('system')

spdlog_proj = subproject('spdlog')
spdlog = spdlog_proj.get_variable('spdlog_dep').as_system('system')

tomlpp_proj = subproject('tomlpp')
tomlpp = tomlpp_proj.get_variable('tomlplusplus_dep').as_system('system')

executable('detection',
           [
               'src/configuration_loader.cpp',
               'src/http/server.cpp',
               'src/main.cpp',
               'src/movement_detector.cpp',
               'src/movement_handler.cpp'
           ],
           dependencies : [
                              opencv,
                              tomlpp,
                              spdlog,
                              sfml,
                              boost
                          ]
          )
