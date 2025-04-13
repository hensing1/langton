LIB_FLAGS := -lncursesw
DEBUG_FLAGS := -g -fsanitize=address -Wall
SOURCE_FILES := $(wildcard *.c)

ant: $(SOURCE_FILES)
	clang $(SOURCE_FILES) -o ant $(LIB_FLAGS)

debug: $(SOURCE_FILES)
	clang $(SOURCE_FILES) -o ant $(LIB_FLAGS) $(DEBUG_FLAGS)

clean:
	rm -f ./ant
