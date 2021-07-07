#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "errorlog.h"
#include "paths.h"
#include "question_types.h"
#include "serialisers.h"
#include "sha1.h"
#include "survey.h"
#include "validators.h"
#include "utils.h"

/**
 * see: survey.h enum actions
 * #455
 */
char *session_action_names[NUM_SESSION_ACTIONS] = {
  "NONE",
  "SESSION_NEW",
  "SESSION_DELETE",
  "SESSION_NEXTQUESTIONS",
  "SESSION_ADDANSWER",
  "SESSION_DELETEANSWER",
  "SESSION_ANALYSIS"
};

/**
 * #379 validate requested action against current session
 */
int validate_session_action(enum actions action, struct session *ses, char *msg, size_t sz) {
  int is_error = 0;
  msg[0] = 0;

  if (!ses) {
    LOG_WARNV("validate_session_action(): session is NULL", 0);
    return -1;
  }

  switch (action) {

    case ACTION_SESSION_DELETE:
      // TODO implement regime for allowing session deletion
      is_error = -1;
      strncpy(msg, "(DELETE SESSION) Session cannot be deleted!", sz);
      break;

    /*
    // #408, disabled, let it pass to nexquestion handlers in order to calculate progress
    case ACTION_SESSION_NEXTQUESTIONS:
      if (ses->state >= SESSION_CLOSED) {
        is_error = -1;
        strncpy(msg, "(NEXT QUESTIONS) Session is closed!", sz);
        break;
      }
      break;
    */

    case ACTION_SESSION_ADDANSWER:
      if (ses->state == SESSION_FINISHED) {
        is_error = -1;
        strncpy(msg, "(ADD ANSWER) Session is finished!", sz);
        break;
      }
      if (ses->state >= SESSION_CLOSED) {
        is_error = -1;
        strncpy(msg, "(ADD ANSWER) Session is closed!", sz);
        break;
      }
      break;

    case ACTION_SESSION_DELETEANSWER:
      if (ses->state <= SESSION_NEW) {
        is_error = -1;
        strncpy(msg, "(DELETE ANSWER) Session is empty!", sz);
        break;
      }
      if (ses->state >= SESSION_CLOSED) {
        is_error = -1;
        strncpy(msg, "(DELETE ANSWER) Session is closed!", sz);
        break;
      }
      break;

    case ACTION_SESSION_ANALYSIS:
      if (ses->state < SESSION_FINISHED) {
        is_error = -1;
        strncpy(msg, "(ANALYSE) Session is not closed!", sz);
        break;
      }
      break;

    default:
      // pass
      break;
  }

  if (is_error) {
    LOG_WARNV("validate_session_action(): %s (action %d, state %d)",msg, action, ses->state);
  }

  return is_error;
}

/*
  Get len bytes from the cryptographically secure randomness source.
 */
int urandombytes(unsigned char *buf, size_t len) {
  int retVal = -1;

  do {
    if (!buf) {
      LOG_ERROR("buf is null");
      break;
    }

    static int urandomfd = -1;

    int tries = 0;

    if (urandomfd == -1) {
      for (tries = 0; tries < 4; ++tries) {
        urandomfd = open("/dev/urandom", O_RDONLY);
        if (urandomfd != -1) {
          break;
        }
        // LOG_WARN("failed to open /dev/urandom on try #%d, retrying", tries + 1);
        sleep(1);
      }

      if (urandomfd == -1) {
        LOG_ERROR("failed to open /dev/urandom, stop retrying");
        perror("open(/dev/urandom)");
        break;
      }
    }

    tries = 0;
    while (len > 0) {
      ssize_t i = read(urandomfd, buf, (len < 1048576) ? len : 1048576);
      if (i == -1) {

        if (++tries > 4) {
          LOG_ERROR("failed to read from /dev/urandom, even after retries");
          perror("read(/dev/urandom)");
          if (errno == EBADF) {
            LOG_ERROR("EBADF on /dev/urandom, resetting urandomfd to -1");
            urandomfd = -1;
          }
          break; // while
        } else {
          LOG_ERRORV("failed to read from /dev/urandom, retry %d, retrying",
                     tries);
        }

      } else {
        tries = 0;
        buf += i;
        len -= i;
      }
    } // endwhile

    if (len == 0) {
      retVal = 0;
    }
  } while (0);

  return retVal;
}

/*
  Generate a random (and hopefully unique) session ID.
  Our session IDs are modelled on RFC 4122 UUIDs, but are not exactly
  the same, largely out of convenience of implementation. There is nothing
  stopping us moving to full compliance as time permits.
 */
int random_session_id(char *session_id_out) {
  int retVal = 0;
  do {
    if (!session_id_out) {
      LOG_ERROR("session_id_out is NULL");
    }

    unsigned char randomness[32];
    if (urandombytes(randomness, 32)) {
      LOG_ERROR("get_randomness() failed");
    }

    sha1nfo s1;
    sha1_init(&s1);
    sha1_write(&s1, (char *)randomness, 32);
    unsigned char *hash = sha1_result(&s1);
    time_t t = time(0);
    unsigned int time_low = t & 0xffffffffU;
    unsigned int time_high = (t >> 32L);

    // Our session IDs look like RFC 4122 UUIDs, and are mostly the same,
    // except we put randomness at the front of the string, since we use
    // that to workout which directory each lives in, and we want them
    // evenly distributed from the outset, instead of directories being
    // filled up one at a time as time advances
    snprintf(
        session_id_out, 64,
        "%02x%02x%04x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x",
        // Here is that randomness at the start
        hash[6], hash[7], time_low & 0xffff, (time_high >> 16) & 0xffff,
        (0x0 << 12) + ((time_high >> 12) & 0xfff),
        // Also, we use the process ID as the clock sequence and related things field
        getpid() & 0xffff, hash[0], hash[1], hash[2], hash[3], hash[4],
        hash[5]);

  } while (0);
  return retVal;
}

/**
 * #363 save session meta data
 * This function leaves the file pointer open, regardless of success or fail
 */
