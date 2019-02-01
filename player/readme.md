# Survey content player

Command line tool for progressing through a survey with generic or custom answers.
The s
urvey progress is displayed to the cli and logged into a csv file for inspection.

**Install:**

```
$ npm install
```

**Setup:**

copy `template.config.js` to `config.js` and
    -  fill in valid survey server configuration.
    - add custom answers for specified question ids in order to to overwrite the default answers.

**Usage:**

```
# run sequence
$ ./player
```


