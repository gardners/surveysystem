# Survey content player

Command line tool for progressing through a survey with generic or custom answers.
The survey progress is displayed to the cli and logged into `./logs` for inspection.

**Setup:**

copy `config_template.py` to `config_<my_scenario>.py`
    1) **config**: fill in valid survey server configuration.
    2) **scenario**: a (currently complete) map of question ids and answers

**Usage:**

```bash
# with config prompt
./player
# specify config
./player --config config_<my_scenario>.py

```

TODO:
    - input option for missing scenario answers and save/load last completed scenario
