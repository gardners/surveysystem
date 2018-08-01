/*
  Serialiasers and de-serialisers for various structures.

*/

#include <stdio.h>
#include <string.h>

#include "survey.h"
#include "question_types.h"

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

int serialise_int(int in,char *out,int max_len)
{
  int retVal=0;
  char temp[16];
  snprintf(temp,16,"%d",in);
  if (strlen(temp)>=max_len) retVal=-1;
  else { strcpy(out,temp); retVal=strlen(temp); }
  return retVal;
}

int serialise_longlong(long long in,char *out,int max_len)
{
  int retVal=0;
  char temp[32];
  snprintf(temp,32,"%lld",in);
  if (strlen(temp)>=max_len) retVal=-1;
  else { strcpy(out,temp); retVal=strlen(temp); }
  return retVal;
}

int space_check(int append_len,int existing_len,int max_len)
{
  int retVal=0;
  if ((existing_len+append_len)>max_len) retVal=-1;
  return retVal;
}

#define APPEND_STRING(NEW,NL,O,L) { strcpy(&O[L],NEW); L+=NL; }

#define APPEND_COLON(O,L,ML) { if (space_check(1,L,ML)) break; O[L++]=':'; O[L]=0; }

#define SERIALISE_BEGIN(O,L,ML) { int encoded_len=0; const int encoded_max_len=65536; char encoded[encoded_max_len]; L=0;

#define SERIALISE_COMPLETE(O,L,ML) if (L>0) { L--; O[L]=0; }  }

#define SERIALISE_THING(S,O,L,ML,SERIALISER) \
  encoded_len=SERIALISER(S,encoded,encoded_max_len); \
  if (encoded_len<0) break; \
  if (space_check(encoded_len,L,ML)) break; \
  APPEND_STRING(encoded,encoded_len,O,L); \
  APPEND_COLON(O,L,ML);

#define SERIALISE_STRING(S,O,L,ML) SERIALISE_THING(S,O,L,ML,escape_string);
#define SERIALISE_INT(S,O,L,ML) SERIALISE_THING(S,O,L,ML,serialise_int);
#define SERIALISE_LONGLONG(S,O,L,ML) SERIALISE_THING(S,O,L,ML,serialise_longlong);

int serialise_question_type(int qt,char *out,int out_max_len)
{
  int retVal=0;
  do {
    if (qt<1) {retVal=-1; break;}
    if (qt>NUM_QUESTION_TYPES) {retVal=-1; break;}
    if (strlen(question_type_names[qt])>=out_max_len) {retVal=-1; break;}
    strcpy(out,question_type_names[qt]); retVal=strlen(out);
  } while (0);

  return retVal;
}

int serialise_question(struct question *q,char *out,int max_len)
{
  int retVal=-1;
  int len=0;
  do {
    SERIALISE_BEGIN(out,len,max_len);
    
    SERIALISE_STRING(q->uid,out,len,max_len);
    SERIALISE_STRING(q->question_text,out,len,max_len);
    SERIALISE_STRING(q->question_html,out,len,max_len);
    SERIALISE_THING(q->type,out,len,max_len,serialise_question_type);
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
