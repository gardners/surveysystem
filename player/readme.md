# Survey content player

Command line tool for progressing through a survey with generic or custom answers.
The survey progress is displayed to the cli and logged into a csv file for inspection.

**Install:**

```
$ npm install
```

**Setup:**

copy `template.config.js` to `config.js` and
    -  fill in valid survey server configuration.
    - add custom answers for specified question ids in order to to overwrite the default answers.

It is possible to have multiple configs by simply prefixing them, ie.e `<yourprefix>.config.js`.
The `./player` command will pick them up and ask you to select one.

**Usage:**

```
# run sequence
$ ./player

# alternatively
$ node run <yourprefix>.config.js
```

CSV log rows

 * `step`:  survey progess number
 * `question id`:  question.id
 * `question type`:  question.type
 * `question title`:  question.type
 * `answer type`:  'generic' for an automatted answer ,'custom' for a custom answer provided by config
 * `submitted answer`:  the serialized answer sent to the api (csv fragment)
 * `stored answer`:  the stored answer in the backend session file, not applicable when using player on mock server
 * `next questions`  next question ids provided by the api response
