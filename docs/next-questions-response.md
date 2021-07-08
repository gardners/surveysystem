# Next questions response format

```javascript
{

    /* Context */
    "status": 0,        // status  0: INFO, 1: WARN, 2: ERROR
    "message": "",      // accepts html
    "progress": [1, 5], // array [ <given_answer_count>, <question_count> ]

    /* Questions */
    "next_questions": [{
        "id": "UltimateQuestion",
        "name": "UltimateQuestion",
        "title": "Answer to the Ultimate Question of Life, the Universe, and Everything",
        "description": "", // accepts html
        "type": "INT",
        "default_value": "",
        "min_value": 0,
        "max_value": 0,
        "choices": [],
        "unit": ""
    }]
}
```

## Session

Every time next questions are being processed, a comma separated list of the next question ids will be stored inside the session header, see [**@state** header](sessions.md)

## nextQuestions Context (Python)

The next_questions context allows the (python) controller to convey custom messages to the frontend.
At the current state (v0.2.2) the backend is not involved with these values and just passing them on.

This can be used to implement some context based validation

```python
def nextquestion(questions, answers, , **kwargs):

    # ... answer to UltimateQuestion was 43,
    # ... serve last question again
    return {
        'status': 1 // WARN
        'message': 'Are you <em>sure</em> about this? Please try again.'
        'progress': [1, 5], // array [ <given_answer_count>, <question_count> ]
        'next_questions': [ 'UltimateQuestion' ]
    }

```

### Session context args (kwargs)

 * `survey_id`: (str) survey id and hashed version - pattern: '<survey_id>/<hash>'
 * `session_id` (str) current session id
 * `action`: (str) text representation of current (performed) session action (see survey.h enum actions) - one of:
   - 'SESSION_NEXTQUESTIONS',
   - 'SESSION_ADDANSWER',
   - 'SESSION_DELETEANSWER',
   - 'SESSION_ANALYSIS',
 * `affected_count`: (int) count of affected answers by above action, additions are positive and deletions are negative


Controllers who don't support progress calculation may use negative values `[-1, -1]` to signal this to the frontend.

## Template tags (Frontend)

Html enabled fields (context message, question description) support a limited set of template tags.
This allows the backend controller to render some frontend context vars.

* `%SURVEY_ID%`: (SurveyContext) current survey id,
* `%SESSION_ID%`: (SurveyContext) current sessionI id,
* `%PUBLIC_URL%`: (.env) PUBLIC_URL, use this for linking static documents in your message
* `%SITE_NAME%`:  (.env) REACT_APP_SITE_NAME,
* `%SURVEY_PROVIDER%`: (.env) REACT_APP_SURVEY_PROVIDER,

See `InnerHtml` Component for a complete list
