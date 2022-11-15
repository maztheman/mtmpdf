set(SRCS
	ContentParser.h
	ContentParser.cpp
	Helpers.h
	Helpers.cpp
	Command.h
	Command.cpp
	CommandArgs.cpp
	CommandArgs.h
	CommandState.h
	CommandState.cpp
)

target_sources(parserlib PRIVATE ${SRCS})