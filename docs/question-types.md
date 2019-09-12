# Question types overview

* `backend/includes/survey.h`

* `front/src/Question.js`
* `front/src/Answer.js`

| Question tpye         | Backend dev state     | Frontend dev state    | Answer fields                 | Unit          | description |
| ---                   | ---                   | ---                   | ---                           | ---           | ---         |
| INT                   | x                     | x                     | `value`                       | set by client | Answer is an integer, bounded by min_value and max_value. |
| FIXEDPOINT            | x                     | x                     | `value`                       | -             | Answer is a fixed point value encoded as a 64-bit integer and with decimal_places places after the decimal. |
| MULTICHOICE           | x                     | x                     | `text`                        | -             | Answer is a single choice or comma separated list choices, out of `question->choices`. |
| MULTISELECT           | x                     | x                     | `text`                        | -             | Answer is a single choice or comma separated list of choices, out of `question->choices`. |
| LATLON                | x                     | x                     | `lat`, `lon`                  | degrees       | Answer is a pair geographic coordinates ([-90 +90] & [-180 +180]). |
| DATETIME              | x                     | no                    | `time_begin`                  | seconds       | Answer is a UNIX *date time* timestamp (+-). |
| DAYTIME               | x                     | x                     | `time_begin`                  | seconds       | Answer is a *day time* value for a generic day in seconds since midnight. |
| TIMERANGE             | x                     | x                     | `time_begin`, `time_end`      | seconds       | Answer is a generic time period. |
| UPLOAD                | no                    | -                     | -                             | -             | |
| TEXT                  | x                     | x                     | `text`                        | -             | Answer is text. |
| CHECKBOX              | x                     | x                     | `text`                        | -             | Answer is a single choice, one of `question->choices`. Requires two defined choices in the following order: [OFF-value, ON-value]. |
| HIDDEN                | x                     | x                     | `text`                        | -             | Answer is `question->default_value` or a text value set by the client. Instruction for instructional text (`question->description`), displayed as text/markup without input field. |
| TEXTAREA              | x                     | x                     | `text`                        | -             | Answer is text. Instruction for textarea. |
| EMAIL                 | partial               | x                     | `text`                        | -             | Answer is text. Instruction for email address. Currently, it is up to the client to validate the input. |
| PASSWORD              | partial               | x                     | `text`                        | -             | Answer is text. Instruction for a password field (masked field). Currently, it is up to the client to validate the input. |
| SINGLECHOICE          | x                     | x                     | `text`                        | -             | Answer is a single choice, one of `question->choices`. Requires two defined choices in the following order: [OFF-value, ON-value]. |
| SINGLESELECT          | x                     | x                     | `text`                        | -             | Answer is a single choice, one of `question->choices`. |
| FIXEDPOINT_SEQUENCE   | partial               | no                    | `text` (temp)                 | set by client | Answer is a comma separated list of float values. The values must be an ascending sequence mapped to `question->choices`, which are acting as text labels. Currently, it is up to the client to validate the input. |
| DAYTIME_SEQUENCE      | partial               | x                     | `text` (temp)                 | seconds       | Answer is a comma separated list of *day time* values (generic day in seconds since midnight). The values must be an ascending sequence mapped to `question->choices`, which are acting as text labels. Currently, it is up to the client to validate the input. |
| DATETIME_SEQUENCE     | partial               | no                    | `text` (temp)                 | seconds       | Answer is a comma separated list of  UNIX *date time* timestamp values (+-, generic day in seconds since midnight). The values must be an ascending sequence mapped to `question->choices`, which are acting as text labels. Currently, it is up to the client to validate the input. |
| DURATION24            | x                     | no                    | `value`                       | seconds       | Answer is an integer within the range of 0 - 86400 seconds |
| DIALOG_DATA_CRAWLER   | x                     | x                     | `text`                        | [module id]   | Answer is a single choice, one of `question->choices`. Instruction for a dialog to give consent to accessing external service. Requires two defined choices in the following order: Requires two defined choices: [DENIED, GRANTED]. Question and answer `unit` field is used to store the service module id. See [fitbit-queue](https://github.com/RoboSparrow/fitbit-queue) for an example of linked a service module |
| UUID                  | no                    | no                    | `text`                        | -             | Answer is a valid RFC 4122 (variant 2?) UUID |
