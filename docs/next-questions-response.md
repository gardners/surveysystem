# Next questions response format

PR #96 added `next_questions { unit }` (> v0.0.3)
PR #334 added context fields (> v0.2.2)
since PR #343 `next_questions { min_value, max_value, choices }` properties are included by default (> v0.2.2)
PR #405 add `progress[]` property

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

## nextQuestions Context (Python)

The next_questions context allows the (python) controller to convey custom messages to the frontend.
At the current state (v0.2.2) the backend is not involved with these values and just passing them on.

This can be used to implement some context based validation

```python
def nextquestion(questions, answers, logFilename=None):

    # ... answer to UltimateQuestion was 43,
    # ... serve last question again
    return {
        'status': 1 // WARN
        'message': 'Are you <em>sure</em> about this? Please try again.'
        'progress': [1, 5], // array [ <given_answer_count>, <question_count> ]
        'next_questions': [ 'UltimateQuestion' ]
    }

```

Controllers who don't support progress calculation may use negative values to signal this to the frontend

## Template tags (Frontend)

Html enabled fields (context message, question description) support a limited set of template tags.
This allows the backend controller to render some frontend context vars.

* `%SURVEY_ID%`: (SurveyContext) current survey id,
* `%SESSION_ID%`: (SurveyContext) current sessionI id,
* `%PUBLIC_URL%`: (.env) PUBLIC_URL, use this for linking static documents in your message
* `%SITE_NAME%`:  (.env) REACT_APP_SITE_NAME,
* `%SURVEY_PROVIDER%`: (.env) REACT_APP_SURVEY_PROVIDER,

See `InnerHtml` Component for a complete list
