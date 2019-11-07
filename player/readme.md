# Survey content player

Command line tool for progressing through a survey with generic or custom answers.
The survey progress is displayed to the cli and logged into a csv file for inspection.

**Install:**

```
$ npm install
```

**Setup:**

copy `template.config.js` to `./configs/config.js` and
    -  fill in valid survey server configuration.
    - add custom answers for specified question ids in order to to overwrite the default answers.

It is possible to have multiple configs by simply prefixing them, ie.e `./configs/<YOUR CUSTOM FILENAME>.js`.
The `./player` command will pick them up and ask you to select one.

**Usage:**

```
# run sequence
$ ./player
```

**Logs**

Apart from the console output the player logs all steps into the './log` folder.
Log file names are prefixed with the matching `surveyid` and `sessionid`.
The matching `/analysis` response is also stored in that folder.
