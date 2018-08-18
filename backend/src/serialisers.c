/*
  Serialiasers and de-serialisers for various structures.

*/

#include <stdio.h>
#include <stdlib.h>
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

int deserialise_parse_field(char *in,int *in_offset,char *out)
{
  int retVal=0;
  int offset=*in_offset;
  int olen=0;

  do {
    if (offset) {
      if (in[offset]!=':') { retVal=-1; break; }
      else offset++;
    }
    
    out[olen]=0;
    if (!in) { retVal=-1; break; }
    if (!in[0]) { retVal=-1; break; }
    for(offset=0;in[offset]&&(olen<16383)&&in[offset]!=':';olen++)
      {
	// Allow some \ escape characters
	if (in[offset]=='\\') {
	  if (!in[offset+1]) { retVal=-1; break; }
	  switch (in[offset+1]) {
	  case ':': out[olen++]=in[offset+1]; break;
	  case 'r': out[olen++]='\r'; break;
	  case 'n': out[olen++]='\n'; break;
	  case 'b': out[olen++]='\b'; break;
	  default:
	    fprintf(stderr,"Illegal escape character 0x%02x\n",in[offset+1]);
	    retVal=-1;
	    break;
	  }
	  offset++;
	  out[olen]=0;
	} else {
	  out[olen++]=in[offset];
	  out[olen]=0;
	}
      }
    *in_offset=offset;
    
  } while(0);
  
  return retVal;  
}

int deserialise_int(char *field,int *s)
{
  int retVal=0;
  do {
    if (!field) { retVal=-1; break; }
    if (!strlen(field)) { retVal=-1; break; }
    int offset=0;
    if (field[offset]=='-') offset++;
    for(int i=offset;field[i];i++)
      if (field[i]<'0'||field[i]>'9') { retVal=-1; break; }
    if (!retVal) *s=atoi(field);
    
  } while(0);
  return retVal;
}

int deserialise_longlong(char *field,long long *s)
{
  int retVal=0;
  do {
    if (!field) { retVal=-1; break; }
    if (!strlen(field)) { retVal=-1; break; }
    int offset=0;
    if (field[offset]=='-') offset++;
    for(int i=offset;field[i];i++)
      if (field[i]<'0'||field[i]>'9') { retVal=-1; break; }
    if (!retVal) *s=atoll(field);
    
  } while(0);
  return retVal;
}


int deserialise_string(char *field,char **s)
{
  int retVal=0;
  *s=NULL;
  if (!field) retVal=-1; else *s=strdup(field);
  if (!*s) retVal=-1;
  return retVal;
}

#define DESERIALISE_BEGIN(O,L,ML) { int offset=0; char field[16384]; 
#define DESERIALISE_COMPLETE(O,L,ML) if (offset<L) { fprintf(stderr,"Junk at end of serialised object\n"); retVal=-1; break; } }
#define DESERIALISE_NEXT_FIELD() if (deserialise_parse_field(in,&offset,field)) { retVal=-1; break; }
#define DESERIALISE_THING(S,DESERIALISER) \
  DESERIALISE_NEXT_FIELD();		  \
  DESERIALISER(field,&S);
#define DESERIALISE_INT(S) DESERIALISE_THING(S,deserialise_int)
#define DESERIALISE_STRING(S) DESERIALISE_THING(S,deserialise_string)
#define DESERIALISE_LONGLONG(S) DESERIALISE_THING(S,deserialise_longlong)


#define APPEND_STRING(NEW,NL,O,L) { strcpy(&O[L],NEW); L+=NL; }

#define APPEND_COLON(O,L,ML) { if (space_check(1,L,ML)) break; O[L++]=':'; O[L]=0; }

#define SERIALISE_BEGIN(O,L,ML) { int encoded_len=0; const int encoded_max_len=65536; char encoded[encoded_max_len]; L=0;

#define SERIALISE_COMPLETE(O,L,ML) if (L>0) { L--; O[L]=0; }  }

#define SERIALISE_THING(S,SERIALISER)	     \
  encoded_len=SERIALISER(S,encoded,encoded_max_len); \
  if (encoded_len<0) break; \
  if (space_check(encoded_len,len,max_len)) break; \
  APPEND_STRING(encoded,encoded_len,out,len); \
  APPEND_COLON(out,len,max_len);

