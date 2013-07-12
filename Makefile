CXX = g++
CXXFLAGS += -Wall -Wextra -Wno-return-type-c-linkage -std=c++11 -fPIC
LDFLAGS += -shared
AR = ar
ARFLAGS = rcs

INCLUDES = -I. -I./v8pp
LIBS = -L. -lv8pp -lv8 -lboost_system -lboost_program_options -lboost_filesystem -ldl

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

v8pp_test: $(patsubst %.cpp, %.o, test/main.cpp) lib plugins
	$(CXX) $< -o $@ $(LIBS)

lib: $(patsubst %.cpp, %.o, $(wildcard v8pp/*.cpp))
	$(AR) $(ARFLAGS) libv8pp.a $^

plugins: console file

console: $(patsubst %.cpp, %.o, plugins/console.cpp)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@.so

file: $(patsubst %.cpp, %.o, plugins/file.cpp)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@.so

clean:
	rm -rf v8pp/*.o test/*.o plugins/*.o libv8pp.a v8pp_test *.so

