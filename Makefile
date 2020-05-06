
#FLAGS = -O3
#FLAGS = -O3 -pg
FLAGS = -O0 -ggdb

all: callisto

OBJDIR = objs_linux
LIBDIR = lib

-include $(OBJDIR)/*.d

CC = g++ -MD -Wall -Werror $(FLAGS) $(LIBS) -c -o

LIBS = -lpthread

OBJS = \
	$(OBJDIR)/callisto_tests.o \
	$(OBJDIR)/callisto_cli.o \
	lib/libcallisto.a \

ETEST = \
	$(OBJDIR)/callisto_min.o \
	lib/libmincallisto.a \

LOBJS = \
	$(OBJDIR)/event.o \
	$(OBJDIR)/api.o \
	$(OBJDIR)/vm.o \
	$(OBJDIR)/token.o \
	$(OBJDIR)/cc.o \
	$(OBJDIR)/value.o \
	$(OBJDIR)/callisto_stdlib.o \
	$(OBJDIR)/callisto_string.o \
	$(OBJDIR)/callisto_math.o \
	$(OBJDIR)/callisto_file.o \
	$(OBJDIR)/callisto_json.o \
	$(OBJDIR)/callisto_iterators.o \

EOBJS = \
	$(OBJDIR)/api.o \
	$(OBJDIR)/event.o \
	$(OBJDIR)/vm.o \
	$(OBJDIR)/value.o \
	$(OBJDIR)/callisto_stdlib.o \
	$(OBJDIR)/callisto_string.o \
	$(OBJDIR)/callisto_math.o \
	$(OBJDIR)/callisto_file.o \
	$(OBJDIR)/callisto_json.o \
	$(OBJDIR)/callisto_iterators.o \

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

lib/libmincallisto.a: $(EOBJS)
	ar r lib/libmincallisto.a $(EOBJS)
	ranlib lib/libmincallisto.a
	@find . -name "*.d" -exec mv {} $(EOBJS) \; 2>/dev/null

test: callisto
	./callisto t

min: $(ETEST)
	g++ $(ETEST) $(LIBS) $(FLAGS) -o min

callisto: $(OBJS)
	g++ $(OBJS) $(LIBS) $(FLAGS) -o callisto

$(OBJDIR)/callisto_cli.o: callisto_cli.cpp
	$(CC) $@ $<

$(OBJDIR)/callisto_min.o: callisto_min.cpp
	$(CC) $@ $<

$(OBJDIR)/event.o: source/event.cpp
	$(CC) $@ $<

$(OBJDIR)/vm.o: source/vm.cpp
	$(CC) $@ $<

$(OBJDIR)/token.o: source/token.cpp
	$(CC) $@ $<

$(OBJDIR)/api.o: source/api.cpp
	$(CC) $@ $<

$(OBJDIR)/cc.o: source/cc.cpp
	$(CC) $@ $<

$(OBJDIR)/value.o: source/value.cpp
	$(CC) $@ $<

$(OBJDIR)/callisto_tests.o: callisto_tests.cpp
	$(CC) $@ $<

$(OBJDIR)/callisto_stdlib.o: source/callisto_stdlib.cpp
	$(CC) $@ $<

$(OBJDIR)/callisto_string.o: source/callisto_string.cpp
	$(CC) $@ $<

$(OBJDIR)/callisto_math.o: source/callisto_math.cpp
	$(CC) $@ $<

$(OBJDIR)/callisto_file.o: source/callisto_file.cpp
	$(CC) $@ $<

$(OBJDIR)/callisto_json.o: source/callisto_json.cpp
	$(CC) $@ $<

$(OBJDIR)/callisto_iterators.o: source/callisto_iterators.cpp
	$(CC) $@ $<

