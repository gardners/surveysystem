SRCDIR = src
INCDIR = include

# Python API: #539, #396
PY_VERSION_MAJOR = 3
PY_VERSION_MINOR = 8
PY_COPT  := $(shell python3-config --embed --includes)
PY_LOPT := $(shell python3-config --embed --ldflags)
PY_CHECK = "import sys; assert sys.version_info >= (${PY_VERSION_MAJOR}, ${PY_VERSION_MINOR}), \"Python >= ${PY_VERSION_MAJOR}.${PY_VERSION_MINOR} required!\""

KCGIARS=	kcgi/libkcgihtml.a kcgi/libkcgi.a kcgi/libkcgijson.a

STATICHEADERS=	Makefile \
		$(INCDIR)/errorlog.h \
		$(INCDIR)/survey.h \
		$(INCDIR)/validators.h \
		$(INCDIR)/sha1.h \
		$(INCDIR)/serialisers.h \
		$(INCDIR)/utils.h \
		$(INCDIR)/py_module.h \
		$(INCDIR)/test.h

STATICSRCS=	$(SRCDIR)/serialisers.c \
		$(SRCDIR)/sha1.c \
		$(SRCDIR)/paths.c \
		$(SRCDIR)/log.c \
		$(SRCDIR)/validators.c \
		$(SRCDIR)/sessions.c \
		$(SRCDIR)/nextquestion.c \
		$(SRCDIR)/filelocks.c \
		$(SRCDIR)/errorlog.c \
		$(SRCDIR)/utils.c \
		$(SRCDIR)/py_module.c \
		$(SRCDIR)/py_hooks.c \
		$(SRCDIR)/test_utils.c

FCGIHEADERS=	$(INCDIR)/fcgi.h
FCGISRC=	$(SRCDIR)/fcgi_request.c \
		$(SRCDIR)/fcgi_response.c

GENERATEDHEADERS=$(INCDIR)/question_types.h
GENERATEDSRCS=	$(SRCDIR)/question_types.c

COREOBJS=	$(SRCDIR)/errorlog.o \
		$(SRCDIR)/sha1.o \
		$(SRCDIR)/paths.o \
		$(SRCDIR)/log.o \
		$(SRCDIR)/validators.o \
		$(SRCDIR)/sessions.o \
		$(SRCDIR)/nextquestion.o \
		$(SRCDIR)/serialisers.o \
		$(SRCDIR)/filelocks.o \
		$(SRCDIR)/question_types.o \
		$(SRCDIR)/utils.o \
		$(SRCDIR)/py_module.o \
		$(SRCDIR)/py_hooks.o \
		$(SRCDIR)/test_utils.o

TESTOBJS =	$(COREOBJS) \
		$(SRCDIR)/test_units.o \
		$(SRCDIR)/test_utils.o

CLIOBJS=	$(COREOBJS) \
		$(SRCDIR)/main.o

FCGIOBJS=	$(COREOBJS) \
		$(SRCDIR)/fcgi_main.o \
		$(SRCDIR)/fcgi_request.o \
		$(SRCDIR)/fcgi_response.o

HEADERS=	$(STATICHEADERS) $(GENERATEDHEADERS) $(FCGIHEADERS)

CC=	clang
COPT=	-Wall -O3 -g -Iinclude $(PY_COPT) -Ikcgi
LOPT=	$(PY_LOPT)

all:	pycheck test_units surveycli surveyfcgi test_runner

test_runner:	Makefile $(SRCDIR)/test_runner.o $(TESTOBJS)
	$(CC) $(COPT) -o test_runner $(SRCDIR)/test_runner.o $(COREOBJS) $(LOPT)

%.o:	%.c $(HEADERS)
	$(CC) $(COPT) -c $< -o $@

$(INCDIR)/question_types.h:	$(INCDIR)/survey.h $(STATICHEADERS)
	@echo "// clang-format on" > $@
	grep '#define QTYPE_' $(INCDIR)/survey.h | cut -c8- | sort -k2n -t' ' | tail -1 | awk -F' ' '{ printf("#define NUM_QUESTION_TYPES %s\n",$$2); }' >> $@
	@echo "extern char *question_type_names[1+NUM_QUESTION_TYPES+1];" >> $@
	@echo "// clang-format off" >> $@

$(SRCDIR)/question_types.c:	$(INCDIR)/survey.h $(HEADERS)
	@echo "// clang-format on" > $@
	@echo "#include \"question_types.h\"" >> $@
	@echo "char *question_type_names[1+NUM_QUESTION_TYPES+1]={" >> $@
	@echo "  \"start of list\"," >> $@
	@grep '#define QTYPE_' $(INCDIR)/survey.h | cut -f2- -d_ | sort -k2n -t' ' | awk -F' ' '{ printf("  \"%s\",\n",$$1); }' >> $@
	@echo "  \"end of list\"\n};" >> $@
	@echo "// clang-format off" >> $@

test_units:	$(TESTOBJS)
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

pycheck:
	@echo ----
	python3 -c $(PY_CHECK)
	@echo ----

clean:
	rm -f $(SRCDIR)/*.o $(GENERATEDHEADERS) $(GENERATEDSRCS) $(KCGIARS)

/usr/bin/bmake:
	@echo "running: sudo apt-get install bmake"

# Run static analysis of code using sparse and scan-build
analyse_scanbuild:
	scan-build make

analyse_sparse:
	sparse -std=c99 -D__USE_MISC=1 -Iinclude/ -Ikcgi $(PY_COPT) -Wsparse-all -Wnoundef -Wsomething src/*.c

analyse_cppcheck:
	cppcheck --enable=all src/*c
