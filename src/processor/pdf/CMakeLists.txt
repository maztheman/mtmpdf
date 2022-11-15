add_subdirectory(types)

set(SRCS 
	Processor.h
	Processor.cpp
)

target_sources(processorlib PRIVATE ${SRCS})

