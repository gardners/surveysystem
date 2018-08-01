/*
  Serialiasers and de-serialisers for various structures.

*/

#include "survey.h"

int escape_string(char *in,char *out,int max_len)
{
  int retVal=0;
  int out_len=0;
  for(int i=0;in[i];i++) {
    switch(in[i]) {
    case ':':
      if (out_len>=max_len) retVal=-1;
      else out[out_len++]=in[i];
      break;
    default:
      if (out_len>=max_len) retVal=-1;
      else out[out_len++]=':';
      if (out_len>=max_len) retVal=-1;
      else out[out_len++]=in[i];
    }
  }
  if (retVal==0) retVal=out_len;
  return retVal;
}

int space_check(int append_len,int exising_len,int max_len)
{
  int retVal=0;
  if ((existing_len+append_len)>max_len) retVal=-1;
  return retVal;
}

#define APPEND_STRING(NEW,NL,O,L) { strcpy(&O[L],NEW); L+=NL; }

#define SERIALISE_STRING(S,O,L,ML) \
  encoded_len=escape_string(S,encoded,encoded_max_len); \
  if (encoded_len<0) break; \
  if (space_check(encoded_len,L,ML)) break; \
  APPEND_STRING(encoded,encoded_len,O,L);

int serialise_question(struct question *q,char *out,int max_len)
{
  int retVal=-1;
  int len=0;
  do {
    SERIALISE_BEGIN(out,len,max_len);
    
    SERIALISE_STRING(q->uid,out,len,max_len);
    SERIALISE_STRING(q->question_text,out,len,max_len);
    SERIALISE_STRING(q->question_html,out,len,max_len);
    SERIALISE_ENUM(q->type,question_type,out,len,max_len);
    SERIALISE_INT(q->flags,out,len,max_len);
    SERIALISE_STRING(q->default_value,out,len,max_len);
    SERIALISE_LONGLONG(q->min_value,out,len,max_len);
    SERIALISE_LONGLONG(q->max_value,out,len,max_len);
    SERIALISE_INT(q->decimal_places,out,len,max_len);
    SERIALISE_INT(q->num_choices,out,len,max_len);

    // Trim terminal separator character
    SERIALISE_COMPLETE(out,len,max_len);
    
    retVal=0;
  } while(0);

  return retVal;
}
