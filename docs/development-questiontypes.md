# Backed development notes

## Adding a new question type

### Backend

1. `survey.h`; define a new question type in `struct question{}`
2. add new type to `answer_get_value_raw()`
3. add new type to `answer_set_value_raw()`
4. fastcgi: if new type is supposed to offer choices and update `response_nextquestion_add_choices()`
5. preprcessing: check if question type requires answer preprocessing: `pre_add_answer_special_transformations()`
6. add  unit test for type in `test_units`
7. add test for type in `/tests`, file naming convention is `QTYPE_<MY_TYPE>.test`
8. update docs: [question-types.md](question-types.md)

### Frontend

1. parser: Answer.js: `Answer.setValue()`
2. parser: Answer.js: `Answer.getValue()`
3. add component
4. register component: Question.jsx  `getComponentByType()`

