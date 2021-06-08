"""
@see backend/include/survey.h
@see backend/src/nextquestion.c

"""
import time

class SerialiseError(Exception):
    pass


class DeSerialiseError(Exception):
    pass


def _is_int_list_asc(vals):
    """
    validates an ascending list of integers or integer strings
    """
    for i, v in enumerate(values):
        if i > 0 and int(v) < int(values[i - 1]):
            return False
    return True


####
## specific csv (de)serialisers for backend
####


def serialise_csw_row(row):
    """
    serialises values of a given dict into a backend csv string
    @see serialisers.c
    """
    res = []
    for val in row:
        if val is None:
            res.append('')
            continue

        if isinstance(val, bool):
            res.append(str(val).lower())
            continue

        sval = str(val)
        sval = sval.replace(':', '\:')
        res.append(sval)

    return ':'.join(res)


def deserialise_csw_row(row):
    """
    de-serialises a backend csv string into a dict
    @see serialisers.c
    """
    line = row.strip('\r\n').replace('\:', '[[D]]')
    sline = line.split(':')
    res = []
    for val in sline:
        val = val.replace('[[D]]', ':')
        res.append(val)
    return res


####
## answer model
####


def answer_model():
    """
    create a blank answer dict (backend answer scope)
    @see survey.h

    struct answer {
        char *uid;
        char *text;
        long long value;
        long long lat;
        long long lon;
        long long time_begin;
        long long time_end;
        int time_zone_delta;
        int dst_delta;
        char *unit;
        int flags;
        long long stored;
    };
    """

    model = {
        'uid': '',
        'type': '', # type added to answer, see gh issue #358
        'text': '',
        'value': 0,
        'lat': 0,
        'lon': 0,
        'time_begin': 0,
        'time_end': 0,
        'time_zone_delta': 0,
        'dst_delta': 0,
        'unit': '',
        'flags': 0,
        'stored': 0,
    }
    return model


def deserialise_answer(line, isBackend=True):
    """
    Loads and parses an answer dict from a line of a session file
    returns None or an answer dict
    """
    answer = answer_model()
    parts = deserialise_csw_row(line)

    if len(parts) != len(answer.keys()): # header or invalid
        return None

    answer.update({
        'uid':                       parts[0].strip(),
        'type':                      parts[1].strip(),
        'text':                      parts[2].strip(),
        'value':            0 if not parts[3].strip() else int(parts[3].strip()),
        'lat':              0 if not parts[4].strip() else int(parts[4].strip()),
        'lon':              0 if not parts[5].strip() else int(parts[5].strip()),
        'time_begin':       0 if not parts[6].strip() else int(parts[6].strip()),
        'time_end':         0 if not parts[7].strip() else int(parts[7].strip()),
        'time_zone_delta':  0 if not parts[8].strip() else int(parts[8].strip()),
        'dst_delta':        0 if not parts[9].strip() else int(parts[9].strip()),
        'unit':                      parts[10].strip(),
        'flags':            0 if not parts[11].strip() else int(parts[11].strip()),
        'stored':           0 if not parts[12].strip() else int(parts[12].strip()),
    })

    if not isBackend:
        answer.pop('stored', None)
        answer.pop('flags', None)

    return answer


def serialise_answer(answer, isBackend=True):
    """
    parses an answer dict from a csv line
    returns None or an answer dict
    """
    model = answer_model()

    if not isBackend:
        model.pop('type')
        model.pop('flags')
        model.pop('stored')

    for key in model:
        model[key] = answer[key]

    serialized = serialise_csw_row(model.values())
    return serialized


