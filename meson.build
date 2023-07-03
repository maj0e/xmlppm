project('xmlppm', ['c', 'cpp'], default_options: [ 'b_ndebug=if-release' ])

# Content of config.h.in converted via meson's ac_converter.py
cc = meson.get_compiler('c')
cdata = configuration_data()
check_headers = [
  'fcntl.h',
  'inttypes.h',
  'memory.h',
  'stdint.h',
  'stdlib.h',
  'strings.h',
  'string.h',
  'sys/stat.h',
  'sys/types.h',
  'unistd.h',
]

foreach h : check_headers
  if cc.has_header(h)
    cdata.set('HAVE_' + h.underscorify().to_upper(), 1)
  endif
endforeach

check_functions = [
# check token ['HAVE_LIBC']
# check token ['HAVE_LIBEXPAT']
# check token ['HAVE_LIBICONV']
  ['HAVE_MALLOC', 'malloc', '#include<stdlib.h>'],
  ['HAVE_MEMMOVE', 'memmove', '#include<string.h>'],
  ['HAVE_MEMSET', 'memset', '#include<string.h>'],
  ['HAVE_REALLOC', 'realloc', '#include<stdlib.h>'],
  ['HAVE_STRDUP', 'strdup', '#include<string.h>'],
  ['HAVE_STRERROR', 'strerror', '#include<string.h>'],
  ['HAVE_STRRCHR', 'strrchr', '#include<string.h>'],
]

foreach f : check_functions
  if cc.has_function(f.get(1), prefix : f.get(2))
    cdata.set(f.get(0), 1)
  endif
endforeach

configure_file(input : 'config.h.meson',
  output : 'config.h',
  configuration : cdata)


expat_prj = subproject('expat')
expat_dep = expat_prj.get_variable('expat_dep')

iconv_dep = dependency('iconv')



#if cc.get_define('_O_BINARY') == ''
  add_project_arguments([ '-D_O_BINARY=0' ], language: 'cpp')
#endif


# --
# -- libxmlppm
# --
incdir = include_directories('src')
xmlppm_lib = static_library(
	'tuskdat',
	[ 
		'src/Coder.cpp',
		'src/DecodeSax.cpp',
		'src/EncodeSax.cpp',
		'src/IFile.cpp',
		'src/Model.cpp',
		'src/PrintSax.cpp',
		'src/StringArray.cpp',
		'src/XmlModel.cpp',
		'src/Args.cpp'
	],
	include_directories : incdir,
	dependencies : [ expat_dep, iconv_dep ]
)

xmlppm_dep = declare_dependency(
	version             : '0.98.3',
	include_directories : incdir,
	link_with           : xmlppm_lib,
	dependencies        : [ expat_dep ]
)

# --
# -- xmlppm/xmlunppm executable
# --
xmlppm = executable(
	'xmlppm', 
	[ 'src/xmlppm.cpp' ],
	dependencies : [ xmlppm_dep ]
)

xmlunppm = executable(
	'xmlunppm', 
	[ 'src/xmlunppm.cpp' ],
	dependencies : [ xmlppm_dep ]
)

