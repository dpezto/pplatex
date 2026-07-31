
add_executable(pplatex main.cpp regex.cpp outputinfo.cpp
	               latexoutputfilter.cpp style.cpp render.cpp knowledge.cpp)

# main.cpp stringifies these for --version
target_compile_definitions(pplatex PRIVATE
	RELEASE_NAME=${PROJECT_NAME}
	RELEASE_VERSION=${PPLATEX_VERSION})

target_include_directories(pplatex PRIVATE ${PCRE2_INCLUDE_DIRS})
target_link_libraries(pplatex PRIVATE ${PCRE2_LINK_LIBRARIES})

# pcre2posix.h declares plain externs by default, which is what static linking
# needs. Importing from a DLL on Windows requires opting in explicitly.
option(PCRE2_SHARED "Link against a PCRE2 DLL instead of a static library" OFF)
if (WIN32 AND PCRE2_SHARED)
  target_compile_definitions(pplatex PRIVATE PCRE2POSIX_SHARED)
endif()

install(TARGETS pplatex RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# pplatex picks the LaTeX engine from the name it was invoked as, so the other
# two entry points are the same binary under a different name. Symlinks are
# enough: argv[0] reports the link name, not the resolved target.
foreach (alias ppdflatex ppluatex)
  if (WIN32)
    # Windows symlinks need privileges that an install step should not assume
    install(PROGRAMS $<TARGET_FILE:pplatex> DESTINATION ${CMAKE_INSTALL_BINDIR}
            RENAME ${alias}${CMAKE_EXECUTABLE_SUFFIX})
  else()
    # \${CMAKE_INSTALL_PREFIX} is escaped so it expands at install time, not
    # configure time: `cmake --install --prefix X` overrides the prefix after
    # configuration, and a configure-time path would send the symlinks to the
    # old prefix while the binary honors the new one.
    install(CODE "
      set(_link \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${alias}\")
      file(REMOVE \"\${_link}\")
      file(CREATE_LINK pplatex \"\${_link}\" SYMBOLIC)
      message(STATUS \"Created symlink: \${_link} -> pplatex\")
    ")
  endif()
endforeach()
