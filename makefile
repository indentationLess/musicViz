CXX = g++
CXXFLAGS = -Wall -std=c++11
INCLUDES = -Iincludes
LIBS = -lportaudio -lsndfile -lfftw3
TARGET = audio_visualizer
SRCS = $(wildcard src/*.cpp) $(wildcard src/*.hpp)
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(TARGET) $(OBJS) $(LIBS)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