int session_new_add_meta(struct session *ses, struct session_meta *meta) {
  // LOG_INFOV(
  //   "session_meta\n"
  //   "  |_ user: %s\n"
  //   "  |_ group: %s\n"
  //   "  |_ authority: %s\n"
  //   "  |_ provider: %d\n",
  //
  //   meta->user,
  //   meta->group,
  //   meta->authority,
  //   meta->provider
  // );

  int retVal = 0;
  struct answer *a = NULL;

  do {
    if (!ses || !ses->session_id) {
      LOG_ERROR("Add answer: Session structure or ses->session_id is NULL.");
    }
    if (!meta) {
        LOG_ERROR("cannot create session: session meta is NULL");
    }
    if (ses->state != SESSION_NEW) {
        LOG_ERRORV("cannot create session: session state is != SESSION_NEW (%d != %d)", ses->state, SESSION_NEW);
    }

    time_t now = time(NULL);
    long long stored = (long long) now;

    enum Headers { HEADER_USER, HEADER_GROUP, HEADER_AUTHORITY, HEADER_STATE, HEADER_MAX };

    for (int i = 0; i < HEADER_MAX; i++) {
      ses->answers[i] = calloc(sizeof(struct answer), 1);
      if (!ses->answers[i]) {
        LOG_ERRORV("calloc() of answer structure (header line %d) failed.", i);
      }
      ses->answers[i]->type = QTYPE_META;
      ses->answers[i]->stored = stored;
    }

    if (retVal) {
      break;
    }

    ses->answer_offset = HEADER_MAX;
    ses->answer_count = HEADER_MAX;

    // @user header
    a = ses->answers[HEADER_USER];
    a->uid = strdup("@user");
    if (meta->user) {
      a->text = strdup(meta->user);
    }

    // @group header
    a = ses->answers[HEADER_GROUP];
    a->uid = strdup("@group");
    if (meta->group) {
      a->text = strdup(meta->group);
    }

    // @authority header
    a = ses->answers[HEADER_AUTHORITY];
    a->uid = strdup("@authority");
    a->value = meta->provider;
    if (meta->authority) {
      a->text = strdup(meta->authority);
    }

    // @astate header
    a = ses->answers[HEADER_STATE];
    a->uid = strdup("@state");
    a->value = ses->state;
    a->time_begin = stored;

    LOG_INFOV("added %d header lines to new session '%s'", HEADER_MAX, ses->session_id);
  } while (0);

  return retVal;
}

void log_session_meta(struct session_meta *meta) {
  LOG_INFOV(
    "session_meta\n"
    "  |_ user: %s\n"
    "  |_ group: %s\n"
    "  |_ authority: %s\n"
    "  |_ provider: %d\n",

    meta->user,
    meta->group,
    meta->authority,
    meta->provider
  );
}

/**
 * #239
 * Make sure that the survey exists, and if the exact version of the survey
 * has not yet been recorded persistently, it makes a copy of it named as the
 * SHA1 hash of the survey definition.  This allows the survey definition in
 * surveys/survey_id/current to be freely modified, and any existing sessions
 * will continue to use the version of the survey that they were created under,
 * unless you modify the session record file to point to the new session.
 *
 * (If you have added or deleted questions, that might cause complicated problems,
 * so we don't do that automatically. As time allows, we can create a session
 * upgrade function, that checks if the current version of the form is logically
 * equivalent (same set of question UIDs and question types for all questions that
 * have been answered), and if so, updates the session to use the latest version.
 * This could even be implemented in load_session().)
 */
int create_survey_snapshot(char *survey_id, char *sha1, int sha1_len) {
  int retVal = 0;

  do {
    char survey_path[1024];
    char snapshot_path[1024];
    char temp_path[1024];
    int hash_len = HASH_LENGTH * 2 + 1; // @see sha1_file()

    if (!survey_id) {
      LOG_ERROR("survey_id is NULL");
    }
    if (!sha1) {
      LOG_ERROR("out is NULL");
    }
    if (sha1_len < hash_len) {
      LOG_ERROR("assigned length to sha1 too small");
    }
    if (validate_survey_id(survey_id)) {
      LOG_ERROR("Invalid survey id");
    }

    // get path to survey manifest
    if (generate_survey_path(survey_id, "current", survey_path, 1024)) {
      LOG_ERRORV("generate_survey_path() failed to build path for survey '%s'", survey_id);
    }
    if (access(survey_path, F_OK) == -1) {
      LOG_ERRORV("Survey '%s' does not exist", survey_id);
    }

    // Get sha1 hash of survey file
    if (sha1_file(survey_path, sha1)) {
      LOG_ERRORV("Could not hash survey specification file '%s'", survey_path);
    }
    if (generate_survey_path(survey_id, sha1, snapshot_path, 1024)) {
      LOG_ERRORV("generate_survey_path() failed to build path for snapshot '%s'", sha1);
    }

    if (access(snapshot_path, F_OK) != -1) {
        // a snapshot exists already, return
        break;
    }

    // No copy of the survey exists with the hash, so make one.
    // To avoid a race condition where the current survey form could be modified,
    // we copy it, and hash it as we go, and rename the copy based on the hash value
    // of what we read.

    // open input file (manifest)
    FILE *in = fopen(survey_path, "r");
    if (!in) {
        LOG_ERRORV("Could not read from survey manifest '%s'", survey_path);
    }

    // make a hopefully unique name for the temporary file
    char temp_name[1024];
    snprintf(temp_name, 1024, "temp.%lld.%d.%s", (long long)time(0), getpid(), sha1);

    if (generate_survey_path(survey_id, temp_name, temp_path, 1024)) {
        fclose(in);
        LOG_ERRORV("generate_path() failed to build temporary path for survey '%s'", temp_name);
    }

    FILE *c = fopen(temp_path, "w");
    if (!c) {
        fclose(in);
        LOG_ERRORV("Could not create temporary file '%s'", temp_path);
    }

    char buffer[8192];
    int count;
    sha1nfo s;
    sha1_init(&s);

    do {
        count = fread(buffer, 1, 8192, in);
        if (count < 0) {
            LOG_ERRORV("Error copying file '%s' to snapshot", survey_path);
        }

        if (count > 0) {
            sha1_write(&s, buffer, count);
            int wrote = fwrite(buffer, count, 1, c);
            if (wrote != 1) {
                fclose(in);
                fclose(c);
                unlink(temp_path);
                LOG_ERRORV("Failed to write all bytes during survey specification copy into '%s'", temp_path);
            }
        }

        if (retVal) {
            break;
        }
    } while (count > 0);

    fclose(in);
    fclose(c);

    // Rename temporary file
    if (rename(temp_path, snapshot_path)) {
    LOG_ERRORV("Could not rename survey specification copy to name of hash from '%s' to '%s'", temp_path, snapshot_path);
    }

    LOG_INFOV("Created new hashed survey specification file '%s' for survey '%s'", snapshot_path, survey_id);

  } while (0);

  return retVal;
}

/**
 * #239
 * creates a random session id and checks if session already exists in files.
 * In that (rare) case the generation is repeated.
 */
int create_session_id(char *session_id_out, int max_len) {
  int retVal = 0;

  do {
    if (!session_id_out) {
      LOG_ERROR("session_id is NULL");
    }

    char path[1024];

    // Generate new unique session ID
    int tries = 0;
    for (tries = 0; tries < 5; tries++) {
      if (random_session_id(session_id_out)) {
        LOG_ERROR("random_session_id() failed to generate new session_id");
      }

      if (generate_session_path(session_id_out, session_id_out, path, 1024)) {
        LOG_ERRORV("generate_path('%s') failed to build path for new session for session '%s'", session_id_out);
      }

      // Try again if session ID already exists
      if (access(path, F_OK) != -1) {
        session_id_out[0] = 0;
        continue;
      } else {
        break;
      }
    } // endfor

    if (!session_id_out[0]) {
      LOG_ERROR("Failed to generate unique session ID after several tries");
    }

  } while (0);

  return retVal;
}

