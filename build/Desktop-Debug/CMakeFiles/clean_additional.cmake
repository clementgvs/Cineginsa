# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/Cineginsa_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/Cineginsa_autogen.dir/ParseCache.txt"
  "Cineginsa_autogen"
  )
endif()
