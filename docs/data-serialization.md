
# Question Definitions

definition:

 * `backend/include/survey.h` struct question

serialization:

 * `backend/src/serialisers.c: serialise_question()`

corresponding frontend module:

* `front/src/Question.js`

Format of an question row:

```csv
<uid>:<question_text>:<question_html>:<type>:<flags>:<default_value>:<min_value>:<max_value>:<decimal_places>:<num_choices>:<choices>:<unit>
```
Location: `surveys/<surveyID>/current` (can be symlinked)

| field              | data type | json   | notes       |
| ---                | ---       | ---    | ---         |
| **uid**            | char[]    | string | question id |
| **question_text**  | char[]    | string | question label |
| **question_html**  | char[]    | string | question description (html tags allowed) |
| **type**           | char[]    | string | question type |
| **flags**          | int       | number |             |
| **default_value**  | char[]    | string |             |
| **min_value**      | long long | number |             |
| **max_value**      | long long | number |             |
| **decimal_places** | int       | number |             |
| **num_choices**    | int       | number |             |
| **choices**        | char[]    | string | comma separated list |
| **unit**           | char[]    | string |             |

---

# Answer Definitions

definition:

 * `backend/include/survey.h` struct answer

serialization:

* `backend/src/serialisers.c: serialise_answer()`

corresponding frontend module:

* `front/src/Answer.js`


Format of an answer row:

```csv
<uid>:<text>:<value>:<lat>:<lon>:<time_begin>:<time_end>:<time_zone_delta>:<dst_delta>:<unit>:<flag>
```

Location: `sessions/<session-prefix>/sessionID`

| field                 | API access |               | data type | json   | notes       |
| ---                   | ---        | ---           | ---       | ---    | ---         |
| **uid**               | read/write | answer value  | char[]    | string | question id |
| **text**              | read/write | answer value  | char[]    | string |             |
| **value**             | read/write | answer value  | long long | number |             |
| **lat**               | read/write | answer value  | long long | number |             |
| **lon**               | read/write | answer value  | long long | number |             |
| **time_begin**        | read/write | answer value  | long long | number |             |
| **time_end**          | read/write | answer value  | long long | number |             |
| **time_zone_delta**   | read/write | answer value  | int       | number |             |
| **dst_delta**         | read/write | answer value  | int       | number |             |
| **unit**              | read/write | answer value  | char[]    | string | unit for numeric types, see below   |
| **flag**              | -          | control field | int       | -      | bit control flag, see below |
| **stored**            | -          | control field | long long | -      | UNIX timestamp, time of answer storage, see below |

**unit**

 - Time based question times: Use `seconds` only. The returned answer unit will always be a duration in seconds since midnight (where midnight == 0)
 - All other numeric qquestion types are free to define a human friendly unit (displayed on the frontend form) the unit will be included into the returned answer

**flag**

- Currently only supported flags are `answered` (0) or `deleted` (1)

**stored**

- timestamp is set on adding, deleting or updating an answer: *addanswer*, *updateanswer*, *delanswer*, *delanswerandfollowing*
