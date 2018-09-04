#define MISMATCH_IS_NOT_AN_ERROR 0
#define MISMATCH_IS_AN_ERROR 1
int compare_questions(struct question *q1, struct question *q2, int mismatchIsError);
int compare_answers(struct answer *a1, struct answer *a2, int mismatchIsError);
int serialise_question(struct question *q,char *out,int max_len);
int serialise_answer(struct answer *a,char *out,int max_len);
int deserialise_question(char *in,struct question *q);
int deserialise_answer(char *in,struct answer *a);
int dump_question(FILE *f,char *msg,struct question *q);
int dump_answerr(FILE *f,char *msg,struct answer *a);

