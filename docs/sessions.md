# Sessions

## Example session

This is an example of a session with the id `381544dc-0000-0000-0d04-01123f06e306`.

It contains two answered questions.
The session file is located in `<project_ root>/backend/sessions/3815/381544dc-0000-0000-0d04-01123f06e306`.
Our session id stored as the file name.

```
foo/9d8a5066756c437445aca87593e7cd35bca155d5
@user:META::0:0:0:0:0:0:0::0:1595557084
@group:META::0:0:0:0:0:0:0::0:1595557084
@authority:META:127.0.0.1(8099):1:0:0:0:0:0:0::0:1595557084
@closed:META::0:0:0:0:0:0:0::0:1595557084
question1:TEXT:Hello World 1:0:0:0:0:0:0:0::0:1595558084
question2:TEXT:Hello World 2:0:0:0:0:0:0:0::0:1595559084

```

## Linked survey definition

The matching survey definition for our session is located in: `<project_ root>/backend/surveys/foo/current`

```
version 2
Silly test survey updated
without python
question1:Question 1::TEXT:0::-1:-1:0:0::
question2:Question 2::TEXT:0::-1:-1:0:0::
```

When a session is created the backend saves a *snapshot copy* of the "current" `current` file, using the file's SHA1 hash as file name (if not alredy exists).

`<project_ root>/backend/surveys/foo/current` > `<project_ root>/backend/surveys/foo/9d8a5066756c437445aca87593e7cd35bca155d5`

For the remainder of the session *that copy* is being loaded and matched against the session answers.

## Session structure

```
                       ------
survey definition copy | foo/9d8a5066756c437445aca87593e7cd35bca155d5
                       ------
                       | @user:META::0:0:0:0:0:0:0::0:1595557084
header                 | @group:META::0:0:0:0:0:0:0::0:1595557084
                       | @authority:META:127.0.0.1(8099):1:0:0:0:0:0:0::0:1595557084
                       | @closed:META::0:0:0:0:0:0:0::0:1595557084
                       ------
session body           | question1:TEXT:Hello World 1:0:0:0:0:0:0:0::0:1595558084
                       | question2:TEXT:Hello World 2:0:0:0:0:0:0:0::0:1595559084
                       ------
```

Sections:

1) survey definition: Identifies the linked survey defintion cpy in format `<survey name>/<survey_sha>`.
2) session header: meta data set by backend: see below
3) session body: contains the *user* answers given to the questions defined inside our example survey definition (copy)

## Special Session answers

### Session `META` answers

Sessions can contain Meta answers.

example session line: `internalData:META:some text:12345:0:0:0:0:0:0::0:1595559714`

* Meta answers have no corresponding question definitions and are set by the backend.
* Meta answers report internal data and can occupy any data field in the answer. Since there is no question the uid is the identifier for parsing out the data.
* Meta answers are not visible to the frontend or nextquestion controllers. They cannot be deleted or edited by these components either.

### Session `@header` answers

On creation sessions report some internal data into header `META` fields.
The fields are: `@user`, `@group`, `@authority`, `@closed` (in this exact oerder)

* @header answers have no corresponding question definitions and are set by the backend *at the start of a session*
* @header answers are not visible to the frontend or nextquestion controllers. They cannot be deleted or edited by these components either.

