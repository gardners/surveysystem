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

**Test sessions:**

Test sessions by copying a backend session file into the players `sessions` folder

**Usage:**

```
# with config prompt
./player
# with config
./player --config myconfig.js
# with config  and using answers from a session file
./player --config myconfig.js --session 0d8bfeff-0000-0000-18f7-53908bbace03

```

**Logs**

Apart from the console output the player logs all steps into the './log` folder.
Log file names are prefixed with the matching `surveyid` and `sessionid`.
The matching `/analysis` response is also stored in that folder.
