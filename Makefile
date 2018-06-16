CXX ?= c++
CXXFLAGS += -Wall -Wextra -std=c++11 -fPIC -fno-rtti -DV8PP_ISOLATE_DATA_SLOT=0
LDFLAGS += -shared
AR = ar
ARFLAGS = rcs

INCLUDES = -I. -I./v8pp -isystem./v8/include -isystem./v8 -isystem/usr -isystem/usr/lib -isystem/opt/libv8-${V8_VERSION}/include
LIBS = -L./v8/lib -L/opt/libv8-${V8_VERSION}/lib -Wl,-rpath,/opt/libv8-${V8_VERSION}/lib -lv8 -lv8_libplatform -lv8_libbase -licui18n -licuuc -L. -Wl,-whole-archive -lv8pp -Wl,-no-whole-archive -ldl -lpthread

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

all: lib plugins v8pp_test

v8pp_test: $(patsubst %.cpp, %.o, $(wildcard test/*.cpp))
	$(CXX) $^ -o $@ $(LIBS)

lib: $(patsubst %.cpp, %.o, $(wildcard v8pp/*.cpp))
	$(AR) $(ARFLAGS) libv8pp.a $^

plugins: console file

console: $(patsubst %.cpp, %.o, plugins/console.cpp)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(LIBS) $^ -o $@.so

file: $(patsubst %.cpp, %.o, plugins/file.cpp)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(LIBS) $^ -o $@.so

clean:
	rm -rf v8pp/*.o test/*.o plugins/*.o libv8pp.a v8pp_test console.so file.so

