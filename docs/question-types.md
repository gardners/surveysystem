# Question types overview

* `backend/includes/survey.h`

* `front/src/Question.js`
* `front/src/Answer.js`

|  Type flag                    | Type name             | Backend dev state     | Frontend dev state    | Answer fields                 | Unit          | description |
| ---                           | ---                   | ---                   | ---                   | ---                           | ---           | ---         |
| QTYPE_INT                     | INT                   | x                     | x                     | `value`                       | set by client | Answer is an integer, bounded by min_value and max_value. |
| QTYPE_FIXEDPOINT              | FIXEDPOINT            | x                     | x                     | `value`                       | -             | Answer is a fixed point value encoded as a 64-bit integer and with decimal_places places after the decimal. |
| QTYPE_MULTICHOICE             | MULTICHOICE           | x                     | x                     | `text`                        | -             | Answer is a single choice or comma separated list choices, out of `question->choices`. |
| QTYPE_MULTISELECT             | MULTISELECT           | x                     | x                     | `text`                        | -             | Answer is a single choice or comma separated list of choices, out of `question->choices`. |
| QTYPE_LATLON                  | LATLON                | x                     | x                     | `lat`, `lon`                  | degrees       | Answer is a pair geographic coordinates ([-90 +90] & [-180 +180]). |
| QTYPE_DATETIME                | DATETIME              | x                     | no                    | `time_begin`                  | seconds       | Answer is a UNIX *date time* timestamp (+-). |
| QTYPE_DAYTIME                 | DAYTIME               | x                     | x                     | `time_begin`                  | seconds       | Answer is a *day time* value for a generic day in seconds since midnight. |
| QTYPE_TIMERANGE               | TIMERANGE             | x                     | x                     | `time_begin`, `time_end`      | seconds       | Answer is a generic time period. |
| QTYPE_UPLOAD                  | UPLOAD                | no                    | -                     | -                             | -             | |
| QTYPE_TEXT                    | TEXT                  | x                     | x                     | `text`                        | -             | Answer is text. |
| QTYPE_CHECKBOX                | CHECKBOX              | x                     | x                     | `text`                        | -             | Answer is a single choice, one of `question->choices`. Requires two defined choices in the following order: [OFF-value, ON-value]. |
| QTYPE_HIDDEN                  | HIDDEN                | x                     | x                     | `text`                        | -             | Answer is `question->default_value` or a text value set by the client. Instruction for instructional text (`question->description`), displayed as text/markup without input field. |
| QTYPE_TEXTAREA                | TEXTAREA              | x                     | x                     | `text`                        | -             | Answer is text. Instruction for textarea. |
| QTYPE_EMAIL                   | EMAIL                 | partial               | x                     | `text`                        | -             | Answer is text. Instruction for email address. Currently, it is up to the client to validate the input. |
| QTYPE_PASSWORD                | PASSWORD              | partial               | x                     | `text`                        | -             | Answer is text. Instruction for a password field (masked field). Currently, it is up to the client to validate the input. |
| QTYPE_SINGLECHOICE            | SINGLECHOICE          | x                     | x                     | `text`                        | -             | Answer is a single choice, one of `question->choices`. Requires two defined choices in the following order: [OFF-value, ON-value]. |
| QTYPE_SINGLESELECT            | SINGLESELECT          | x                     | x                     | `text`                        | -             | Answer is a single choice, one of `question->choices`. |
| QTYPE_FIXEDPOINT_SEQUENCE     | FIXEDPOINT_SEQUENCE   | partial               | no                    | `text` (temp)                 | set by client | Answer is a comma separated list of float values. The values must be an ascending sequence mapped to `question->choices`, which are acting as text labels. Currently, it is up to the client to validate the input. |
| QTYPE_DAYTIME_SEQUENCE        | DAYTIME_SEQUENCE      | partial               | x                     | `text` (temp)                 | seconds       | Answer is a comma separated list of *day time* values (generic day in seconds since midnight). The values must be an ascending sequence mapped to `question->choices`, which are acting as text labels. Currently, it is up to the client to validate the input. |
| QTYPE_DATETIME_SEQUENCE       | DATETIME_SEQUENCE     | partial               | no                    | `text` (temp)                 | seconds       | Answer is a comma separated list of  UNIX *date time* timestamp values (+-, generic day in seconds since midnight). The values must be an ascending sequence mapped to `question->choices`, which are acting as text labels. Currently, it is up to the client to validate the input. |
| QTYPE_UUID                    | UUID                  | no |                  | no                    | `text`                        | -             | Answer is a valid RFC 4122 (variant 2?) UUID |
