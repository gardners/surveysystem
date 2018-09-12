#!/bin/bash

if [ ! -f ./surveys/foo/current ]; then
	ln -s /Users/wall0159/code/SleepCompanionREimplementation/engine/SleepCompanion/current ./surveys/foo/
fi

# set up python
mkdir -p ./python
if [ ! -f ./python/nextquestion.py ]; then
        ln -s /Users/wall0159/code/SleepCompanionREimplementation/nextquestion.py ./python/nextquestion.py 
        ln -s /Users/wall0159/code/SleepCompanionREimplementation/questionLogic.py ./python/
fi

SESSIONID=`ls -1 ./surveys/foo/ | head -n 1`
#SURVEY_HOME=. ./surveycli newsession foo

SURVEY_HOME=. ./surveycli nextquestion ${SESSIONID}
