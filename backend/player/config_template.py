import sys
from os.path import abspath
sys.path.append(abspath('../'))

from collections import OrderedDict

# helper for creating timestamps for time fields second = hhmmss(22, 30, 0)
from surveyh.serialiser import hhmmss

"""
server configuration
"""

config = {
    'uri': 'http://localhost:8099/surveyapi',
    'survey_id': 'demo',
}
# sandbox config, start dev server via ../devrun

"""
map of question ids and answer values
"""

scenario = OrderedDict({
    'question0': 'yes',
    'question1': 'Answer 1',
    'question2': 'Answer 2',
    'question3': 'foobar',
})
