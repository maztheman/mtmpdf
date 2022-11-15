set(SRCS
	tools.cpp 
	tools.h
	ToUnicode.cpp
	ToUnicode.h
	TrueTypeFont.cpp
	TrueTypeFont.h
	zlib_helper.cpp
	zlib_helper.h
)

target_sources(pdflib PRIVATE ${SRCS})