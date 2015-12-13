
BIN_TARGET = font-tool
SRC_FILE   = font_tool.cpp
CXXFLAGS   = -std=c++14 -O3 -Wall -Wextra -Weffc++ -Wshadow

all:
	$(CXX) $(CXXFLAGS) $(SRC_FILE) -o $(BIN_TARGET)

clean:
	rm -f *.o
	rm -f $(BIN_TARGET)

