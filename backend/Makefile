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

KCGIARS=	kcgi/libkcgihtml.a kcgi/libkcgi.a kcgi/libkcgijson.a

STATICHEADERS=	Makefile \
		$(INCDIR)/errorlog.h \
		$(INCDIR)/survey.h \
		$(INCDIR)/sha1.h \
		$(INCDIR)/code_instrumentation.h \
		$(INCDIR)/serialisers.h \
		$(INCDIR)/timeutils.h

STATICSRCS=	$(SRCDIR)/serialisers.c \
		$(SRCDIR)/sha1.c \
		$(SRCDIR)/paths.c \
		$(SRCDIR)/log.c \
		$(SRCDIR)/sessions.c \
		$(SRCDIR)/nextquestion.c \
		$(SRCDIR)/code_instrumentation.c \
		$(SRCDIR)/filelocks.c \
		$(SRCDIR)/errorlog.c \
		$(SRCDIR)/timeutils.c

GENERATEDHEADERS=$(INCDIR)/question_types.h
GENERATEDSRCS=	$(SRCDIR)/question_types.c

COREOBJS=	$(SRCDIR)/errorlog.o \
		$(SRCDIR)/sha1.o \
		$(SRCDIR)/paths.o \
		$(SRCDIR)/log.o \
		$(SRCDIR)/sessions.o \
		$(SRCDIR)/nextquestion.o \
		$(SRCDIR)/serialisers.o \
		$(SRCDIR)/code_instrumentation.o \
		$(SRCDIR)/filelocks.o \
		$(SRCDIR)/question_types.o \
		$(SRCDIR)/timeutils.o 

TESTOBJS=	$(COREOBJS) \
		$(SRCDIR)/test_serialisers.o

CLIOBJS=	$(COREOBJS) \
		$(SRCDIR)/main.o

FCGIOBJS=	$(COREOBJS) \
		$(SRCDIR)/fcgimain.o

HEADERS=	$(STATICHEADERS) $(GENERATEDHEADERS)

CC=	clang
COPT=	-Wall -O3 -g -Iinclude `$(PYCONFIG) --includes` -Ikcgi
LOPT=	`$(PYCONFIG) --libs --ldflags`

all:	test_serialisers surveycli surveyfcgi test_runner

test_runner:	Makefile $(SRCDIR)/test_runner.o $(COREOBJS)
	$(CC) $(COPT) -o test_runner $(SRCDIR)/test_runner.o $(COREOBJS) $(LOPT)  

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
	$(CC) $(COPT) -o $@ $^ $(LOPT)

kcgi/kcgijson.c:
	git submodule init
	git submodule update

$(KCGIARS):	/usr/bin/bmake kcgi/kcgijson.c
	( cd kcgi ; ./configure && bmake )

surveyfcgi:	$(FCGIOBJS) $(KCGIARS)
	$(CC) $(COPT) -o $@ $^ $(LOPT) $(KCGIARS) -lz

clean:
	rm $(SRCDIR)/*.o $(GENERATEDHEADERS) $(GENERATEDSRCS) $(KCGIARS)

/usr/bin/bmake:
	echo "Run sudo apt-get install bmake"

# Run static analysis of code using sparse and scan-build
analyse_scanbuild:
	scan-build-3.8 make

analyse_sparse:
	sparse -std=c99 -D__USE_MISC=1 -Iinclude/ -Ikcgi `$(PYCONFIG) --includes`  -Wsparse-all -Wnoundef src/*.c
