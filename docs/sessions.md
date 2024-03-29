# Sessions

## Example session

This is an example of a session with the id `381544dc-0000-0000-0d04-01123f06e306`.

It contains one answer out of two defined questions.
The session file is located in `<project_root>/backend/sessions/3815/381544dc-0000-0000-0d04-01123f06e306`.
Our session id stored as the file name.

```
foo/b884bcb19954b245d8be53db2b266c4798dc742d
@user:META::0:0:0:0:0:0:0::0:1595557084
@group:META::0:0:0:0:0:0:0::0:1595557084
@authority:META:127.0.0.1(8099):1:0:0:0:0:0:0::0:1595557084
@state:META::2:0:0:1595557084:0:0:0::0:1595559999
question1:TEXT:Hello World 1:0:0:0:0:0:0:0::0:1595559999
```

### Linked survey definition

The matching survey definition for our session is located in: `<project_root>/backend/surveys/foo/current`

```
version 2
Silly test survey updated
without python
question1:Question 1::TEXT:0::-1:-1:0:0::
question2:Question 2::TEXT:0::-1:-1:0:0::
```

When a session is created the backend saves a *snapshot copy* of the "current" `current` file, using the file's SHA1 hash as file name (if not alredy exists).

```bash
cp <project_root>/backend/surveys/foo/current <project_root>/backend/surveys/foo/b884bcb19954b245d8be53db2b266c4798dc742d
```

For the remainder of the session this copy is being loaded and matched against the session answers.

## Session structure

```bash
                       ------<survey_name>
survey definition copy | foo/b884bcb19954b245d8be53db2b266c4798dc742d
                       ------
                       | @user:META::0:0:0:0:0:0:0::0:1595557084
header                 | @group:META::0:0:0:0:0:0:0::0:1595557084
                       | @authority:META:127.0.0.1(8099):1:0:0:0:0:0:0::0:1595557084
                       | @state:META::1:0:0:1595557084:0:0:0::0:1595557084
                       ------
session body           | question1:TEXT:Hello World 1:0:0:0:0:0:0:0::0:1595558084
                       | question2:TEXT:Hello World 2:0:0:0:0:0:0:0::0:1595559084
                       ------
```

Sections:

1) survey definition: survey defintion copy, format `<survey_name>/<survey_sha>`.
2) session header: sessioin meta data (set by backend): see below
3) session body: contains the *user* answers given to the questions defined for our example survey

## Session header

First recall the the fields of session answer, see also [data-serialization](data-serialisation.md):

```csv
<uid>:<text>:<value>:<lat>:<lon>:<time_begin>:<time_end>:<time_zone_delta>:<dst_delta>:<unit>:<flag>:<stored>
```

Header META answers

* **@user**: records username (if supplied) in column `text`
* **@group**: records groupname (if supplied) in column `text`
* **@authority**: records authority source in column `<text>` (if applicable), authority type is recorded in column `<value>` (see `struct session_meta`)
* **@state** records:
  - `<text>`: a comma-separated list of question ids returned by the last [next_questions response](next-questions-response.md)
  - `<value>`: current session state in column `<value>` (see `enum session_state`)
  - `<time_begin>` time of session creation
  - `<time_end>` The time a sesson was closed is recorded in `time_end`

# System answers

System answers are not linked to a survey question defintion and are not exposed to frontend and questionlogic controllers.

## Session `META` answers

Sessions can contain Meta answers.

example session line: `internalData:META:some text:12345:0:0:0:0:0:0::0:1595559714`

* Meta answers have no corresponding question definitions and are set by the backend.
* Meta answers report internal data and can occupy any data field in the answer. Since there is no question the uid is the identifier for parsing out the data.
* Meta answers are not visible to the frontend or next_question controllers. They cannot be deleted or edited by these components either.

## Session `@header` answers

On creation sessions report some internal data into header `META` fields.
The fields are: `@user`, `@group`, `@authority`, `@closed` (in this exact oerder)

* @header answers have no corresponding question definitions and are set by the backend *at the start of a session*
* @header answers are not visible to the frontend or next_question controllers. They cannot be deleted or edited by these components either.

# Session Lifetime (example)

In this chapter we are going step by step thought the life of a session.

Recall our example survey:

```
version 2
Silly test survey updated
without python
question1:Question 1::TEXT:0::-1:-1:0:0::
question2:Question 2::TEXT:0::-1:-1:0:0::
```

## 1. New session (`SESSION_NEW`)

* `GET /session?surveyid=foo`

A new 'empty' session is created, authority and session state are recorded into the header. Our example session id is `381544dc-0000-0000-0d04-01123f06e306`

```
foo/b884bcb19954b245d8be53db2b266c4798dc742d
@user:META::0:0:0:0:0:0:0::0:1596163402
@group:META::0:0:0:0:0:0:0::0:1596163402
@authority:META:127.0.0.1(8099):1:0:0:0:0:0:0::0:1596163402
@state:META:question1:1:0:0:1596163402:0:0:0::0:1596163402
```

The `<value>` field in `@state` header is set to `SESSION_NEW`, the session creation is recorded in field `time_begin`.