/**
 * #239
 * Create a new session for a given session id.
 *  This creates the session record file, as well as
 * making sure that the survey exists, and if the exact version of the survey
 * has not yet been recorded persistently, it makes a copy of it named as the
 * SHA1 hash of the survey definition.  This allows the survey definition in
 * surveys/survey_id/current to be freely modified, and any existing sessions
 * will continue to use the version of the survey that they were created under,
 * unless you modify the session record file to point to the new session.
 * (If you have added or deleted questions, that might cause complicated problems,
 * so we don't do that automatically. As time allows, we can create a session
 * upgrade function, that checks if the current version of the form is logically
 * equivalent (same set of question UIDs and question types for all questions that
 * have been answered), and if so, updates the session to use the latest version.
 * This could even be implemented in load_session().)
 */
struct session *create_session(char *survey_id, char *session_id, struct session_meta *meta) {
  int retVal = 0;
  struct session *ses = NULL;

  do {
    if (!session_id) {
        LOG_ERROR("cannot create session: session id is NULL");
    }
    if (validate_session_id(session_id)) {
        LOG_ERROR("cannot create session: session id invalid");
    }
    if (!meta) {
        LOG_ERROR("cannot create session: session meta is NULL");
    }

    char survey_sha1[256];
    if(create_survey_snapshot(survey_id, survey_sha1, 256)) {
        LOG_ERRORV("cannot create session: failed to create sha1 snapshot of survey '%s'", survey_id);
    }

    // Make directories if they don't already exist
    char session_path[1024];

    // /<survey_home>/sessions/
    if (generate_path("sessions", session_path, 1024)) {
      LOG_ERROR("generate_path() failed to build path for <survey_home>/sessions");
    }
    mkdir(session_path, 0750);

    // <survey_home>/sessions/<suffix>/
    if (generate_session_path(session_id, NULL, session_path, 1024)) {
      LOG_ERRORV("generate_session_path('%s') failed to build path for <survey_home>/sessions/<suffix>", session_id);
    }
    mkdir(session_path, 0750);

    // <survey_home>/sessions/<suffix>/<session_id>
    if (generate_session_path(session_id, session_id, session_path, 1024)) {
      LOG_ERRORV("generate_path('%s') failed to build path for new session of <survey_home>/sessions/<suffix>/<session_id>", session_id);
    }

    // Write survey_id to new empty session.
    // This must take the form <survey id>/<sha1 hash of current version of survey>
    char sid[256];
    snprintf(sid, 256, "%s/%s", survey_id, survey_sha1);

    ses = calloc(sizeof(struct session), 1);
    if (!ses) {
      LOG_ERRORV("calloc(%d,1) failed when loading session '%s'", sizeof(struct session), session_id);
    }

    // #363 save session meta
    // #379 set session state to SESSION_NEW
    ses->survey_id = strdup(sid);
    ses->session_id = strdup(session_id);
    ses->state = SESSION_NEW;

    if (session_new_add_meta(ses, meta)) {
      free_session(ses);
      LOG_ERRORV("Cannot write session meta '%s'", session_path);
    }

    if (save_session(ses)) {
      free_session(ses);
      LOG_ERROR("save_session() failed");
    }


    LOG_INFOV("Created new session file '%s' for survey '%s'", session_path, survey_id);
  } while (0);

  if (retVal) {
    free_session(ses); // if not done before
    ses = NULL;
  }
  return ses;

}

/*
   The opposite of create_session().  It will complain if the session does not exist,
   or cannot be deleted.
*/
int delete_session(char *session_id) {
  int retVal = 0;

  do {
    char session_prefix[5];
    char session_path_suffix[1024];
    char session_path[1024];

    if (!session_id) {
      LOG_ERROR("session_id is NULL");
    }
    if (validate_session_id(session_id)) {
      LOG_ERRORV("Session ID '%s' is malformed", session_id);
    }

    for (int i = 0; i < 4; i++) {
      session_prefix[i] = session_id[i];
    }
    session_prefix[4] = 0;

    snprintf(session_path_suffix, 1024, "sessions/%s/%s", session_prefix,
             session_id);
    if (generate_path(session_path_suffix, session_path, 1024)) {
      LOG_ERRORV("generate_path('%s') failed to build path while deleting "
                 "session '%s'",
                 session_path_suffix, session_id);
    }

    // Try again if session ID already exists
    if (access(session_path, F_OK) == -1) {
      LOG_ERRORV("Session file '%s' does not exist", session_path);
    }

    if (unlink(session_path)) {
      LOG_ERRORV("unlink('%s') failed", session_path);
    }
    LOG_INFOV("Deleted session '%s'.", session_path);
  } while (0);

  return retVal;
}

void free_answer(struct answer *a) {
  if (!a) {
    return;
  }

  freez(a->uid);
  freez(a->text);
  // #72 unit field
  freez(a->unit);

  free(a);
  return;
}

void free_question(struct question *q) {
  if (!q) {
    return;
  }

  freez(q->uid);
  freez(q->question_text);
  freez(q->question_html);
  freez(q->default_value);
  freez(q->choices);
  // #72 unit field
  freez(q->unit);
  freez(q);
  return;
}

void free_session(struct session *s) {
  if (!s) {
    return;
  }

  freez(s->survey_id);
  freez(s->survey_description);
  freez(s->session_id);
  freez(s->consistency_hash); // #268

  for (int i = 0; i < s->question_count; i++) {
    free_question(s->questions[i]);
  }

  for (int i = 0; i < s->answer_count; i++) {
    free_answer(s->answers[i]);
  }

  s->answer_count = 0;
  s->question_count = 0;
  s->given_answer_count = 0; // #13 count given answers
  s->state = SESSION_NULL; // #379 reset state

  free(s);
  return;
}

// #363 free struct session meta
void free_session_meta(struct session_meta *m) {
  if (!m) {
    return;
  }

  freez(m->user);
  freez(m->group);
  freez(m->authority);

  free(m);
  return;
}

