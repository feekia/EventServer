aux_source_directory(handler DIR_SRCS)
aux_source_directory(network DIR_SRCS)
aux_source_directory(codec DIR_SRCS)
aux_source_directory(utils DIR_SRCS)


add_library (es ${DIR_SRCS})

