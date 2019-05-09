
# Question Definitions

* backend: see `serialisers.c: serialise_question()`
* frontend: n/a

Format of an question row:

```csv
<uid>:<question_text>:<question_html>:<type>:<flags>:<default_value>:<min_value>:<max_value>:<decimal_places>:<num_choices>:<choices>:<unit>
```
Location: `surveys/<surveyID>/current` (can be symlinked)

| field              | data type | json   | notes       |
| ---                | ---       | ---    | ---         |
| **uid**            | char[]    | string | question id |
| **question_text**  | char[]    | string | question label |
| **question_html**  | char[]    | number | question description (html tags allowed) |
| **type**           | char[]    | number | question type |
| **flags**          | int       | number |             |
| **default_value**  | char[]    | number |             |
| **min_value**      | long long | number |             |
| **max_value**      | long long | number |             |
| **decimal_places** | int       | number |             |
| **num_choices**    | int       | string |             |
| **choices**        | char[]    | string | comma separated list |
| **unit**           | char[]    | string |             |

---

# Answer Definitions

* backend: see `serialisers.c: serialise_answer()`
* frontend: see `serializer` module

Format of an answer row:

```csv
<uid>:<text>:<value>:<lat>:<lon>:<time_begin>:<time_end>:<time_zone_delta>:<dst_delta>:<unit>
```

Location: `sessions/<session-prefix>/sessionID`

| field                 | data type | json   | notes       |
| ---                   | ---       | ---    | ---         |
| **uid**               | char[]    | string | question id |
| **text**              | char[]    | string |             |
| **value**             | long long | number |             |
| **lat**               | long long | number |             |
| **lon**               | long long | number |             |
| **time_begin**        | long long | number |             |
| **time_end**          | long long | number |             |
| **time_zone_delta**   | int       | number |             |
| **dst_delta**         | int       | number |             |
| **unit**              | char[]    | string |             |
