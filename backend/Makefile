SRCDIR=	src
INCDIR= include

# NOTE: This program REQUIRES Python >=3.7, as 3.6 has a bug with some of the C
# API stuff
PYTHONVERSION=	3.7
PYCONFIG=/usr/bin/python$(PYTHONVERSION)-config
ifeq ("$(wildcard $(PYCONFIG))","")
PYCONFIG=/opt/local/Library/Frameworks/Python.framework/Versions/$(PYTHONVERSION)/bin/python$(PYTHONVERSION)-config
endif
ifeq ("$(wildcard $(PYCONFIG))","")
PYCONFIG=/Library/Frameworks/Python.framework/Versions/$(PYTHONVERSION)/bin/python$(PYTHONVERSION)-config
endif

KCGIARS=	kcgi/libkcgihtml.a kcgi/libkcgi.a

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

FCGIOBJS=	$(COREOBJS) \
		$(SRCDIR)/fcgimain.o

HEADERS=	$(STATICHEADERS) $(GENERATEDHEADERS)

CC=	clang
COPT=	-Wall -O3 -g -Iinclude `$(PYCONFIG) --includes` -Ikcgi
LOPT=

all:	test_serialisers surveycli surveyfcgi

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
	$(CC) $(COPT) -o $@ $^ $(LOPT) `$(PYCONFIG) --libs --ldflags`

surveycli:	$(CLIOBJS)
	$(CC) $(COPT) -o $@ $^ $(LOPT) `$(PYCONFIG) --libs --ldflags`

$(KCGIARS):	
	git submodule init
	git submodule update
	( cd kcgi ; ./configure && make )

surveyfcgi:	$(FCGIOBJS) $(KCGIARS)
	$(CC) $(COPT) -o $@ $^ $(LOPT) `$(PYCONFIG) --libs --ldflags` $(KCGIARS) -lz

lighttpd2/bin/lighttpd:
	git submodule init
	git submodule update
	( cd lighttpd2 ; autoreconf -i && autoconf && ./configure && make )

clean:
	rm $(SRCDIR)/*.o $(GENERATEDHEADERS) $(GENERATEDSRCS)
