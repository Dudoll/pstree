add_executable(pstree pstree.c variable_array.c)

target_include_directories(pstree PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)