INCLUDE = -I. -I.. -I../.. -I../../..
LIBRARY =

CXXFLAGS    = -Wall -ggdb -pthread

LINKFLAGS   = -Wl,-rpath,./:../bin
LINKFLAGS_MT = $(LINKFLAGS)

CXXFLAGS_EXTERN =
LINKFLAGS_EXTERN = -L../.. -levwork -lev -ljsoncpp -lboost_thread-mt

TARGET  = jsonsrv

SRC_FILES   = $(wildcard *.cpp)

OBJ_FILES   = $(SRC_FILES:.cpp=.o)

$(TARGET) : $(OBJ_FILES)
	g++ -o $@ $(OBJ_FILES) $(CXXFLAGS) $(CXXFLAGS_EXTERN) $(LINKFLAGS_EXTERN)

%.o : %.cpp
	g++ -c -o $@ $< $(CXXFLAGS) $(CXXFLAGS_EXTERN) $(INCLUDE)
%.ro : %.cpp
	g++ -c -o $@ $< $(CXXFLAGS_R) $(CXXFLAGS_EXTERN) $(INCLUDE)

.PHONY : all clean

all : $(TARGET)

clean :
	-rm $(OBJ_FILES)
	-rm $(TARGET)
