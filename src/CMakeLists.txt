#
#  Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

set(FLUID_BIN fluid)
set(FLUID_BIN_SRC
	fluid.c     fluid.h
	lexer.c     lexer.h
	liquid.c    liquid.h
	parser.c    parser.h
)

add_executable(${FLUID_BIN} ${FLUID_BIN_SRC})
target_include_directories(${FLUID_BIN} PRIVATE
	${CMAKE_SOURCE_DIR}/utils/include/
)
target_link_libraries(${FLUID_BIN} utils)
