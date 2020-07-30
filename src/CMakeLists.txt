#
#  Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

set(FLUID_BIN fluid)
set(FLUID_BIN_SRC
	lexer.c
	fluid.c
)

add_executable(${FLUID_BIN} ${FLUID_BIN_SRC})
target_include_directories(${FLUID_BIN} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include/
	${CMAKE_SOURCE_DIR}/utils/include/
)
target_link_libraries(${FLUID_BIN} utils)