def get_answer_value(answer, question_type=None):
    """
    Get value of an answer based on it's question type
    - backend answers include question type since surveysystem #359
    - public answers or legacy answers require question_type arg
    """
    qtype = None

    if 'type' in answer:
        qtype = answer['type']
    else:
        qtype = question_type

    if not qtype:
        raise ValueError('No type defined for anwser: {}'.format(answer))

    # please keep below in the order types listed in survey.h
    if qtype == 'INT':
        return answer['value']

    if qtype == 'MULTICHOICE':
        return answer['text'].split(',')

    if qtype == 'MULTISELECT':
        return answer['text'].split(',')

    if qtype == 'LATLON':
        return [answer['lat'], answer['lon']]

    if qtype == 'DATETIME':
        return answer['time_begin']

    if qtype == 'DAYTIME':
        return answer['time_begin']

    if qtype == 'TIMERANGE':
        return [answer['time_begin'], answer['time_end']]

    if qtype == 'UPLOAD':
        raise ValueError('"{}" type is not supported!'.format(qtype))

    if qtype == 'TEXT':
        return answer['text']

    if qtype == 'CHECKBOX':
        return answer['text']

    if qtype == 'HIDDEN':
        return answer['text']

    if qtype == 'TEXTAREA':
        return answer['text']

    if qtype == 'EMAIL':
        return answer['text']

    if qtype == 'PASSWORD':
        return answer['text']

    if qtype == 'SINGLECHOICE':
        return answer['text']

    if qtype == 'SINGLESELECT':
        return answer['text']

    if qtype == 'FIXEDPOINT':
        return answer['value']

    if qtype == 'FIXEDPOINT_SEQUENCE':
        ret = []
        parts = answer['value'].split(',')
        for part in parts:
            val = float(part) # could raise ValueError
            ret.append(val)
        return ret

    if qtype == 'DAYTIME_SEQUENCE':
        ret = []
        parts = answer['value'].split(',')
        for part in parts:
            val = int(part) # could raise ValueError
            ret.append(val)
        return ret

    if qtype == 'DATETIME_SEQUENCE':
        ret = []
        parts = answer['value'].split(',')
        for part in parts:
            val = int(part) # could raise ValueError
            ret.append(val)
        return ret

    if qtype == 'DURATION24':
        return answer['value']

    if qtype == 'QTYPE_DIALOG_DATA_CRAWLER':
        return answer['text']

    raise ValueError('Unkown or unsupported question type "{}"'.format(qtype))


def set_answer_value(answer, value, question_type=None):
    """
    validate and set value for a given answer
    - backend answers include question type since surveysystem #359
    - public answers or legacy answers require question_type arg
    """
    qtype = None


    if 'type' in answer:
        qtype = answer['type']
    else:
        qtype = question_type

    if not qtype:
        raise ValueError('No type defined for anwser: {}'.format(answer))

    # pleasee keep below in the order types listed in survey.h
    if qtype == 'INT':
        answer['value'] = int(value)
        return answer

    if qtype == 'MULTICHOICE':
        if isinstance(value, list):
            answer['text'] = ','.join(value)
            return answer
        raise ValueError('"{}" answer needs to be an istance of list'.format(qtype))

    if qtype == 'MULTISELECT':
        if isinstance(value, list):
            answer['text'] = ','.join(value)
            return answer
        raise ValueError('"{}" answer needs to be an istance of list'.format(qtype))

    if qtype == 'LATLON':
        if isinstance(value, list) and len(value) == 2:
            answer['lat'] = int(value[0])
            answer['lon'] = int(value[1])
            answer['unit'] = 'degrees'
            return answer
        raise ValueError('"{}" answer needs to be an istance of list with exact 2 elements'.format(qtype))

    if qtype == 'DATETIME':
        answer['time_begin'] = int(value)
        answer['unit'] = 'seconds'
        return answer

    if qtype == 'DAYTIME':
        answer['time_begin'] = int(value)
        answer['unit'] = 'seconds'
        return answer

    if qtype == 'TIMERANGE':
        if isinstance(value, list) and len(value) == 2:
            answer['time_begin'] = int(value[0])
            answer['time_end'] = int(value[1])
            answer['unit'] = 'seconds'
            return answer
        raise ValueError('"{}" answer needs to be an istance of list with exact 2 elements'.format(qtype))

    if qtype == 'UPLOAD':
        raise ValueError('"{}" type is not supported!'.format(qtype))

    if qtype == 'TEXT':
        answer['text'] = str(value)
        return answer

    if qtype == 'CHECKBOX':
        answer['text'] = str(value)
        return answer

    if qtype == 'HIDDEN':
        answer['text'] = str(value)
        return answer

    if qtype == 'TEXTAREA':
        answer['text'] = str(value)
        return answer

    if qtype == 'EMAIL':
        answer['text'] = str(value)
        return answer

    if qtype == 'PASSWORD':
        answer['text'] = str(value)
        return answer

    if qtype == 'SINGLECHOICE':
        answer['text'] = str(value)
        return answer

    if qtype == 'SINGLESELECT':
        answer['text'] = str(value)
        return answer

    if qtype == 'FIXEDPOINT':
        answer['value'] = int(value)
        return answer

    if qtype == 'FIXEDPOINT_SEQUENCE':
        if isinstance(value, list):
            # validate list of  sorted ints
            if not _is_int_list_asc(value):
                raise ValueError('"{}" needs to be an ascending list of numbers!'.format(qtype))

            answer['text'] = ','.join(value)
            return answer
        raise ValueError('"{}" answer needs to be an istance of list with exact 2 elements'.format(qtype))

    if qtype == 'DAYTIME_SEQUENCE':
        if isinstance(value, list):
            # validate list of  sorted ints
            if not _is_int_list_asc(value):
                raise ValueError('"{}" needs to be an ascending list of numbers!'.format(qtype))

            answer['text'] = ','.join(value)
            answer['unit'] = 'seconds'
            return answer
        raise ValueError('"{}" answer needs to be an istance of list with exact 2 elements'.format(qtype))

    if qtype == 'DATETIME_SEQUENCE':
        if isinstance(value, list):
            # validate list of  sorted ints
            if not _is_int_list_asc(value):
                raise ValueError('"{}" needs to be an ascending list of numbers!'.format(qtype))

            answer['text'] = ','.join(value)
            answer['unit'] = 'seconds'
            return answer
        raise ValueError('"{}" answer needs to be an istance of list with exact 2 elements'.format(qtype))

    if qtype == 'DURATION24':
        answer['value'] = int(value)
        answer['unit'] = 'seconds'
        return answer

    if qtype == 'DIALOG_DATA_CRAWLER':
        answer['text'] = str(value)
        return answer

    raise ValueError('Unkown or unsupported question type "{}"'.format(qtype))


