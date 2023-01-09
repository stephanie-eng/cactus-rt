add_executable(rt_logging_example
  main.cc
)

target_link_libraries(rt_logging_example
  PRIVATE
  cactus_rt
)

if (ENABLE_CLANG_TIDY)
  set_target_properties(rt_logging_example PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY_PATH})
endif()

install(
  TARGETS rt_logging_example
  DESTINATION bin
)
