CXX ?= c++
CXXFLAGS += -Wall -Wextra -std=c++11 -fPIC
LDFLAGS += -shared
AR = ar
ARFLAGS = rcs

INCLUDES = -I. -I./v8pp -isystem./v8/include -isystem./v8
LIBS = -L./v8/lib -lv8 -lv8_libplatform -licui18n  -licuuc -L. -lv8pp -ldl -lpthread

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

all: lib plugins v8pp_test

v8pp_test: $(patsubst %.cpp, %.o, $(wildcard test/*.cpp))
	$(CXX) $^ -o $@ $(LIBS)

lib: $(patsubst %.cpp, %.o, $(wildcard v8pp/*.cpp))
	$(AR) $(ARFLAGS) libv8pp.a $^

plugins: console file

console: $(patsubst %.cpp, %.o, plugins/console.cpp)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@.so

file: $(patsubst %.cpp, %.o, plugins/file.cpp)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@.so

clean:
	rm -rf v8pp/*.o test/*.o plugins/*.o libv8pp.a v8pp_test console.so file.so

