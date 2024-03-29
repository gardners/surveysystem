"""
A simple example of a Python survey controller, includes the required C api hook functions
Copy this file to "nextquestion.py" and restart the server without ENV "SURVEY_PYTHONDIR" (external package path) being set.

Hook arguments supplied from backend:

1) questions:

 - list of all defined question uids for this survey. The list is generated from a hashed copy of the main survey file, the hash is stored in the backend session file.
 - The hashed copy definitions may differ from the main survey file.

['question1', 'question2', 'question3' ]

2) answers: a

 - list of dicts containing the already given answers for this session, including (since #449) some merged in matching question properties
 - internal system answers (generated by backend in run-time) are not included

[{

   # answer

    'uid': 'question1',
    'type': 'TEXT',
    'text': 'Answer 1',
    'value': 0,
    'latitude': 0,
    'longitude': 0,
    'time_begin': 0,
    'time_end': 0,
    'time_zone_delta': 0,
    'dst_delta': 0,
    'unit': '',
    'flags': 0,
    'stored': 1618633195,

    # some matching question props (showing defaults)

    '_flags': 0,
    '_default_value': '',
    '_min_value': -1,
    '_max_value': -1,
    '_choices': '',
    '_unit': ''
}]

3) kwargs (since #445):

 * survey_id: (str) survey id and hashed version - pattern: '<survey_id>/<hash>'
 * session_id: (str) current session id
 * action: (str) text representation of current (performed) session action (see survey.h enum actions) - one of:
   - 'ACTION_SESSION_NEXTQUESTIONS',
   - 'ACTION_SESSION_ADDANSWER',
   - 'ACTION_SESSION_DELETEANSWER',
   - 'ACTION_SESSION_ANALYSIS',
 * affected_count: (int) count of affected answers by above action, additions are positive and deletions are negative

"""
import os
import json
import logging
import traceback
from datetime import datetime


def init_logging(filename):
    """
    set up root logger
    """
    level = logging.DEBUG
    logging.basicConfig(
        filename=filename,
        level=level,
        format='%(asctime)s [%(levelname)s] %(message)s',
        datefmt='%m/%d/%Y %I:%M:%S %p'
    )


# survey.h : struct { next_questions }
CONST_STATUS_INFO = 0
CONST_STATUS_WARN = 1
CONST_STATUS_ERROR = 2


####
# C backend hooks
####

def nextquestion(questions, answers, **kwargs):
    """
    invoke questionLogic and return next question ids
     - this example simply retuns the next question id
     - args, kwargs: see module description
    """

    log_file = kwargs.get('log_file')
    if log_file:
        init_logging(log_file)

    logging.info('nextquestion hook invoked: {}'.format(os.getcwd()))

    progress = [len(answers), len(questions)]

    if len(answers) < len(questions):
        return {
            'status': CONST_STATUS_INFO,
            'message': '',
            'progress': progress,
            'next_questions': [
                questions[len(answers)]
            ],
        }

    return {
        'status': CONST_STATUS_INFO,
        'message': 'Survey finished!',
        'progress': progress,
        'next_questions': [],
    }


def analyse(questions, answers, **kwargs):
    """
    invoke analytics and return analyse json
    - args, kwargs: see module description
    """

    log_file = kwargs.get('log_file')
    if log_file:
        init_logging(log_file)

    logging.info('analyse hook invoked: {}'.format(os.getcwd()))

    now = datetime.now()

    analysis = {
        'created': now.isoformat(),
        'version': '1.0.0',
        'constraints': [{
            'key': 'Notification',
            'message': 'Lorem ipsum..',
        }],
        'evaluations': [{
            'category': 'Lorem',
            'classification': 'Lorem ipsum dolor sit amet, consectetur adipiscing elit',
            'rank': 0,
            'riskRating': 0,
            'recommendation': 'Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi',
            'additionalInsights': [{
                'displayName': 'Duis aute irure dolor',
                'displayText': 'Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.',
            }],
            'conditions': {
                'condition': 'Facilisis volutpat',
                'subcondition': 'Facilisis volutpat est velit egestas dui id ornare arcu.',
                'mainText': answers,
                'learnMore': questions,
                'mainRecommendation': 'Sodales ut eu sem integer vitae justo eget magna fermentum.',
                'mandatoryTips': 'Integer eget aliquet nibh praesent tristique magna sit amet.',
                'additionalInsights': [{
                    'displayName': 'Semper risus',
                    'displayText': 'Mauris sit amet massa vitae.',
                }],
            },
        }],
    }

    return json.dumps(analysis)


def cmodule_traceback(exc_type, exc_value, exc_tb):
    """
    Hook for returning a printable traceback string for a given error from  within c code, invoked by backend
    """
    lines = []
    lines = traceback.format_exception(exc_type, exc_value, exc_tb)
    output = '\n'.join(lines)
    return output
