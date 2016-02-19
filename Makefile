
BIN_TARGET = font-tool
SRC_FILES  = font_tool.cpp utils.cpp fnt.cpp compressor.cpp data_writer.cpp
CXXFLAGS   = -std=c++14 -O3 -Wall -Wextra -Weffc++ -Wshadow -pedantic

all:
	$(CXX) $(CXXFLAGS) $(SRC_FILES) -o $(BIN_TARGET)

clean:
	rm -f *.o
	rm -f $(BIN_TARGET)

