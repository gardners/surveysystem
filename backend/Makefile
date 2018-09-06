SRCDIR=	src
INCDIR= include

STATICHEADERS=	Makefile \
		$(INCDIR)/errorlog.h \
		$(INCDIR)/survey.h \
		$(INCDIR)/sha1.h \
		$(INCDIR)/code_instrumentation.h \
		$(INCDIR)/serialisers.h

STATICSRCS=	$(SRCDIR)/serialisers.c \
		$(SRCDIR)/sha1.c \
		$(SRCDIR)/paths.c \
		$(SRCDIR)/sessions.c \
		$(SRCDIR)/nextquestion.c \
		$(SRCDIR)/code_instrumentation.c \
		$(SRCDIR)/errorlog.c

GENERATEDHEADERS=$(INCDIR)/question_types.h
GENERATEDSRCS=	$(SRCDIR)/question_types.c

COREOBJS=	$(SRCDIR)/errorlog.o \
		$(SRCDIR)/sha1.o \
		$(SRCDIR)/paths.o \
		$(SRCDIR)/sessions.o \
		$(SRCDIR)/nextquestion.o \
		$(SRCDIR)/serialisers.o \
		$(SRCDIR)/code_instrumentation.o \
		$(SRCDIR)/question_types.o
TESTOBJS=	$(COREOBJS) \
		$(SRCDIR)/test_serialisers.o

CLIOBJS=	$(COREOBJS) \
		$(SRCDIR)/main.o

HEADERS=	$(STATICHEADERS) $(GENERATEDHEADERS)

CC=	clang
COPT=	-Wall -O3 -g -Iinclude `/usr/bin/python-config --includes`
LOPT=

all:	test_serialisers surveycli

%.o:	%.c $(HEADERS)
	$(CC) $(COPT) -c $< -o $@


$(INCDIR)/question_types.h:	$(INCDIR)/survey.h $(STATICHEADERS)
	grep QTYPE_ $(INCDIR)/survey.h | cut -c8- | sort -k2n -t' ' | tail -1 | awk -F' ' '{ printf("#define NUM_QUESTION_TYPES %s\n",$$2); }' >$@
	echo "extern char *question_type_names[1+NUM_QUESTION_TYPES+1];" >>$@

$(SRCDIR)/question_types.c:	$(INCDIR)/survey.h $(HEADERS)
	grep QTYPE_ $(INCDIR)/survey.h | cut -c8- | sort -k2n -t' ' | tail -1 | awk -F' ' '{ printf("#define NUM_QUESTION_TYPES %s\n",$$2); }' >>$@
	echo "#include \"question_types.h\"" > $@
	echo "char *question_type_names[1+NUM_QUESTION_TYPES+1]={" >> $@
	echo "  \"start of list\"," >> $@
	grep QTYPE_ $(INCDIR)/survey.h | cut -f2- -d_ | sort -k2n -t' ' | awk -F' ' '{ printf("  \"%s\",\n",$$1); }' >>$@
	echo "  \"end of list\"};" >> $@

test_serialisers:	$(TESTOBJS)
	$(CC) $(COPT) -o $@ $^ $(LOPT)

surveycli:	$(CLIOBJS)
	$(CC) $(COPT) -o $@ $^ $(LOPT) `/usr/bin/python-config --libs --ldflags`

clean:
	rm $(SRCDIR)/*.o $(GENERATEDHEADERS) $(GENERATEDSRCS)
