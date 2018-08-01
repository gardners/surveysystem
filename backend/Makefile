CC=	clang
COPT=	-Wall -O3 -g -Iinclude
LOPT=

SRCDIR=	src/

all:	test_serialisers

%.o:	%.c	$(HEADERS)
	$(CC) $(COPT) -c $< -o $@

test_serialisers:	$(SRCDIR)/serialisers.o $(SRCDIR)/test_serialisers.o
	$(CC) $(COPT) -o $@ $^ $(LOPT)
