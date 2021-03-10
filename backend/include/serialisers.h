#ifndef __SERIALISERS_H__
#define __SERIALISERS_H__

#include "survey.h"

#define MISMATCH_IS_NOT_AN_ERROR 0
#define MISMATCH_IS_AN_ERROR 1

int compare_questions(struct question *q1, struct question *q2,
                      int mismatchIsError);
int compare_answers(struct answer *a1, struct answer *a2, int mismatchIsError);
int serialise_question(struct question *q, char *out, int max_len);
int serialise_answer(struct answer *a, enum answer_scope scope, char *out, int max_len);
int deserialise_question(char *in, struct question *q);
int deserialise_answer(char *in, answer_scope scope, struct answer *a);
int serialise_question_type(int qt, char *out, int out_max_len); // #358
int escape_string(char *in, char *out, int max_len);

int dump_question(FILE *f, struct question *q);
int dump_answer(FILE *f, struct answer *a);

int serialiser_count_columns(char *line, size_t max_len);
#endif
