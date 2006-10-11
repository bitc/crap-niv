# C++ compiler is traditionally called CXX
CXX = gcc

# The traditional name for the flags to pass to the preprocessor.
# Useful if you need to specify additional directories to look for
# header files.
CPPFLAGS = `curl-config --cflags`

# Another traditional name for the C++ compiler flags
CXXFLAGS = -O2 -Wall -Wextra -pedantic -ansi -Wno-long-long

# Command to make an object file:
COMPILE = $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c

# Command used to link objects:
# LD is sort of traditional as well,
# probably stands for Linker-loaDer
LD = cc

# The flags to pass to the linker
# At UW CSE, if you are using /uns/g++ as your compiler,
# You'll probably want to add -Wl,-rpath,/uns/lib here.
# This way you won't have to set the LD_LIBRARY_PATH environment
# variable just to run the programs you compile.
LDFLAGS = -L/usr/X11R6/lib `curl-config --libs`

# Any libraries we may want to use
# you would need a -lm here if you were using math functions.
LIBS = -lsupc++ -lpthread -lX11 -ljpeg -lpng -lGL

# The name of the executable we are trying to build
PROGRAM = niv

# The object files we need to build the program
OBJS = jpgtest.o Window.o inputjpgcurl.o

$(PROGRAM) : $(OBJS)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

-include $(OBJS:.o=.d)

#%.d : %.cpp
#	set -e; $(CXX) -MM $(CPPFLAGS) $< \
#		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
#		[ -s $@ ] || rm -f $@

%.d: %.cpp
	$(CXX) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.o : %.cpp
	$(COMPILE) $< -o $@

clean :
	rm -f $(PROGRAM) $(OBJS)