int dump_session(FILE *fp, struct session *ses) {
  int retVal = 0;
  int i;

  do {
    if (!fp) {
      LOG_ERROR("dump_session(): invalid file pointer.");
    }

    if (!ses) {
      fprintf(fp, "session { <NULL> }\n");
      break;
    }

    fprintf(
      fp,

      "session {\n"
      "  survey_id: \"%s\"\n"
      "  survey_description: \"%s\"\n"
      "  session_id: \"%s\"\n"
      "  consistency_hash: \"%s\"\n"
      "  nextquestions_flag: %u\n"
      "  answer_offset: %d\n"
      "  answer_count: %d\n"
      "  question_count: %d\n"
      "  given_answer_count: %d\n"
      "  state: %d\n",

      ses->survey_id,
      ses->survey_description,
      ses->session_id,
      (ses->consistency_hash) ? "<private>" : "(null)",
      ses->nextquestions_flag,

      ses->answer_offset,
      ses->answer_count,
      ses->question_count,
      ses->given_answer_count,
      ses->state
    );

    fprintf(fp , "  questions: [\n");
    for (i = 0; i < ses->question_count; i++) {
      fprintf(fp, "    \"%s\"%s\n", ses->questions[i]->uid, (i < ses->question_count - 1) ? ",": "");
    }
    fprintf(fp , "  ]\n");

    fprintf(fp , "  answers: [\n");
    for (i = 0; i < ses->answer_count; i++) {
      fprintf(fp, "    \"%s\"%s\n", ses->answers[i]->uid, (i < ses->answer_count - 1) ? ",": "");
    }
    fprintf(fp , "  ]\n");

  fprintf(fp , "}\n");
  } while (0);

  return retVal;
}

/*
  Load and deserialise the set of questions for the form corresponding to
  this session.
 */
int load_survey_questions(struct session *ses) {
  int retVal = 0;
  FILE *f = NULL;

  do {
    if (!ses) {
      LOG_ERROR("session structure is NULL");
    }
    if (!ses->survey_id) {
      LOG_ERROR("survey_id in session structure is NULL");
    }

    char survey_path_suffix[1024];
    char survey_path[1024];
    snprintf(survey_path_suffix, 1024, "surveys/%s", ses->survey_id);
    if (generate_path(survey_path_suffix, survey_path, 1024)) {
      LOG_ERRORV(
          "generate_path('%s') failed to build path for loading session '%s'",
          survey_path_suffix, ses->session_id);
    }

    f = fopen(survey_path, "r");
    if (!f) {
      LOG_ERRORV("Could not open survey file '%s'", survey_path);
    }
    char line[8192];

    // Check survey file format version
    line[0] = 0;
    if (!fgets(line, 8192, f)) {
      LOG_ERRORV("Failed to read survey file format version in survey "
                 "specification file '%s'",
                 survey_path);
    }
    if (!line[0]) {
      LOG_ERRORV("Failed to read survey file format version in survey "
                 "specification file '%s'",
                 survey_path);
    }

    int format_version = 0;
    int offset = 0;
    trim_crlf(line);
    if (sscanf(line, "version %d%n", &format_version, &offset) != 1) {
      LOG_ERRORV("Error parsing file format version in survey file '%s'",
                 survey_path);
    }

    if (offset < strlen(line)) {
      LOG_ERRORV(
          "Junk at end of version string in survey file '%s'. Line was '%s'",
          survey_path, line);
    }
    if (format_version < 1 || format_version > 2) {
      LOG_ERRORV("Unknown survey file format version in survey file '%s'",
                 survey_path);
    }

    // Get survey file description
    line[0] = 0;
    if (!fgets(line, 8192, f)) {
      LOG_ERRORV(
          "Failed to read survey description in survey specification file '%s'", survey_path);
    }
    if (!line[0]) {
      LOG_ERRORV(
          "Failed to read survey description in survey specification file '%s'", survey_path);
    }

    // Trim CR and LF chars from description
    trim_crlf(line);
    ses->survey_description = strdup(line);
    if (!ses->survey_description) {
      LOG_ERRORV("strdup('%s') failed when loading survey file '%s'", line, survey_path);
    }

    // version 1: Only allow generic implementation of next question picker if explicitly allowed
    ses->nextquestions_flag = NEXTQUESTIONS_FLAG_PYTHON;

    if (format_version > 1) {
      // Check for python directives
      line[0] = 0;
      if (!fgets(line, 8192, f)) {
        LOG_ERRORV("Failed to read survey description in survey specification "
                   "file '%s'",
                   survey_path);
      }

      if (!line[0]) {
        LOG_ERRORV("Failed to read survey description in survey specification "
                   "file '%s'",
                   survey_path);
      }

      // Trim CR and LF chars from description
      trim_crlf(line);
      if (!strcasecmp(line, "without python")) {
        ses->nextquestions_flag = NEXTQUESTIONS_FLAG_GENERIC;
      } else if (!strcasecmp(line, "with python")) {
        // do nothing, see above
        // We are using python, and have recorded a python library directory to add to the search path.
      } else {
        LOG_ERRORV("Missing <without python|with python> directive in survey "
                   "specification file '%s'",
                   survey_path);
      }
    }

    // Now read questions
    do {
      line[0] = 0;
      if (!fgets(line, 8192, f)) {
        break;
      }
      if (!line[0]) {
        break;
      }

      if (ses->question_count >= MAX_QUESTIONS) {
        LOG_ERRORV(
            "Too many questions in survey '%s' (increase MAX_QUESTIONS?)",
            survey_path);
      }

      // Remove end of line markers
      trim_crlf(line);

      // Skip blank lines
      if (!line[0]) {
        continue;
      }

      struct question *q = calloc(sizeof(struct question), 1);
      if (!q) {
        LOG_ERRORV("calloc(struct question) failed while loading survey "
                   "question list from '%s'",
                   survey_path);
      }

      if (deserialise_question(line, q)) {
        free_question(q);
        q = NULL;
        LOG_ERRORV("Error deserialising question '%s' in survey file '%s'",
                   line, survey_path);
      } else {
        ses->questions[ses->question_count++] = q;
      }

    } while (line[0]);

    fclose(f);
    f = NULL;
  } while (0);

  if (f) {
    fclose(f);
  }
  return retVal;
}

/*
  Load the specified session, and return the corresponding session structure.
  This will load not only the answers, but also the full set of questions.
 */
struct session *load_session(char *session_id) {
  int retVal = 0;

  int is_header = 1;
  struct session *ses = NULL;

