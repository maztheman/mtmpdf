find_package(ZLIB CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_library(pdflib STATIC pdf.cpp pdf.h)

target_precompile_headers(pdflib PUBLIC <zlib.h> <map> <set> <utility> <functional> <chrono> <vector> <string> <deque> <fmt/format.h> <cstdint>)

target_link_libraries(pdflib PUBLIC ZLIB::ZLIB project_options fmt::fmt)
target_include_directories(pdflib PUBLIC ..)

add_subdirectory(utils)
add_subdirectory(types)
add_subdirectory(objects)
