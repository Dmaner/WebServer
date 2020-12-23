CC = g++
CPPFLAGES = -Wall
target = server
files = $(wildcard *.cpp) $(wildcard ./net/http/*.cpp)
objs :=  $(patsubst %.cpp,%.o,$(files))

$(target): $(objs)
	$(CC) $(objs) $(CPPFLAGES) -o $(target)

%.o: %.cpp
	$(CC) -c $< -o $@

.PHONY: clean

clean: 
	rm -rf $(objs)