  do {
    if (!session_id) {
      LOG_ERROR("session_id is NULL");
    }
    if (validate_session_id(session_id)) {
      LOG_ERRORV("validate_session_id('%s') failed", session_id);
    }

    char session_path[1024];

    if (generate_session_path(session_id, session_id, session_path, 1024)) {
      LOG_ERRORV("generate_session_path() failed to build path for loading session '%s'", session_id);
    }

    FILE *s = fopen(session_path, "r");
    if (!s) {
      LOG_ERRORV("Could not read from session file '%s'", session_path);
    }

    // Session file consists of:
    // First line = survey name in form <survey id>/<sha1 hash>
    // (this allows the survey to change without messing up sessions that are
    // in progress).
    // Subsequent lines:
    // @header answers: <serialised META answer> <add>
    // survey answers; <serialised MISC TYPES answer> <add|del>

    // Read survey ID line
    char survey_id[1024];
    survey_id[0] = 0;
    if (!fgets(survey_id, 1024, s)) {
      fclose(s);
      LOG_ERRORV("Could not read survey ID from session file '%s'", session_path);
    }
    if (!survey_id[0]) {
      fclose(s);
      LOG_ERRORV("Could not read survey ID from session file '%s'", session_path);
    }

    // Trim CR / LF characters from the end
    trim_crlf(survey_id);

    ses = calloc(sizeof(struct session), 1);
    if (!ses) {
      fclose(s);
      LOG_ERRORV("calloc(%d,1) failed when loading session '%s'", sizeof(struct session), session_id);
    }

    ses->survey_id = strdup(survey_id);
    ses->session_id = strdup(session_id);
    if (!ses->survey_id) {
      fclose(s);
      LOG_ERRORV("strdup(survey_id='%s') failed when loading session '%s'", survey_id, session_id);
    }
    if (!ses->session_id) {
      fclose(s);
      LOG_ERRORV("strdup(session_id) failed when loading session '%s'", session_id);
    }

    // Load survey
    if (load_survey_questions(ses)) {
      fclose(s);
      LOG_ERRORV("Failed to load questions from survey '%s'", survey_id);
    }
    if (!ses->question_count) {
      fclose(s);
      LOG_ERRORV("Failed to load questions from survey '%s', or survey  contains no questions", survey_id);
    }

    // Load answers from session file
    char line[65536];
    line[0] = 0;

    do {
      // Get next line with an answer in it
      line[0] = 0;
      if (!fgets(line, 65536, s)) {
        break;
      }
      if (!line[0]) {
        break;
      }

      int len = strlen(line);
      if (!len) {
        LOG_ERRORV("Empty line in session file '%s'", session_path);
      }
      if (line[len - 1] != '\n' && line[len - 1] != '\r') {
        LOG_ERRORV("Line too long in session file '%s' (limit = 64K)", session_path);
      }

      trim_crlf(line);

      // Add answer to list of answers
      if (ses->answer_count >= MAX_ANSWERS) {
        LOG_ERRORV( "Too many answers in session file '%s' (increase MAX_ANSWERS?)", session_path);
      }

      ses->answers[ses->answer_count] = calloc(sizeof(struct answer), 1);
      if (!ses->answers[ses->answer_count]) {
        LOG_ERRORV("calloc(%d,1) failed while reading session file '%s' ", sizeof(struct answer), session_path);
      }

      // #162 load complete answer, including protected fields
      if (deserialise_answer(line, ANSWER_SCOPE_FULL, ses->answers[ses->answer_count])) {
        LOG_ERRORV("Failed to deserialise answer '%s' from session file '%s'", line, session_path);
      }

      // #363 set header offset
      if (is_header) {
        if (ses->answers[ses->answer_count]->uid[0] != '@') {
          is_header = 0;
        }
      }

      // #13 count given answers
      if (is_given_answer(ses->answers[ses->answer_count])) {
        ses->given_answer_count++;
      }

      ses->answer_count++;

      // #363 set header offset
      if (is_header) {
        ses->answer_offset = ses->answer_count;
      }

    } while (line[0]);

    // #379 record current state of loaded sesion
    struct answer *current_state = session_get_header("@state", ses);
    if (!current_state) {
      fclose(s);
      LOG_ERRORV("Could not find state header for session '%s'", session_id); // #268, changed to error
    }
    ses->state = current_state->value;

    // #268 finally generate current sha1 checksum
    if (session_generate_consistency_hash(ses)) {
      fclose(s);
      LOG_ERRORV("failed to generate consistency hash for session '%s'", session_id);
    }

    if (retVal) {
      fclose(s);
      free_session(ses);
      ses = NULL;
      break;
    }

    fclose(s);

  } while (0);

  if (retVal) {
    ses = NULL;
  }
  return ses;
}

/*
  Save the provided session, including all provided answers.
  Questions are not saved, as they are part of the survey, i.e., form specification.
 */
int save_session(struct session *s) {
  int retVal = 0;
  FILE *o = NULL;

  do {
    if (!s) {
      LOG_ERROR("session structure is NULL");
    }
    if (!s->session_id) {
      LOG_ERROR("s->session_id is NULL");
    }
    if (validate_session_id(s->session_id)) {
      LOG_ERRORV("validate_session_id('%s') failed", s->session_id);
    }

    // update header with current state of loaded and processed session (#379)
    struct answer *header = session_get_header("@state", s);
    if (!header) {
      LOG_ERROR("Could not find state header for session!"); // don't break here fall through
    }

    if (s->state != header->value) {
      int old = header->value;
      header->value = s->state;
      // record time of session closure, note: create_session (SESSION_NEW) writes timestamp into `time_begin` field
      header->time_end = (s->state == SESSION_CLOSED) ? (long long)time(NULL) : 0;
      header->stored = (long long)time(NULL);
      LOG_INFOV("pre-save: Updated state header, old state: %d, new state: %d", old, s->state);
    }

    // write session

    char session_path[1024];
    char session_path_final[1024];
    char session_path_suffix[1024];
    char session_prefix[5];

    for (int i = 0; i < 4; i++) {
      session_prefix[i] = s->session_id[i];
    }
    session_prefix[4] = 0;

    snprintf(session_path_suffix, 1024, "sessions/%s/write.%s", session_prefix,
             s->session_id);
    if (generate_path(session_path_suffix, session_path, 1024)) {
      LOG_ERRORV(
          "generate_path('%s') failed to build path for loading session '%s'",
          session_path_suffix, s->session_id);
    }

    o = fopen(session_path, "w");
    if (!o) {
      LOG_ERRORV("Could not create or open session file '%s' for write", session_path);
    }
    fprintf(o, "%s\n", s->survey_id);

    for (int i = 0; i < s->answer_count; i++) {
      char line[65536];
      if (serialise_answer(s->answers[i], ANSWER_SCOPE_FULL, line, 65536)) {
        LOG_ERRORV("Could not serialise answer for question '%s' for session "
                   "'%s'.  Text field too long?",
                   s->answers[i]->uid, s->session_id);
      }
      fprintf(o, "%s\n", line);
    }
    fclose(o);
    o = NULL;

    snprintf(session_path_suffix, 1024, "sessions/%s/%s", session_prefix, s->session_id);

    if (generate_path(session_path_suffix, session_path_final, 1024)) {
      LOG_ERRORV(
          "generate_path('%s') failed to build path for loading session '%s'",
          session_path_suffix, s->session_id);
    }
    if (rename(session_path, session_path_final)) {
      LOG_ERRORV("rename('%s','%s') failed when updating file for session '%s' "
                 "(errno=%d)",
                 session_path, session_path_final, s->session_id, errno);
    }

    // #268 finally update current sha1 checksum
    if (session_generate_consistency_hash(s)) {
      LOG_ERRORV("failed to generate consistency hash for session '%s'", s->session_id);
    }

    LOG_INFOV("Updated session file '%s'.", session_path_final);
  } while (0);

  if (o) {
    fclose(o);
  }
  return retVal;
}

/**
 * query a session for a question uid
 * returns index position in session or -1 if question was not found
 * #462
 */
