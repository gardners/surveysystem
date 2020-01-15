# Surveysystem
System for on-line and off-grid survey preparation and submissions


# Data storage

This system purposely uses a simplistic data storage scheme, in the
interests of robustness, and also making it easier to scale up and down,
and perform analysis on the data it collects.

The structure is relatively simple:

* `surveys/survey_name/current` - The current definition of a survey called "survey_name".
* `surveys/survey_name/<SHA1 hash>` - Older definitions of a survey, named after the SHA1 hash of the survey definition
* `python/nextquestion.py` - Python functions to select next question.  Functions should be named `nextquestion_<survey_name>_<SHA1 hash>`.  If no such function exists, then `nextquestion_<survey_name>` is tried, and failing that `nextquestion`.
* `sessions/<session uuid prefix>/<session uuid>` - Files containing each live session.  The prefix subdirectories are used to
prevent any given directory becoming too long, and slowing down the retrieval of a given survey.
* `logs/YYYY/MM/DD/YYYY-MM-DD-HH.log` - Log files of all activity

Stale sessions can simply be deleted via the file system, and surveys added or updated or deleted similarly easily.

## Environment Variables

All data lives in `SURVEY_HOME`. The `SURVEY_HOME` environment variable **must** be defined and represents an absolute directory path to the backend dir (no trailing slash).
Optionally you can define an external Python controller path via `SURVEY_PYTHONDIR`. This must be an absolute directory path. The backend will look for `<SURVEY_PYTHONDIR>/nextquestion.py`. This is recommended for more comples analysis requirements.
In case this variable is not defined the backend falls back to the local `<SURVEY_HOME>/python/nextquestion.py` mentioned above

# Installation (backend)

This system requires python 3.7 and clang. Additionally, bmake is required for compiling [kcgi](https://kristaps.bsd.lv/kcgi/index.html). To install on Ubuntu:

```bash
sudo apt-get install python3.7-dev python3.7 clang
```

Then make sure to build and install kcgi:

```bash
git submodule init
git submodule update
cd backend/kcgi
./configure
sudo make install
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

| Path                    | Action                                                                  | Method | Format           | Query params            | Returns |
| ---                     | ---                                                                     | ---    | ---              | ---                     |  ---       |
| **Session**             |                                                                         |        |                  |                         |         |
| `newsession`            | create a new survey session                                             | GET    | application/text | `?surveyid`             | session id |
| `delsession`            | delete current session                                                  | GET    | application/text | `?sessionid`            | -       |
| **Survey**              |                                                                         |        |                  |                         |         |
| `nextquestion`          | get next questions                                                      | GET    | application/json | `?sessionid`            | `{ next_questions }`<br> array of question objects |
| `addanswer`             | provide a single answer and get next questions                          | GET(!) | application/json | `?sessionid&answer`     | `{ next_questions }`<br> array of question objects |
| `updateanswer`          | alias for addanswer                                                     | GET(!) | application/json | `?sessionid&answer`     | `{ next_questions }`<br> array of question objects |
| `delanswer`             | remove a specific answer and get next questions                         | GET    | application/json | `?sessionid&questionid` | `{ next_questions }`<br> array of *updated* question objects |
| `delanswerandfollowing` | remove all answers up to a specified question id and get next questions | GET    | application/json | `?sessionid&questionid` | `{ next_questions }`<br> array of *updated* question objects |
| `analyse`               | fetch analysis of a completed survey                                    | GET    | application/json | `?sessionid`            | `{ feedback, report}`<br> survey analysis |
| **System**              |                                                                         |        |                  |                         |         |
| `accesstest`            | check system (filesystem)                                               | GET    | application/text | -                       | - |
| `fastcgitest`           | check survey access (fastcgi)                                           | GET    | application/text | -                       | - |

The survey model is sequential. `/surveyapi/addanswer`, `/surveyapi/updateanswer` are required to submit the answers for question ids in the exact same order as they were recieved. Similar with `delanswer` requests, where question ids have to be submitted in the exact reverse order.
