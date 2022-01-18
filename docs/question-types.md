# Question Types


Question types have two purposes:

 1) flag the question format to the client
 2) indicate to the client how to display the question
 3) define how to format the answer value(s)
 2) map received answer value(s) to answer the defined answer fields

* definition: `backend/includes/survey.h`

## Field mapping

| Question tpye         | Backend dev state     | Frontend dev state    | Answer fields            | format      | Unit           | description |
| ---                   | ---                   | ---                   | ---                      | ---         | ---            | ---         |
| META                  | x                     | x                     | **any**                  | *per field* | -              | **Reserved type for sessions**. No question definition. [Set by backend](sessions.md) |
| INT                   | x                     | x                     | `value`                  | `long long` | set by backend | Answer is an integer, bounded by min_value and max_value. |
| FIXEDPOINT            | x                     | x                     | `value`                  | `long long` | -              | Answer is a fixed point value encoded as a 64-bit integer and with decimal_places places after the decimal. |
| MULTICHOICE           | x                     | x                     | `text`                   | `char *`    | -              | Answer is a single choice or comma separated list choices, out of `question->choices`. |
| MULTISELECT           | x                     | x                     | `text`                   | `char *`    | -              | Answer is a single choice or comma separated list of choices, out of `question->choices`. |
| LATLON                | x                     | x                     | `lat`, `lon`             | `long long` | degrees        | Answer is a pair geographic coordinates ([-90 +90] & [-180 +180]). |
| DATETIME              | x                     | no                    | `time_begin`             | `long long` | seconds        | Answer is a UNIX *date time* timestamp (+-). |
| DAYTIME               | x                     | x                     | `time_begin`             | `long long` | seconds        | Answer is a *day time* value for a generic day in seconds since midnight. |
| TIMERANGE             | x                     | x                     | `time_begin`, `time_end` | `long long` | seconds        | Answer is a generic time period. |
| UPLOAD                | no                    | no                    | -                        | ---         | -              | |
| TEXT                  | x                     | x                     | `text`                   | `char *`    | -              | Answer is text. |
| CHECKBOX              | x                     | x                     | `text`                   | `char *`    | -              | Answer is a single choice, one of `question->choices`. Requires two defined choices in the following order: [OFF-value, ON-value]. |
| HIDDEN                | x                     | x                     | `text`                   | `char *`    | -              | Answer is `question->default_value` or a text value set by the client. Instruction for instructional text (`question->description`), displayed as text/markup without input field. |
| TEXTAREA              | x                     | x                     | `text`                   | `char *`    | -              | Answer is text. Instruction for textarea. |
| EMAIL                 | partial               | x                     | `text`                   | `char *`    | -              | Answer is text. Instruction for email address. Currently, it is up to the client to validate the input. |
| SINGLECHOICE          | x                     | x                     | `text`                   | `char *`    | -              | Answer is a single choice, one of `question->choices`. Requires two defined choices in the following order: [OFF-value, ON-value]. |
| SINGLESELECT          | x                     | x                     | `text`                   | `char *`    | -              | Answer is a single choice, one of `question->choices`. |
| FIXEDPOINT_SEQUENCE   | partial               | no                    | `text`                   | `char *`    | set by backend | Answer is a comma separated list of float values. The values must be an ascending sequence mapped to `question->choices`, which are acting as text labels. Currently, it is up to the client to validate the input. |
| DAYTIME_SEQUENCE      | partial               | x                     | `text`                   | `char *`    | seconds        | Answer is a comma separated list of *day time* values (generic day in seconds since midnight). The values must be an ascending sequence mapped to `question->choices`, which are acting as text labels. Currently, it is up to the client to validate the input. |
| DATETIME_SEQUENCE     | partial               | no                    | `text`                   | `char *`    | seconds        | Answer is a comma separated list of  UNIX *date time* timestamp values (+-, generic day in seconds since midnight). The values must be an ascending sequence mapped to `question->choices`, which are acting as text labels. Currently, it is up to the client to validate the input. |
| DURATION24            | x                     | no                    | `value`                  | `long long` | seconds        | Answer is an integer within the range of 0 - 86400 seconds |
| DIALOG_DATA_CRAWLER   | x                     | x                     | `text`                   | `char *`    | [module id]    | Answer is a single choice, one of `question->choices`. Instruction for a dialog to give consent to accessing external service. Requires two defined choices in the following order: Requires two defined choices: [DENIED, GRANTED]. Question and answer `unit` field is used to store the service module id. See [fitbit-queue](https://github.com/RoboSparrow/fitbit-queue) for an example of linked a service module |
| SHA1_HASH             | x                     | no                    | `text`                   | `char *`    | -              | Request: Answer is text. Session: text value will be converted into SHA1 checksum. The incoming answer value will not be saved and erased from memory. |
| UUID                  | no                    | no                    | `text`                   | `char *`    | -              | Answer is a valid RFC 4122 (variant 2?) UUID |

## Answer Examples

### QTYPE_INT

 * number input

```
# question definition (serialised)
q1:Your age::INT:0::0:0:0:0::

# client answer (serialised: public)
q1::99:0:0:0:0:0:0

# session (serialised: private)
q1:INT::99:0:0:0:0:0:0::0:1629353313
```

### QTYPE_FIXED_POINT

 TODO


### QTYPE_MULTICHOICE

* ui indicator: tick boxes

```
# question definition (serialised)
q1:Select any::MULTICHOICE:0::-1:-1:-1:4:first,second,third,fourth:

# client answer (serialised: public)
q1:third,second:0:0:0:0:0:0:0

# session (serialised: private)
q1:MULTICHOICE:third,second:0:0:0:0:0:0:0::0:1629353313
```

### QTYPE_MULTICSELECT

* ui indicator: multi select

```
# question definition (serialised)
q1:Select any::MULTICSELECT:0::-1:-1:-1:4:first,second,third,fourth:

# client answer (serialised: public)
q1:third,second:0:0:0:0:0:0:0

# session (serialised: private)
q1:MULTICSELECT:third,second:0:0:0:0:0:0:0::0:1629353313
```