int session_get_question_index(char *uid, struct session *ses) {
  if (!uid) {
    LOG_WARNV("session_get_question_index(): search uid is null", 0);
    return -1;
  }
  if (!ses) {
    LOG_WARNV("session_get_question_index(): session is null", 0);
    return -1;
  }

  for (int i = 0; i < ses->question_count; i++) {
    if (!strcmp(ses->questions[i]->uid, uid)) {
      return i;
    }
  }
  return -1;
}

/**
 * query a session for a question uid
 */
struct question *session_get_question(char *uid, struct session *ses) {
  if (!uid) {
    LOG_WARNV("session_get_question(): search uid is null", 0);
    return NULL;
  }
  if (!ses) {
    LOG_WARNV("session_get_question(): session is null", 0);
    return NULL;
  }

  for (int i = 0; i < ses->question_count; i++) {
    if (!strcmp(ses->questions[i]->uid, uid)) {
      return ses->questions[i];
    }
  }
  return NULL;
}

/**
 * query a session for an answer uid (excluding header answers)
 */
struct answer *session_get_answer(char *uid, struct session *ses) {
  if (!uid) {
    LOG_WARNV("session_get_answer(): search uid is null", 0);
    return NULL;
  }
  if (!ses) {
    LOG_WARNV("session_get_answer(): session is null", 0);
    return NULL;
  }

  for (int i = ses->answer_offset; i < ses->answer_count; i++) {
    if (!strcmp(ses->answers[i]->uid, uid)) {
      return ses->answers[i];
    }
  }
  return NULL;
}

/**
 * query a session for an answer uid (excluding header answers)
 * returns index position in session or -1 if answer was not found
 */
int session_get_answer_index(char *uid, struct session *ses) {
  if (!uid) {
    LOG_WARNV("session_get_answer(): search uid is null", 0);
    return -1;
  }
  if (!ses) {
    LOG_WARNV("session_get_answer(): session is null", 0);
    return -1;
  }

  for (int i = ses->answer_offset; i < ses->answer_count; i++) {
    if (!strcmp(ses->answers[i]->uid, uid)) {
      return i;
    }
  }
  return -1;
}

/**
 * find the last given answer (conditions: no system answer && not deleted) within a session
 */
struct answer *session_get_last_given_answer(struct session *ses) {
  if (!ses) {
    LOG_WARNV("session_get_answer(): session is null", 0);
    return NULL;
  }

  for (int i = ses->answer_count - 1; i >= ses->answer_offset; i--) {
    if (is_given_answer(ses->answers[i])) {
      return ses->answers[i];
    }
  }
  return NULL;
}

/**
 * #363, query a header answer
 */
struct answer *session_get_header(char *uid, struct session *ses) {
  if (!uid) {
    LOG_WARNV("session_get_header(): search uid is null", 0);
    return NULL;
  }
  if (!ses) {
    LOG_WARNV("session_get_header(): session is null", 0);
    return NULL;
  }

  for (int i = 0; i < ses->answer_offset; i++) {
    if (!strcmp(ses->answers[i]->uid, uid)) {
      return ses->answers[i];
    }
  }
  return NULL;
}

/**
 * set a string value of the current answer based on it's type
 * #384, #425
 */
int answer_set_value_raw(struct answer *a, char *in) {
  int retVal = 0;

  do {
      if(!a) {
        LOG_ERROR("answer is null!");
      }
      if(!in) {
        LOG_ERROR("input is null!");
      }
      if(is_system_answer(a)) {
        LOG_ERRORV("answer '%s' is a sytem answer", a->uid);
      }

      long long lat;
      long long lon;
      long long time_begin;
      long long time_end;
      long long value;
      int ival;

      switch (a->type) {
        // value
        case QTYPE_INT:
          if(deserialise_int(in, &ival)) {
            LOG_ERRORV("deserialise_int() for answer '%' failed", a->uid);
          }
          a->value = ival;
        break;

        case QTYPE_FIXEDPOINT:
        case QTYPE_DURATION24:
          if(deserialise_longlong(in, &value)) {
            LOG_ERRORV("deserialise_longlong() for answer '%' failed", a->uid);
          }
          a->value = value;
          break;

        // text
        case QTYPE_TEXT:
        case QTYPE_CHECKBOX:
        case QTYPE_HIDDEN:
        case QTYPE_TEXTAREA:
        case QTYPE_EMAIL:
        case QTYPE_SINGLECHOICE:
        case QTYPE_SINGLESELECT:
        case QTYPE_DIALOG_DATA_CRAWLER:
        case QTYPE_UUID:
        case QTYPE_SHA1_HASH:

        // text (comma separated)
        case QTYPE_MULTICHOICE:
        case QTYPE_MULTISELECT:
        case QTYPE_FIXEDPOINT_SEQUENCE:
        case QTYPE_DAYTIME_SEQUENCE:
        case QTYPE_DATETIME_SEQUENCE:
          // avoid memory conflicts
          if(a->text != NULL) {
            LOG_ERRORV("answer->text must be a NULL pointer for answer (answer '%s')", a->uid);
          }
          if(deserialise_string(in, &(a->text))) {
            LOG_ERRORV("deserialise_string() for answer '%' failed", a->uid);
          }
          break;

        // [lat,lon]
        case QTYPE_LATLON:
          if (serialiser_count_columns(',', in, 65536) != 2) {
            LOG_ERRORV("(count) input '%s' for LATLON type must be a comma separated string of two floats (answer '%s')", in, a->uid);
          }
          if (sscanf(in, "%lld,%lld", &lat, &lon) != 2) {
            LOG_ERRORV("(scsanf) input '%s' for LATLON type must be a comma separated string of two floats (answer '%s')", in, a->uid);
          }
          a->lat = lat;
          a->lon = lon;
          break;

        // time_begin
        case QTYPE_DATETIME:
        case QTYPE_DAYTIME:
          if (deserialise_longlong(in, &time_begin)) {
            LOG_ERRORV("deserialise_longlong() for answer '%' failed", a->uid);
          }
          a->time_begin = time_begin;
          break;

        // [time_begin,time_end]
        case QTYPE_TIMERANGE:
          if (serialiser_count_columns(',', in, 65536) != 2) {
            LOG_ERRORV("(count) input '%s' for TIMERANGE type must be a comma separated string of two floats (answer '%s')", in, a->uid);
          }
          if (sscanf(in, "%lld,%lld", &time_begin, &time_end) != 2) {
            LOG_ERRORV("input '%s' for TIMERANGE type must be a comma separated string of two floats (answer '%s')", in, a->uid);
          }
          a->time_begin = time_begin;
          a->time_end = time_end;
          break;

        default:
          LOG_ERRORV("Unknown question type '%d' for answer '%s'", a->type, a->uid);
          break;
      }// switch

      if (retVal) {
        break;
      }

    } while (0);

    return retVal;
}

/**
 * get a string value of the current answer based on it's type
 * #384
 */
