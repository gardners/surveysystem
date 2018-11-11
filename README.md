# surveysystem
System for on-line and off-grid survey preparation and submissions


# Data storage

This system purposely uses a simplistic data storage scheme, in the
interests of robustness, and also making it easier to scale up and down,
and perform analysis on the data it collects.

The structure is relatively simple:

surveys/survey_name/current - The current definition of a survey called "survey_name".
surveys/survey_name/<SHA1 hash> - Older definitions of a survey, named after the SHA1 hash of the survey definition
python/nextquestion.py - Python functions to select next question.  Functions should be named nextquestion_<survey_name>_<SHA1 hash>.  If no such function exists, then nextquestion_<survey_name> is tried, and failing that nextquestion.
sessions/<session uuid prefix>/<session uuid> - Files containing each live session.  The prefix subdirectories are used to
prevent any given directory becoming too long, and slowing down the retrieval of a given survey.
logs/YYYY/MM/DD/YYYY-MM-DD-HH.log - Log files of all activity

Stale sessions can simply be deleted via the file system, and surveys added or updated or deleted similarly easily.

All data lives in $SURVEY_HOME. The SURVEY_HOME environment variable must be defined.

This system requires python 3.7 and clang. To install on ubuntu:

```
sudo apt-get install python3.7-dev python3.7 clang
```

Then make sure to build and install kcgi:

```
git submodule init
git submodule update
cd kcgi
make install
```

Then create a folder for logs:

```
mkdir surveystsem/backend/logs
sudo chmod 777 surveystsem/backend/logs
```
