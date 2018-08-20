INCLUDE = -I. -I./libevwork 
LIBRARY =

CXXFLAGS    = -Wall -ggdb  -O0 -pthread -D__DEBUG__

LINKFLAGS   = -Wl,-rpath,./:../bin 
LINKFLAGS_MT = $(LINKFLAGS)

CXXFLAGS_EXTERN =
LINKFLAGS_EXTERN = -L./libev -lev -ljsoncpp  -L./libevwork -levwork -L./libtinyredis -ltinyredis -lhiredis 

TARGET   = robot

SRC_FILES   = $(wildcard *.cpp \
			  Cardlib/*.cpp)

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
	rm -f $(OBJ_FILES)
	rm -f $(TARGET)
