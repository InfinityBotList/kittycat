cmake_minimum_required(VERSION 3.5)
project(kittycat VERSION 0.1.0 DESCRIPTION "Kittycat permission handling" LANGUAGES C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -Wno-unused-parameter -O3") # No ASAN
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -Werror -Wno-unused-parameter -O3 -g") # No ASAN+DEBUG
set(CMAKE_C_FLAGS_ASAN "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -Werror -Wno-unused-parameter -O3 -fsanitize=address -fsanitize=undefined -g") # ASAN
set(CMAKE_C_FLAGS_C99 "${CMAKE_C_FLAGS_C99} -Wall -Wextra -Werror -Wno-unused-parameter -O3 -DC99 -std=c99") # No ASAN+C99
set(CMAKE_C_FLAGS_C99_DEBUG "${CMAKE_C_FLAGS_C99_DEBUG} -Wall -Wextra -Werror -Wno-unused-parameter -O3 -DC99 -std=c99 -g") # No ASAN+C99+DEBUG
set(CMAKE_C_FLAGS_C99_ASAN "${CMAKE_C_FLAGS_C99_DEBUG} -Wall -Wextra -Werror -Wno-unused-parameter -O3 -DC99 -std=c99 -fsanitize=address -fsanitize=undefined -g") # ASAN+C99

# Actual library
add_library(kittycat 
    src/lib/hashmap.c
    src/lib/kc_string.c
    src/lib/perms.c
    src/lib/alloc.c
)

# Shared lib config
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
set_target_properties(kittycat PROPERTIES POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS})

# Configure project variables
set_target_properties(kittycat PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(kittycat PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
set(CMAKE_MODULE_PATH, ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
set_target_properties(kittycat PROPERTIES PUBLIC_HEADER "src/lib/alloc.h;src/lib/kc_string.h;src/lib/perms.h;src/lib/hashmap.h")
include(GNUInstallDirs)
install(TARGETS kittycat
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION kittycat/${CMAKE_INSTALL_INCLUDEDIR})
configure_file(src/lib/kittycat.pc.in src/lib/kittycat.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/src/lib/kittycat.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

# Tests
enable_testing()
set(CMAKE_CTEST_ARGUMENTS "--verbose")
add_executable(kc_string_test
    src/tests/kc_string_test.c
)
target_link_libraries(kc_string_test kittycat)
add_test(NAME kc_string_test COMMAND kc_string_test)

add_executable(perms_test
    src/tests/perms_test.c
)

target_link_libraries(perms_test kittycat)
add_test(NAME perms_test COMMAND perms_test)
