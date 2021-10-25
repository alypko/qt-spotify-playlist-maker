# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\qt-spotify-playlist-maker_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\qt-spotify-playlist-maker_autogen.dir\\ParseCache.txt"
  "qt-spotify-playlist-maker_autogen"
  )
endif()
