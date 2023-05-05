.PHONY: all clean build run

CC = g++
CFLAGS = -lGL -lglfw -lGLEW

build_dir = build
source_dir = src
include_dir = include

executable_name = main

all: clean build run

# deletes all the files in the build that does not have a dot(.) in its filename
clean:
	ls $(build_dir) | grep -Pv "\." | sed -E 's/(.*)/$(build_dir)\/\1/' | xargs -d"\n" rm -f

build: 
	mkdir -p build; $(CC) $(source_dir)/*.cpp -o $(build_dir)/$(executable_name) $(CFLAGS) -I $(include_dir)

run:
	$(build_dir)/$(executable_name)