int answer_get_value_raw(struct answer *a, char *out, size_t sz) {
  int retVal = 0;

  do {
      if(!a) {
        LOG_ERROR("answer is null!");
      }
      if(is_system_answer(a)) {
        LOG_ERRORV("answer '%s' is a sytem answer", a->uid);
      }

      switch (a->type) {
        // value
        case QTYPE_INT:
        case QTYPE_FIXEDPOINT:
        case QTYPE_DURATION24:
          snprintf(out, 8192, "%lld", a->value);
          break;

        // text
        case QTYPE_TEXT:
        case QTYPE_CHECKBOX:
        case QTYPE_HIDDEN:
        case QTYPE_TEXTAREA:
        case QTYPE_EMAIL:
        case QTYPE_SINGLECHOICE:
        case QTYPE_SINGLESELECT:
        case QTYPE_DIALOG_DATA_CRAWLER:
        case QTYPE_UUID:

        // text (comma separated)
        case QTYPE_MULTICHOICE:
        case QTYPE_MULTISELECT:
        case QTYPE_FIXEDPOINT_SEQUENCE:
        case QTYPE_DAYTIME_SEQUENCE:
        case QTYPE_DATETIME_SEQUENCE:
          snprintf(out, 8192, "%s", a->text);
          break;

        // [lat,lon]
        case QTYPE_LATLON:
          snprintf(out, 8192, "%lld,%lld", a->lat, a->lon);
          break;

        // time_begin
        case QTYPE_DATETIME:
        case QTYPE_DAYTIME:
          snprintf(out, 8192, "%lld", a->time_begin);
          break;

        // [time_begin,time_end]
        case QTYPE_TIMERANGE:
          snprintf(out, 8192, "%lld,%lld", a->time_begin, a->time_end);
          break;

        default:
          out[0] = 0;
          LOG_ERRORV("Unknown question type '%d' for answer '%s'", a->type, a->uid);
          break;
      }// switch
    } while (0);

    return retVal;
}

/**
 * Applies certain transformations to incoming answers of a special type before they are saved into a session
 * #237
 */
int pre_add_answer_special_transformations(struct answer *an) {
  int retVal = 0;
  do {
    if (!an || !an->uid) {
      LOG_ERROR("Answer is null");
    }

    if (is_system_answer(an)) {
      LOG_ERRORV("No special transformation allowed for system answer '%s'", an->uid);
    }

    // QTYPE_SHA1_HASH: replace answer->text with a hash of that value
    if (an->type == QTYPE_SHA1_HASH) {

      char hash[HASHSTRING_LENGTH];
      if(sha1_string(an->text, hash)) {
        LOG_ERRORV("sha1_string(answer->text) failed for answer '%s'", an->uid);
      }

      freez(an->text);
      an->text = strdup(hash);

      if (!an->text) {
        LOG_ERRORV("strdup(hash) failed for answer '%s' (out of memory)", an->uid);
      }

    }

  } while (0);
  return retVal;
}

/**
 * Marks an answer as deleted, skips when answer was already flagged as deleted (#407, #13)
 * This function also mark system answers!
 * returns 1 if delete flag was set and 0 if deletion was skipped
 */
int answer_mark_as_deleted(struct answer *ans) {
    if(!ans) {
      LOG_WARNV("answer_mark_as_deleted(): answer is null", 0);
      return 0;
    }

    // #363, system answers (QTYPE_META or @uid) answers cannot be deleted
    if (is_system_answer(ans)) {
      LOG_WARNV("answer_mark_as_deleted(): answer '%s' is a system answer", ans->uid);
      return 0;
    }

    if (ans->flags & ANSWER_DELETED) {
      return 0;
    }

    ans->flags |= ANSWER_DELETED;
    ans->stored = (long long)time(NULL);
    LOG_INFOV("Deleted from session answer '%s'.", ans->uid);
    return 1;
}

struct answer *copy_answer(struct answer *aa) {
  int retVal = 0;
  struct answer *a = NULL;
  do {
    // Duplicate aa into a, so that we don't put pointers to structures that are on the
    // stack into our list.
    a = malloc(sizeof(struct answer));
    if (!a) {
      LOG_ERROR("malloc() of struct answer failed.");
    }
    bcopy(aa, a, sizeof(struct answer));

    if (a->uid) {
      a->uid = strdup(a->uid);
      if (!a->uid) {
        LOG_ERROR("Could not copy a->uid");
      }
    }

    if (a->text) {
      a->text = strdup(a->text);
      if (!a->text) {
        LOG_ERROR("Could not copy a->text");
      }
    }

    if (a->unit) {
      a->unit = strdup(a->unit);
      if (!a->unit) {
        LOG_ERROR("Could not copy a->unit");
      }
    }
  } while (0);

  if (retVal) {

    if (a) {
      free_answer(a);
    }
    a = NULL;
    return NULL;

  } else {
    return a;
  }
}

/**
 * Creates a copy of a question.
 * The default_value may be overwritten. The copy needs to be freed independently
 */
struct question *copy_question(struct question *qq, char *default_value) {
  int retVal = 0;
  struct question *q = NULL;
  do {
    // Duplicate aa into a, so that we don't put pointers to structures that are on the
    // stack into our list.
    q = malloc(sizeof(struct question));
    if (!q) {
      LOG_ERROR("malloc() of struct question failed.");
    }
    bcopy(qq, q, sizeof(struct question));

    if (q->uid) {
      q->uid = strdup(q->uid);
      if (!q->uid) {
        LOG_ERROR("Could not copy uid");
      }
    }

    if (q->question_text) {
      q->question_text = strdup(q->question_text);
      if (!q->question_text) {
        LOG_ERROR("Could not copy question_text");
      }
    }

    if (q->question_html) {
      q->question_html = strdup(q->question_html);
      if (!q->question_html) {
        LOG_ERROR("Could not copy question_html");
      }
    }

    // #213 arg default_value (if having content) has predecence over question->default_value
    char *dv = (default_value && strlen(default_value)) ? default_value : q->default_value;
    if (dv) {
      q->default_value = strdup(dv);
      if (!q->default_value) {
        LOG_ERROR("Could not copy default_value");
      }
    }

    if (q->choices) {
      q->choices = strdup(q->choices);
      if (!q->choices) {
        LOG_ERROR("Could not copy choices");
      }
    }

    if (q->unit) {
      q->unit = strdup(q->unit);
      if (!q->unit) {
        LOG_ERROR("Could not copy unit");
      }
    }
  } while (0);

  if (retVal) {

    if (q) {
      free_question(q);
    }
    q = NULL;
    return NULL;

  } else {
    return q;
  }
}

/**
 * Add the provided answer to the set of answers in the provided session.
 * If another answer exists for the same question, it will trigger an error.
 *
 * returns 1 (affected count) on success or -1 on error. (since #445)
 */
