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
    filter.c    filter.h
    objects.c   objects.h
    config.c    config.h
    ferrors.c    ferrors.h
)

add_executable(${FLUID_BIN} ${FLUID_BIN_SRC})

target_include_directories(${FLUID_BIN} PRIVATE
    ${LIBUTILS_INCLUDE_DIR}
    ${LIBYAML_INCLUDE_DIR}
)

target_compile_definitions(${FLUID_BIN} PUBLIC
    -DVERSION="${PROJECT_VERSION}"
    -DPROG_NAME="${PROJECT_NAME}"
)

target_link_libraries(${FLUID_BIN}
    ${LIBUTILS_LIBRARIES}
    ${LIBYAML_LIBRARIES}
)
