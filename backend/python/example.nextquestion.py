"""
A simple example of a Python survey controller, includes the required C api hook functions
Copy this file to "nextquestion.py" and restart the server without ENV "SURVEY_PYTHONDIR" (external package path) being set.

kwargs supplied from backend:
 * survey_id: (str) survey id and hashed version - parttern: '<survey_id>/<hash>'
 * session_id: (str) current session id
 * action: (str) text reperesentation of current session action performed (see survey.h enum actions) - one of:
   - 'SESSION_NEXTQUESTIONS',
   - 'SESSION_ADDANSWER',
   - 'SESSION_DELETEANSWER',
   - 'SESSION_ANALYSIS',
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
     - available kwargs: see module description
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
    - available kwargs: see module description
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