int session_add_answer(struct session *ses, struct answer *a) {
  int retVal = 0;
  int undeleted = 0;

  do {
    // Add answer to list of answers
    if (!ses || !ses->session_id) {
      LOG_ERROR("Add answer: Session structure or ses->session_id is NULL.");
    }
    if (!a || !a->uid) {
      LOG_ERRORV("Add answer: Answer structure or a->uid is null, session '%s'", ses->session_id);
    }
    // #363, system answers (QTYPE_META or @uid) answers cannot be added
    if (is_system_answer(a)) {
      LOG_ERRORV("Add answer: Invalid request to remove private SYSTEM answer '%s' from session '%s'", a->uid, ses->session_id);
    }

    // Don't allow answers to questions that don't exist
    struct question *qn = session_get_question(a->uid, ses);
    if (!qn) {
      LOG_ERRORV("There is no such question '%s'", a->uid);
    }

    // #162 add/update stored timestamp,#358 set question type (for both, adding and deleting)
    // #448 remove overwriting of question 'unit' by answers: merge unit into session answer
    a->stored = (long long)time(NULL);
    a->type = qn->type;
    if (a->unit) { // should not be the case, except maybe in tests
      freez(a->unit);
    }
    a->unit = strdup(qn->unit);

    if (pre_add_answer_special_transformations(a)) {
      LOG_ERRORV("pre-addanswer hook failed for answer '%s', session '%s'", a->uid, ses->session_id);
    }

    // by default, append answer
    int index = ses->answer_count;

    // Don't allow multiple answers to the same question
    // Answer exists, but was deleted, so we can just update the values
    // by replacing the answer structure. #186
    int exists = session_get_answer_index(a->uid, ses);
    if (exists >= 0) {
        if (ses->answers[exists]->flags & ANSWER_DELETED) {
          free(ses->answers[exists]);
          index = exists;
          undeleted = 1;
          LOG_INFOV("Question '%s' has been deleted in session '%s'.", a->uid, ses->session_id);
        } else {
          LOG_ERRORV("Question '%s' has already been answered in session '%s'. Delete old answer before adding a new one", a->uid, ses->session_id);
        }
    }

    if (ses->answer_count >= MAX_ANSWERS) {
      LOG_ERRORV("Too many answers in session '%s' (increase MAX_ANSWERS?)", ses->session_id);
    }

    ses->answers[index] = copy_answer(a);

    // #186 Don't append answer if we are undeleting it.
    if (!undeleted) {
      ses->answer_count++;
    }

    // #13 update count given answers, system answers are already excluded
    ses->given_answer_count++;
    LOG_INFOV("Added to session '%s' answer '%s'.", ses->session_id, a->uid);

    // #379 set session state to 'open' get_next_questions might later progrress this to 'finished'
    ses->state = SESSION_OPEN;

  } while (0);

  if (retVal) {
    return retVal;
  }

  return 1;
}

/**
 * Delete any and all answers to a given question from the provided session structure.
 * It is not an error if there were no matching answers to delete
 * #407, #13, improve counting (only answers where deletion flag changed), make use of helper functions
 * #462 next question consistency: all following questions are now deleted
 *
 * returns number of deleted answers or -1 on (arg validation) error. It is not an error if there were no matching answers to delete
*/
int session_delete_answer(struct session *ses, char *uid) {
  int retVal = 0;
  int deletions = 0;

  do {
    if (!ses || !ses->session_id) {
      LOG_ERROR("Delete Answer (by uid): Session structure or ses->session_id is NULL");
    }
    if (!uid) {
      LOG_ERRORV("Delete Answer (by uid): Asked to remove answers to null question UID from session '%s'", ses->session_id);
    }
    // #363, system answers (QTYPE_META or @uid) answers cannot be deleted, validate uid even before checking if the answer exists and flag error to callee
    if (uid[0] == '@') {
      LOG_ERRORV("Delete Answer (by uid): Invalid uid '%s' provided (session '%s')", uid, ses->session_id);
    }

    int index = session_get_answer_index(uid, ses);
    if (index < 0) {
      LOG_WARNV("Delete Answer (by uid): answer '%s' not found in (session '%s')", uid, ses->session_id);
      break; // leave with 0
    }

    deletions = answer_mark_as_deleted(ses->answers[index]);
    if(!deletions) {
      // nothing deleted, either it was deleted before or is system answer -  abort here
      break;
    }

    // Mark all following answers deleted
    for (int j = index + 1; j < ses->answer_count; j++) {
      deletions += answer_mark_as_deleted(ses->answers[j]);
    }

  } while (0);

  if (!retVal) {
    ses->given_answer_count -= deletions;
    retVal = deletions;
  }

  return retVal;
}

/**
 * Store a data file close to the session answers
 * Should the logging fail, the process will not break and issue warnings to the log
 * Note that the session needed to be created previously, otherwise the session dir would not be accessible!
 */
int session_add_datafile(char *session_id, char *filename_suffix,
                         const char *data) {

  int retVal = 0;
  FILE *fp = NULL;

  do {
    char session_path[1024];
    char session_path_suffix[1024];
    char session_prefix[5];

    for (int i = 0; i < 4; i++) {
      session_prefix[i] = session_id[i];
    }
    session_prefix[4] = 0;

    snprintf(session_path_suffix, 1024, "sessions/%s/%s.%s", session_prefix,
             session_id, filename_suffix);
    if (generate_path(session_path_suffix, session_path, 1024)) {
      LOG_ERRORV("generate_path('%s') failed to build path for udata file, "
                 "session '%s'",
                 session_path_suffix, session_id);
    }

    fp = fopen(session_path, "w");
    if (!fp) {
      LOG_ERRORV("Could not create or open data file '%s' for write",
                 session_path);
    }

    fprintf(fp, "%s\n", data);
    fclose(fp);
    fp = NULL;
  } while (0);

  if (fp) {
    fclose(fp);
  }

  return retVal;
}

/**
 * find the last given answer (conditions: no system answer && not deleted) within a session
 * #268
 */
int session_generate_consistency_hash(struct session *ses) {
  int retVal = 0;

  do {
    if (!ses || !ses->session_id) {
      LOG_ERROR("Add answer: Session structure or ses->session_id is NULL.");
    }

    char line[65536];
    snprintf(line, 256, "%s%d", ses->session_id, ses->state);

    sha1nfo info;
    sha1_init(&info);
    sha1_write(&info, line, strlen(line));

    struct answer *last = session_get_last_given_answer(ses);
    if (last) {
      if (serialise_answer(last, ANSWER_SCOPE_CHECKSUM, line, 65536)) {
        LOG_ERRORV("session_generate_cecksum(): serialise_answer() failed for field '%s'", last->uid);
      }
      sha1_write(&info, line, strlen(line));
    }

    // #268 purge and generate new sha1 checksum
    freez(ses->consistency_hash);
    ses->consistency_hash = NULL;
    char hash[HASHSTRING_LENGTH + 1];
    if (sha1_hash(&info, hash)) {
      LOG_ERRORV("session_generate_cecksum(): generating hash failed for session '%s'", ses->session_id);
    }
    ses->consistency_hash = strdup(hash);

  } while(0);

  return retVal;
}
