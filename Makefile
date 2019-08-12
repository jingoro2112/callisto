
FLAGS = -O3
#FLAGS = -O3 -pg
#FLAGS = -O0 -ggdb
#FLAGS = -O0

all: callisto lib/libcallisto.a

OBJDIR = objs_linux
LIBDIR = lib

-include $(OBJDIR)/*.d

CC = g++ -std=c++11 -MD -Wall -Werror $(FLAGS) $(LIBS) -c -o

OBJS = \
	$(OBJDIR)/callisto_tests.o \
	$(OBJDIR)/callisto_cli.o \
	lib/libcallisto.a \

LOBJS = \
	$(OBJDIR)/api.o \
	$(OBJDIR)/vm.o \
	$(OBJDIR)/cc.o \
	$(OBJDIR)/util.o \
	$(OBJDIR)/token.o \
	$(OBJDIR)/array.o \
	$(OBJDIR)/callisto_stdlib.o \
	$(OBJDIR)/callisto_string.o \
	
clean:
	-@rm -rf $(OBJDIR)
	-@mkdir $(OBJDIR)
	-@rm -rf $(LIBDIR)
	-@mkdir $(LIBDIR)
	-@rm -f callisto

lib/libcallisto.a: $(LOBJS)
	ar r lib/libcallisto.a $(LOBJS)
	ranlib lib/libcallisto.a
	@find . -name "*.d" -exec mv {} $(OBJDIR) \; 2>/dev/null

callisto: $(OBJS) lib/libcallisto.a
	g++ $(OBJS) $(FLAGS) -o callisto

$(OBJDIR)/callisto_cli.o: callisto_cli.cpp
	$(CC) $@ $<

$(OBJDIR)/vm.o: source/vm.cpp
	$(CC) $@ $<

$(OBJDIR)/token.o: source/token.cpp
	$(CC) $@ $<

$(OBJDIR)/array.o: source/array.cpp
	$(CC) $@ $<

$(OBJDIR)/api.o: source/api.cpp
	$(CC) $@ $<

$(OBJDIR)/cc.o: source/cc.cpp
	$(CC) $@ $<

$(OBJDIR)/callisto_tests.o: callisto_tests.cpp
	$(CC) $@ $<

$(OBJDIR)/util.o: source/util.cpp
	$(CC) $@ $<

$(OBJDIR)/callisto_stdlib.o: source/callisto_stdlib.cpp
	$(CC) $@ $<

$(OBJDIR)/callisto_string.o: source/callisto_string.cpp
	$(CC) $@ $<
