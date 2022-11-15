add_library(parserlib STATIC "")

add_subdirectory(pdf)

target_precompile_headers(parserlib PRIVATE <string> <vector> <map> <unordered_map> <algorithm> <string_view> <deque> <fmt/format.h>)
target_link_libraries(parserlib PRIVATE project_options fmt::fmt)
target_include_directories(parserlib PUBLIC ..)


