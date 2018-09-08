## [M1: point 1]
#  we are manually assigning the value for the constant CUR_DIR which will be used throughout in
#  this Makefile (there is CURDIR which can also be used)
#
#  $ is used in this case to denote returning a variable value which is whatever inside of this $()
#  
#  "shell" can be also understood as simply command option where it takes an argument as a shell
#  command and evals to the output of that input, so in this case we can relate it as how the
#  "dirname" function is being called outside of this makefile in the terminal.
#
#  similar to "dirname", "realpath" is also a function which can be ran under terminal to returns
#  the pointer to the resolved name. If error, then it will return NULL pointer.
#
#  Given this, there are few special variables which needs to be explained which are MAKEFILE_LIST
#  and lastword: 
#  > MAKEFILE_LIST = this special variable contains the name of each makefile that is parsed by
#  make in which the order it was parsed. 
#  > lastword = last word which was typed before this is getting called. 
#  ...
CUR_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

## [M2: point 1]
#  CC / CFLGS / LDFLAGS are special constants that are used to denote special flags to this
#  Makefile. in this case, we instruct whichever files are being made, we want to use GCC compiler
#  to use, while adding few compilation commands that are passing on to the compiler
#  -g = sub-processes -O2 = optimization option 2 -Werror = every warning treated as an error
#  -Wall = enables a set of warning, but not all.
#  
#  ...
CC = gcc
CFLAGS += -g -O2 -Werror -Wall
LDFLAGS +=

## [M4: point 1]
#  DEPS in this case means is a macro for dependencies (inferred by the file *.d)
#  which is known as the dependencies file. So this DEPS_DIR would be set to contain the
#  filepath of (Current directory)/.deps(library suffix)
#  several dependencies flags are set to control the preprocessors: -MD = similar to -MF file
#  except that -E is not applied (-E limits the handling of few directives)
#  -MF = specifies a file to write the dependecies to (in this case $(DEPS_DIR)/.d files)
#  -MP = instructing compiling file to add a phony target for each dependencies EXCEPT the main
#  
#  ...
DEPS_DIR  := $(CUR_DIR)/.deps$(LIB_SUFFIX)
DEPCFLAGS = -MD -MF $(DEPS_DIR)/$*.d -MP

## [M5: point 1]
#  this is creating a SRC_FILES macro where the wildcard function within the context of Makefile
#  is to list all of the source files with a particular extension. In this case would be denoted
#  as *.c or all c files as sources (hence, SRC).
#  ...
SRC_FILES = $(wildcard *.c)

## [M6: point 1]
#  this is creating a EXE_FILES macro. : used in this case will set the macro EXEFILES with .c
#  files if it is unset. 
#  ...
EXE_FILES = $(SRC_FILES:.c=)

## [M7: point 1]
#  This is also known as the "dummy target" which states that if inovked, make will build ALL what
#  is needed in order to completely build what is needed to be built. 
#  ...
all: $(EXE_FILES)
	echo $(EXE_FILES)

## [M8: point 1]
#  Character % also denotes "wildcard". therefore what it means by % / % is *any directories*
#  along with %.c means any c source file names. Then $ denotes escaping characters of @ and <
#  where -c option is simply compile option.
#  ...
%/%.c:%.c $(DEPS_DIR)
	$(CC) $(CFLAGS) $(DEPCFLAGS) -c $@ $<

## [M9: point 1]
#  clean in this case is the sub-function that can be invoked along with the "make" command (such
#  as make clean). Then it will simply remove using rm -f (-f in this case is force removal) all of
#  the files listed in the $(EXE_FILES)
#  ...
clean:
	rm -f $(EXE_FILES)

## [M10: point 1]
#  .PHONY is also known as the Phony Target. This is used to enforce two main problems that can
#  occur: 1) avoid potential conflict with a same file name (all clean will run regardless
#  if same name is dtected) and 2) for performance reason (GNU Make skips implicit rule search
#  for phony targets, hence saves time)
#  ...
.PHONY: all clean
