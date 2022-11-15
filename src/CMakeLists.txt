find_package(fmt CONFIG REQUIRED)
find_package(docopt CONFIG REQUIRED)
find_package(libxlsxwriter CONFIG REQUIRED)

add_subdirectory(pdf)
add_subdirectory(parser)
add_subdirectory(processor)

set(SRCS
    CSVExporter.h
    ExcelExporter.h
    ExcelExporter1.h
    PdfToExcel.h
    ScrapePart1.h
    CSVExporter.cpp
    ExcelExporter.cpp
    ExcelExporter1.cpp
    main.cpp
    PdfToExcel.cpp
    ScrapePart1.cpp
    CustomState2.cpp
    CustomState2.h
    CustomTextState.cpp
    CustomTextState.h
    DumpText.cpp
    DumpText.h
    DumpContents.cpp
    DumpContents.h
    DumpUnzippedPdf.h
    DumpUnzippedPdf.cpp
)

add_executable(pdf_reader ${SRCS})

target_precompile_headers(pdf_reader PRIVATE <vector> <string> <map> <algorithm> <xlsxwriter.h>)

target_link_libraries(
  pdf_reader
  PRIVATE project_options
          project_warnings
          fmt::fmt
          docopt::docopt
          pdflib
          libxlsxwriter::libxlsxwriter
          processorlib
)
