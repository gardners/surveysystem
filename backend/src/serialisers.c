/*
  Serialiasers and de-serialisers for various structures.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "survey.h"
#include "errorlog.h"
#include "question_types.h"

#define REPORT_IF_FAILED() { if (retVal) fprintf(stderr,"%s:%d: %s() failed.\n",__FILE__,__LINE__,__FUNCTION__); }

int escape_string(char *in,char *out,int max_len)
{
  int retVal=0;
  int out_len=0;
  for(int i=0;in[i];i++) {
    switch(in[i]) {
    case '\r': case '\n': case '\t': case '\b':
      if (out_len>=max_len) { LOG_ERROR("escaped version of string is too long",out); }
      else out[out_len++]='\\';
      if (out_len>=max_len) { LOG_ERROR("escaped version of string is too long",out); }
      else out[out_len++]=in[i];
      break;
    case ':': case '\\':
      if (out_len>=max_len) { LOG_ERROR("escaped version of string is too long",out); }
      else out[out_len++]=':';
      if (out_len>=max_len) { LOG_ERROR("escaped version of string is too long",out); }
      else out[out_len++]=in[i];
      break;
    default:
      if (out_len>=max_len) { LOG_ERROR("escaped version of string is too long",out); }
      else out[out_len++]=in[i];
      break;
    }
  }
  out[out_len]=0;
  if (retVal==0) retVal=out_len;
  return retVal;
}

int serialise_int(int in,char *out,int max_len)
{
  int retVal=0;
  char temp[16];
  do {
    snprintf(temp,16,"%d",in);
    if (strlen(temp)>=max_len) { LOG_ERROR("integer converts to over-long string",temp); }
    else { strcpy(out,temp); retVal=strlen(temp); }
  } while (0);
  return retVal;
}

int serialise_longlong(long long in,char *out,int max_len)
{
  int retVal=0;
  char temp[32];
  do {
    snprintf(temp,32,"%lld",in);
    if (strlen(temp)>=max_len) { LOG_ERROR("long long converts to over-long string",temp); }
    else { strcpy(out,temp); retVal=strlen(temp); }
  } while (0);
  return retVal;
}

int space_check(int append_len,int existing_len,int max_len)
{
  int retVal=0;
  do {
    if ((existing_len+append_len)>max_len) { LOG_ERROR("Insufficient space to append next string",""); }
  } while (0);
  return retVal;
}

int deserialise_parse_field(char *in,int *in_offset,char *out)
{
  int retVal=0;
  int offset=*in_offset;
  int olen=0;

  do {
    if (offset) {
      if (in[offset]!=':') {
	LOG_ERROR("Expected : before next field\n",&in[offset]);
	break;
      }
      else offset++;
    }
    
    out[olen]=0;
    if (!in) LOG_ERROR("input string is NULL","");
    if (!in[0]) LOG_ERROR("input string is empty","");
    for(;in[offset]&&(olen<16383)&&in[offset]!=':';offset++)
      {
	// Allow some \ escape characters
	if (in[offset]=='\\') {
	  if (!in[offset+1]) LOG_ERROR("String ends in \\\n",in);
	  switch (in[offset+1]) {
	  case ':': case '\\':
	    out[olen++]=in[offset+1]; break;
	  case 'r': out[olen++]='\r'; break;
	  case 'n': out[olen++]='\n'; break;
	  case 'b': out[olen++]='\b'; break;
	  default:
	    LOG_ERROR("Illegal escape character\n",&in[offset+1]);
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
    if (!field) LOG_ERROR("field is NULL","");
    if (!strlen(field)) LOG_ERROR("field is empty string","");
    int offset=0;
    if (field[offset]=='-') offset++;
    for(int i=offset;field[i];i++)
      if (field[i]<'0'||field[i]>'9') LOG_ERROR("integer field contains non-digit",field);
    if (!retVal) *s=atoi(field);
    
  } while(0);
  return retVal;
}

int deserialise_longlong(char *field,long long *s)
{
  int retVal=0;
  do {
    if (!field) LOG_ERROR("field is NULL","");
    if (!strlen(field)) LOG_ERROR("field is empty string","");
    int offset=0;
    if (field[offset]=='-') offset++;
    for(int i=offset;field[i];i++)
      if (field[i]<'0'||field[i]>'9') LOG_ERROR("long long field contains non-digit",field);
    if (!retVal) *s=atoll(field);
    
  } while(0);
  return retVal;
}


int deserialise_string(char *field,char **s)
{
  int retVal=0;
  do {
    *s=NULL;
    if (!field) { LOG_ERROR("field is NULL",""); }
    else *s=strdup(field);
    if (!*s) { LOG_ERROR("field is empty string",""); }
  } while (0);
  return retVal;
}

#define DESERIALISE_BEGIN(O,L,ML) { int offset=0; char field[16384]; 
#define DESERIALISE_COMPLETE(O,L,ML) if (offset<L) { LOG_ERROR("Junk at end of serialised object",""); } }
#define DESERIALISE_NEXT_FIELD() if (deserialise_parse_field(in,&offset,field)) { LOG_ERROR("failed to parse next field",&in[offset]); }
#define DESERIALISE_THING(S,DESERIALISER) \
  DESERIALISE_NEXT_FIELD();				\
  if (DESERIALISER(field,&S)) { LOG_ERROR("call to " #DESERIALISER " failed",field); }
#define DESERIALISE_INT(S) DESERIALISE_THING(S,deserialise_int)
#define DESERIALISE_STRING(S) DESERIALISE_THING(S,deserialise_string);
#define DESERIALISE_LONGLONG(S) DESERIALISE_THING(S,deserialise_longlong)


#define APPEND_STRING(NEW,NL,O,L) { strcpy(&O[L],NEW); L+=NL; }

#define APPEND_COLON(O,L,ML) { if (space_check(1,L,ML)) { LOG_ERROR("serialised string too long",""); } O[L++]=':'; O[L]=0; }

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
    if (qt<1) LOG_ERROR("was asked to serialise an illegal question type","");
    if (qt>NUM_QUESTION_TYPES) LOG_ERROR("was asked to serialise an illegal question type","");
    if (strlen(question_type_names[qt])>=out_max_len) LOG_ERROR("question type name too long",question_type_names[qt]);
    strcpy(out,question_type_names[qt]); retVal=strlen(out);
  } while (0);

  REPORT_IF_FAILED();
  return retVal;
}

int deserialise_question_type(char *field,int *s)
{
  int retVal=0;
  do {
    int qt;
    for(qt=1;qt<NUM_QUESTION_TYPES;qt++)
      if (!strcasecmp(field,question_type_names[qt])) {
	retVal=0;
	*s=qt;
	break;
      }
    if (qt==NUM_QUESTION_TYPES) LOG_ERROR("invalid question type name",field);
  } while (0);

  return retVal;  
}

int serialise_question(struct question *q,char *out,int max_len)
{
  int retVal=0;
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
    
  } while(0);

  return retVal;
}

int dump_question(FILE *f,char *msg,struct question *q)
{
  int retVal=0;
  do {
    char temp[8192];
    fprintf(f,"%s:\n",msg);
    escape_string(q->uid,temp,8192);
    fprintf(f,"  uid='%s'\n",temp);
    escape_string(q->question_text,temp,8192);
    fprintf(f,"  question_text='%s'\n",temp);
    escape_string(q->question_html,temp,8192);
    fprintf(f,"  question_html='%s'\n",temp);
    fprintf(f,"  question type=%s\n",
	    ((q->type>=1)&&(q->type<=NUM_QUESTION_TYPES)) ? question_type_names[q->type]: "<unknown>");
    fprintf(f,"  flags=0x%08X\n",q->flags);
    escape_string(q->default_value,temp,8192);
    fprintf(f,"  default_value='%s'\n",temp);
    fprintf(f,"  min_value=%lld\n",q->min_value);
    fprintf(f,"  max_value=%lld\n",q->max_value);
    fprintf(f,"  decimal_places=%d\n",q->decimal_places);
    fprintf(f,"  num_choices=%d\n",q->num_choices);

  } while(0);

  return retVal;
}

int deserialise_question(char *in,struct question *q)
{
  int retVal=0;
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
    
  } while(0);

  return retVal;
}

int serialise_answer(struct answer *a,char *out,int max_len)
{
  int retVal=0;
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
    
  } while(0);

  return retVal;
}

int deserialise_answer(char *in,struct answer *a)
{
  int retVal=0;
  int len=0;
  do {
    DESERIALISE_BEGIN(out,len,max_len);

    DESERIALISE_STRING(a->uid);
    DESERIALISE_LONGLONG(a->value);
    DESERIALISE_LONGLONG(a->lat);
    DESERIALISE_LONGLONG(a->lon);
    DESERIALISE_LONGLONG(a->time_begin);
    DESERIALISE_LONGLONG(a->time_end);
    DESERIALISE_INT(a->time_zone_delta);
    DESERIALISE_INT(a->dst_delta);


    // Check that we are at the end of the input string
    DESERIALISE_COMPLETE(out,len,max_len);
    
  } while(0);

  return retVal;
}



#define COMPARE_INT(S) { if (q1->S>q2->S) { LOG_ERROR(#S " fields do not match",""); }  else if (q1->S<q2->S) { LOG_ERROR(#S " fields do not match",""); } else retVal=0; if (retVal) break; }
#define COMPARE_LONGLONG(S) COMPARE_INT(S)
#define COMPARE_STRING(S) { if ((!q1->S)||(!q2->S)) { LOG_ERROR( #S " fields dot not match",""); } else { if (strcmp(q1->S,q2->S)) { fprintf(stderr,#S " fields do not match\n");  LOG_ERROR(#S " fields do not match",""); }  } }

int compare_questions(struct question *q1, struct question *q2)
{
  int retVal=0;
  do {
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