#define SERIALISE_STRING(S) SERIALISE_THING(S,escape_string);
#define SERIALISE_INT(S) SERIALISE_THING(S,serialise_int);
#define SERIALISE_LONGLONG(S) SERIALISE_THING(S,serialise_longlong);

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

int deserialise_question_type(char *field,int *s)
{
  int retVal=-1;
  do {
    for(int qt=1;qt<NUM_QUESTION_TYPES;qt++)
      if (!strcasecmp(field,question_type_names[qt])) {
	retVal=0;
	*s=qt;
      }
  } while (0);

  return retVal;  
}

int serialise_question(struct question *q,char *out,int max_len)
{
  int retVal=-1;
  int len=0;
  do {
    SERIALISE_BEGIN(out,len,max_len);
    
    SERIALISE_STRING(q->uid);
    SERIALISE_STRING(q->question_text);
    SERIALISE_STRING(q->question_html);
    SERIALISE_THING(q->type,serialise_question_type);
    SERIALISE_INT(q->flags);
    SERIALISE_STRING(q->default_value);
    SERIALISE_LONGLONG(q->min_value);
    SERIALISE_LONGLONG(q->max_value);
    SERIALISE_INT(q->decimal_places);
    SERIALISE_INT(q->num_choices);

    // Trim terminal separator character
    SERIALISE_COMPLETE(out,len,max_len);
    
    retVal=0;
  } while(0);

  return retVal;
}

int deserialise_question(char *in,struct question *q)
{
  int retVal=-1;
  int len=0;
  do {
    DESERIALISE_BEGIN(out,len,max_len);

    DESERIALISE_STRING(q->uid);
    DESERIALISE_STRING(q->question_text);
    DESERIALISE_STRING(q->question_html);
    DESERIALISE_THING(q->type,deserialise_question_type);
    DESERIALISE_INT(q->flags);
    DESERIALISE_STRING(q->default_value);
    DESERIALISE_LONGLONG(q->min_value);
    DESERIALISE_LONGLONG(q->max_value);
    DESERIALISE_INT(q->decimal_places);
    DESERIALISE_INT(q->num_choices);

    // Check that we are at the end of the input string
    DESERIALISE_COMPLETE(out,len,max_len);
    
    retVal=0;
  } while(0);

  return retVal;
}

int serialise_answer(struct answer *a,char *out,int max_len)
{
  int retVal=-1;
  int len=0;
  do {
    SERIALISE_BEGIN(out,len,max_len);
    
    SERIALISE_STRING(a->uid);
    SERIALISE_LONGLONG(a->value);
    SERIALISE_LONGLONG(a->lat);
    SERIALISE_LONGLONG(a->lon);
    SERIALISE_LONGLONG(a->time_begin);
    SERIALISE_LONGLONG(a->time_end);
    SERIALISE_INT(a->time_zone_delta);
    SERIALISE_INT(a->dst_delta);

    // Trim terminal separator character
    SERIALISE_COMPLETE(out,len,max_len);
    
    retVal=0;
  } while(0);

  return retVal;
}

#define COMPARE_INT(S) { if (q1->S>q2->S) result=1; else if (q1->S<q2->S) result=-1; else result=0; if (result) break; }
#define COMPARE_LONGLONG(S) COMPARE_INT(S)
#define COMPARE_STRING(S) { if ((!q1->S)||(!q2->S)) result=-1; else result=strcmp(q1->S,q2->S); if (result) break; }

int compare_questions(struct question *q1, struct question *q2)
{
  int retVal=0;
  do {
    int result;

    COMPARE_STRING(uid);
    COMPARE_STRING(question_text);
    COMPARE_STRING(question_html);
    COMPARE_INT(type);
    COMPARE_INT(flags);
    COMPARE_STRING(default_value);
    COMPARE_LONGLONG(min_value);
    COMPARE_LONGLONG(max_value);
    COMPARE_INT(decimal_places);
    COMPARE_INT(num_choices);

  } while(0);
  return retVal;
}
