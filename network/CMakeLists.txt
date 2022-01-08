aux_source_directory(. CURRENT_DIR_SRCS)

include_directories(${PROJECT_SOURCE_DIR}/utils)

add_library (network ${CURRENT_DIR_SRCS})
