# Surveysystem
System for on-line and off-grid survey preparation and submissions


# Data storage

This system purposely uses a simplistic data storage scheme, in the
interests of robustness, and also making it easier to scale up and down,
and perform analysis on the data it collects.

The structure is relatively simple:

* `surveys/survey_name/current` - The current definition of a survey called "survey_name".
* `surveys/survey_name/<SHA1 hash>` - Older definitions of a survey, named after the SHA1 hash of the survey definition
* `python/nextquestion.py` - Python functions to select next question.
    - Next question provider: Functions should be named `nextquestion_<survey_name>_<SHA1 hash>`.  If no such function exists, then `nextquestion_<survey_name>` is tried, and failing that `nextquestion`.
    - Analysis provider: As above `analyse_<survey_name>_<SHA1 hash>`.  or `analyse_<survey_name>` or `analyse`.
* `sessions/<session uuid prefix>/<session uuid>` - Files containing each live session.  The prefix subdirectories are used to
prevent any given directory becoming too long, and slowing down the retrieval of a given survey.
* `logs/YYYY/MM/DD/YYYY-MM-DD-HH.log` - Log files of all activity

Stale sessions can simply be deleted via the file system, and surveys added or updated or deleted similarly easily.

## Environment Variables

**SURVEY_HOME** (required):

All data lives in `SURVEY_HOME`. The `SURVEY_HOME` environment variable **must** be defined and represents an absolute directory path to the backend dir (no trailing slash).

**SURVEY_PYTHONDIR** (optional):

Optionally you can define an external Python controller path via `SURVEY_PYTHONDIR`. This must be an absolute directory path. The backend will look for `<SURVEY_PYTHONDIR>/nextquestion.py`. This is recommended for more complex analysis requirements.
If not defined the backend falls back to the local `<SURVEY_HOME>/python/nextquestion.py` (see structure)

**SURVEY_FORCE_PYINIT** (tests <span color="red">only!</span>):

If `SURVEY_FORCE_PYINIT` is set to `"1"` the Python interpreter will re-initialise on *every* Python C-Api function call.

Use this only for *testing* or *development* environments, as this substantially slows down the application and might cause side effects inside your python controller.

**SS_TRUSTED_MIDDLEWARE**:

Register a trusted authentication middleware source. For details see [authorisation-and-middleware.md](docs/authorisation-and-middleware.md)

**SS_LOG_FILE**

Path to a writable custom log file

# Installation (backend)

This system requires Python >= 3.8 and clang. Additionally, `zlib and bmake` is required for compiling [kcgi](https://kristaps.bsd.lv/kcgi/index.html). To install on Ubuntu:

```bash
sudo apt-get install clang make
sudo apt-get install python3.8 python3.8-dev
sudo apt-get install zlib1g-dev bmake
```

Tests require Lighttpd and Curl

```
sudo apt-get install curl lighttpd
```

Then make sure to build and install kcgi:

```bash
git submodule init
git submodule update
cd backend/kcgi
./configure
sudo bmake install
```

Then create a folder for logs:

```bash
mkdir surveysystem/backend/logs
sudo chmod 777 surveysystem/backend/logs
mkdir surveysystem/backend/testlog
sudo chmod 777 surveysystem/backend/testlog
```

# Overview

![surveysystem architecture](docs/architecture.png)

## REST API

Note that the following section reflects the *current state* of development and will be subject to future changes.

### Endpoint

 * `GET /surveyapi/<path>?<query>`

### Paths and queries


| method | endpoint                                                           | response                                                | description                                                                                                          |
| ---    | ---                                                                | ---                                                     | ---                                                                                                                  |
| GET    | `/`                                                                |                                                         | index (not used, returns `204 no content`)                                                                           |
| GET    | `/session?surveyid`                                                | text: sessionid                                         | create session and retrieve generated session id                                                                     |
| POST   | `/session?surveyid&sessionid`                                      | text: sessionid                                         | create session with a given session id (uuidv4)                                                                      |
| GET    | `/questions?sessionid`                                             | json: [next questions](docs/next-questions-response.md) | get next questions to answer (questions, progress, status)                                                           |
| POST   | `/answers?sessionid` <sup>1)</sup> <sup>2)</sup>                   | json: [next questions](docs/next-questions-response.md) | answer previous questions, format: serialised answers<sup>5)</sup> (lines of colon separated values) in request body |
| POST   | `/answers?sessionid&answer` <sup>1)</sup>                          | json: [next questions](docs/next-questions-response.md) | answer single previous question, format: serialised answer<sup>5)</sup> (colon separated values)                     |
| POST   | `/answers?sessionid&{uid1}={value1}&{uid2}={value2}` <sup>1)</sup> | json: [next questions](docs/next-questions-response.md) | answer previous questions by ids and values, format: question id = answer value                                      |
| DELETE | `/answers?sessionid` <sup>3)</sup>                                 | json: [next questions](docs/next-questions-response.md) | delete last answers (roll back to previous questions)                                                                |
| DELETE | `/answers?sessionid&questionid` <sup>3)</sup>                      | json: [next questions](docs/next-questions-response.md) | delete last answers until (and including) the given question id (rollback)                                           |
| GET    | `/analysis?sessionid` <sup>4)</sup>                                | json                                                    | get analysis based on your answers                                                                                   |
| GET    | `/status(?extended)`                                               | status 200/204 no content                               | system status use the `extended` param for checking correct configuration and paths                                  |

- **1)**: Answers must match previous questions
- **2)**: requires header: `Content-Type: text/csv`
- **3)**: Request requires the `If-Modified`or `if-modified` param header with a valid consistency checksum. The checksum is provided  by the previous `ETag` response header value
- **4)**: Session must be finished (all questions answered)
- **5)** example for a serialised answer csv (QTYPE_TEXT): `question1:Hello+World:0:0:0:0:0:0:0`, see [serialisation docs for **public** answer definitions](docs/data-serialisation.md#answer-definitions)

The survey model is sequential. `POST /surveyapi/answer` is required to submit the answers for question ids in the exact same order as they were recieved. Similar with `DELETE /answer` requests, where question ids have to be submitted in the exact reverse order.

## Documentation

Documentation files live in [docs](docs/). The most important docs are about the [backend data serialisation model](docs/data-serialisation.md) and the [backend session life cycle](docs/sessions.md)
