SRCDIR=	src
INCDIR= include

STATICHEADERS=	Makefile \
		$(INCDIR)/survey.h \
		$(INCDIR)/serialisers.h \

GENERATEDHEADERS=$(INCDIR)/question_types.h
GENERATEDSRCS=	$(SRCDIR)/question_types.c

HEADERS=	$(STATICHEADERS) $(GENERATEDHEADERS)

CC=	clang
COPT=	-Wall -O3 -g -Iinclude
LOPT=

all:	test_serialisers

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

test_serialisers:	$(SRCDIR)/serialisers.o $(SRCDIR)/test_serialisers.o $(SRCDIR)/question_types.c
	$(CC) $(COPT) -o $@ $^ $(LOPT)



clean:
	rm $(SRCDIR)/*.o $(GENERATEDHEADERS) $(GENERATEDSRCS)
