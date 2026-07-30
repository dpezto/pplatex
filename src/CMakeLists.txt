
add_executable(pplatex main.cpp regex.cpp outputinfo.cpp
	               outputfilter.cpp
	               latexoutputfilter.cpp)

target_include_directories(pplatex PRIVATE ${PCRE2_INCLUDE_DIRS})
target_link_libraries(pplatex PRIVATE ${PCRE2_LINK_LIBRARIES})

# pcre2posix.h declares plain externs by default, which is what static linking
# needs. Importing from a DLL on Windows requires opting in explicitly.
option(PCRE2_SHARED "Link against a PCRE2 DLL instead of a static library" OFF)
if (WIN32 AND PCRE2_SHARED)
  target_compile_definitions(pplatex PRIVATE PCRE2POSIX_SHARED)
endif()