def set_answer(uid, qtype, value, ts=None):
    """
    creates an answer dict for a given uid, type and value
    """
    answer = answer_model()
    answer['uid'] = uid
    answer['type'] = qtype
    answer['stored'] = time.time() if ts == None else ts

    return set_answer_value(answer, value)


####
## question model
####


def question_model():
    model = {
        'uid': '',
        'question_text': '',
        'question_html': '',
        'type': '',
        'flags': 0,
        'default_value': '',
        'min_value': 0,
        'max_value': 0,
        'decimal_places': 0,
        'num_choices': 0,
        'choices': [],
        'unit': '',
    }
    return model

"""
Loads and parses a question dict from a line of a current file
returns None or an question dict
"""
def deserialise_question(line):
    question = question_model()
    parts = deserialise_csw_row(line)

    if len(parts) != len(question.keys()): # header or invalid
        return None

    question.update({
        'uid':              parts[0].strip(),
        'question_text':    parts[1].strip(),
        'question_html':    parts[2].strip(),
        'type':             parts[3].strip(),
        'flags':            0 if not parts[4].strip() else int(parts[4].strip()),
        'default_value':    parts[5].strip(),
        'min_value':        0 if not parts[6].strip() else int(parts[6].strip()),
        'max_value':        0 if not parts[7].strip() else int(parts[7].strip()),
        'decimal_places':   0 if not parts[8].strip() else int(parts[8].strip()),
        'num_choices':      0 if not parts[9].strip() else int(parts[9].strip()),
        #choices: see below
        'unit':             parts[11].strip()
    })

    ch = parts[10].strip()
    if not ch:
        question['choices'] = []
    else:
        question['choices'] = ch.split(',')
    return question


"""
 strips GROUPING convention from a given version 2 question id: i.e "ESS_1__10" becomes "ESS_1"
"""
def normalise_uid(key):
    index = key.rfind('__')
    # -1: NOT FOUND,
    # 0: edge case '__ID' prefix
    if index <= 0:
        return key
    return key[0:index]

"""
 strips GROUPING convention from a FLAT list of uids i.e ['ESS_1__10', 'ESS_2__10'] becomes ['ESS_1', 'ESS_2']
"""
def normalise_uids(uids):
    return list(map(lambda item: normalise_uid(item), uids))


#####
# time
#####


def hhmmss(hh, mm=0, ss=0):
    """
    converts time24 to seconds
    """
    return (hh * 3600) + (mm * 60) + ss
