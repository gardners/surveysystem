#!/bin/bash -x

# set up some testing of the command line interface

make || exit

# create a symbolic link of the survey question definitions so that we can test them while keeping them outside the public repo
if [ ! -f ./surveys/foo/current ]; then
	ln -s ${HOME}/code/SleepCompanionREimplementation/engine/SleepCompanion/current ./surveys/foo/
fi


# set up python function by creating symbolic links
mkdir -p ./python
if [ ! -f ./python/nextquestion.py ]; then
        ln -s ${HOME}/code/SleepCompanionREimplementation/nextquestion.py ./python/nextquestion.py 
        ln -s ${HOME}code/SleepCompanionREimplementation/questionLogic.py ./python/
fi

# seems to be a bug that the surveycli doesn't actually return a SESSIONID, so get it from the surveys/foo file
#SESSIONID=`ls -1 ./surveys/foo/ | head -n 1`
SESSIONID=`SURVEY_HOME=. ./surveycli newsession foo`

SURVEY_HOME=. ./surveycli nextquestion ${SESSIONID}
