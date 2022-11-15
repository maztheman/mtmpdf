find_package(fmt CONFIG REQUIRED)

add_library(processorlib STATIC "")

target_precompile_headers(processorlib PRIVATE <vector> <string> <map> <unordered_map> <algorithm> <functional> <deque> <array>)
target_link_libraries(processorlib PRIVATE project_options fmt::fmt pdflib parserlib)
target_include_directories(processorlib PUBLIC ..)

add_subdirectory(types)
add_subdirectory(pdf)