An internal next_question query is performed (since #484) and the next_questions uid are stored in  the `<text>` field.

Notes:

 * Requesting next question within a new session will not change the state. *GET /questions?sessionid=381544dc-0000-0000-0d04-01123f06e306*

## 2. Open Session (`SESSION_OPEN`)

* `POST /answers?sessionid=381544dc-0000-0000-0d04-01123f06e306&question1=Answer+1`

We assume a simple gneric question logic where just the next question  (`question2`) is returned by our next_questions response
Answering the first question records the answer into the session and progresses the session to "open".

```
foo/b884bcb19954b245d8be53db2b266c4798dc742d
@user:META::0:0:0:0:0:0:0::0:1596163402
@group:META::0:0:0:0:0:0:0::0:1596163402
@authority:META:127.0.0.1(8099):1:0:0:0:0:0:0::0:1596163402
@state:META:question2:2:0:0:1596163402:0:0:0::0:1596163402
question1:TEXT:Answer 1:0:0:0:0:0:0:0::0:1596163402
```

The `<value>` field in `@state` header has progressed to `SESSION_OPEN`
The `<text>` field in `@state` contains the expected question id of the next answer

Notes:

* Deleting any or all answers within an open session will not change the state. * DELETE /answer?sessionid=sessionid=381544dc-0000-0000-0d04-01123f06e306&questionid=question1*

## 3. Finish Session (`SESSION_FINISHED`)

* `POST /answers?sessionid=381544dc-0000-0000-0d04-01123f06e306&question2=Answer+2`

We are answering our last question. This progresses the session to "closed".

```
foo/b884bcb19954b245d8be53db2b266c4798dc742d
@user:META::0:0:0:0:0:0:0::0:1596163402
@group:META::0:0:0:0:0:0:0::0:1596163402
@authority:META:127.0.0.1(8099):1:0:0:0:0:0:0::0:1596163402
@state:META::3:0:0:1596163402:0:0:0::0:1596163402
question1:TEXT:Answer 1:0:0:0:0:0:0:0::0:1596163402
question2:TEXT:Answer 2:0:0:0:0:0:0:0::0:1596163402

```

The `<value>` field in `@state` header has progressed to `SESSION_FINISHED`.
Finished sessions can still be accessed by "next questions" and delete requests

The `<text>` field in `@state` is empty

### 3.1 Reopen Session (`SESSION_OPEN`)

* `DELETE /answers?sessionid=sessionid=381544dc-0000-0000-0d04-01123f06e306&questionid=question2`

We 'delete' our last answer. This regresses the state back to `SESSION_OPEN`

```
foo/b884bcb19954b245d8be53db2b266c4798dc742d
@user:META::0:0:0:0:0:0:0::0:1596163402
@group:META::0:0:0:0:0:0:0::0:1596163402
@authority:META:127.0.0.1(8099):1:0:0:0:0:0:0::0:1596163402
@state:META:question2:2:0:0:1596163402:0:0:0::0:1596163402
question1:TEXT:Answer 1:0:0:0:0:0:0:0::0:1596163402
question2:TEXT:Answer 2:0:0:0:0:0:0:0::1:1596163402
```

The `<value>` field in `@state` header has progressed to `SESSION_OPEN`

Note that the old answer to `question2` still exists in the record however the `<flags>` field is moved to `ANSWER_DELETED` (byte flag)
The `<text>` field in `@state` contains the expected question id of the next answer

### 3.2. Finish Session again (`SESSION_FINISHED`)

* `POST /answers?sessionid=381544dc-0000-0000-0d04-01123f06e306&question2=Answer+2`

We are answering our last question again. This progresses the session to "closed".

```
foo/b884bcb19954b245d8be53db2b266c4798dc742d
@user:META::0:0:0:0:0:0:0::0:1596163402
@group:META::0:0:0:0:0:0:0::0:1596163402
@authority:META:127.0.0.1(8099):1:0:0:0:0:0:0::0:1596163402
@state:META::3:0:0:1596163402:0:0:0::0:1596163402
question1:TEXT:Answer 1:0:0:0:0:0:0:0::0:1596163402
question2:TEXT:Answer 2 Again:0:0:0:0:0:0:0::0:1596163402

```

The `<value>` field in `@state` header has progressed again to `SESSION_FINISHED`. Our 'deleted' answer has been updated.

The `<text>` field in `@state` is empty

## 4. Close Session (`SESSION_CLOSED`)

* /analysis?sessionid=381544dc-0000-0000-0d04-01123f06e306

We are fetching our analysis. This progresses the session to "closed".

```
foo/b884bcb19954b245d8be53db2b266c4798dc742d
@user:META::0:0:0:0:0:0:0::0:1596163402
@group:META::0:0:0:0:0:0:0::0:1596163402
@authority:META:127.0.0.1(8099):1:0:0:0:0:0:0::0:1596163402
@state:META::4:0:0:1596163402:1596163402:0:0::0:1596163402
question1:TEXT:Answer 1:0:0:0:0:0:0:0::0:1596163402
question2:TEXT:Answer 2 Again:0:0:0:0:0:0:0::0:1596163402

```

The `<value>` field in `@state` header has progressed again to `SESSION_CLOSED`. The time of closure has been recorded int field `<time_end>`.

* This state is **final** and cannot be changed.
* Any session request other than */analysis* will be **rejected**

The `<text>` field in `@state` is empty
