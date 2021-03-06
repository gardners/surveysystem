/**
 * @module answer object to backend csv row parser
 */

import PropTypes from 'prop-types';

import { isArray, isScalar } from './Utils';
const { isFinite, isInteger } = Number;

/*
 Notes:
 @see backend/include/survey.h
 @see backend/src/serialisers.c

#define QTYPE_META                1 // internal sytem answer
#define QTYPE_INT                 2 // Answer is an integer, bounded by min_value and max_value
#define QTYPE_FIXEDPOINT          3  // Answer is a fixed point value encoded as a 64-bit integer
#define QTYPE_MULTICHOICE         4  // answer->text (comma separated): instruction for multi choice inputs (checkbox), answer is a single choice or comma separated list.
#define QTYPE_MULTISELECT         5  // answer->text (comma separated): instruction for multi choice inputs (select), answer is a single choice or comma separated list. (unit: *)
#define QTYPE_LATLON              6  // answer->lat & answer->lon: instruction for geographic coordinate ([-90 +90] & [-180 +180]). (unit: degrees)
#define QTYPE_DATETIME            7  // answer->time_begin: instruction for UNIX datetime (+-). (unit: seconds)
#define QTYPE_DAYTIME             8  // answer->time_begin: instruction for time of a generic day in seconds since midnight. (unit: seconds)
#define QTYPE_TIMERANGE           9  // answer->time_begin & answer->time_end: instruction for time range within a generic day. (unit: seconds)
#define QTYPE_UPLOAD              10
#define QTYPE_TEXT                11 // answer->text: instruction for generic text. (unit: *)
#define QTYPE_CHECKBOX            12 // answer->text: instruction for single html checkbox, requires two defined choices in the following order: [OFF-value, ON-value]. (unit: *)
#define QTYPE_HIDDEN              13 // answer->text: instruction for hidden input (pure textslide) answer is default value or default value. (unit: *)
#define QTYPE_TEXTAREA            14 // answer->text: instruction for textarea. (unit: *)
#define QTYPE_EMAIL               15 // answer->text: instruction for email input. (unit: *)
#define QTYPE_SINGLECHOICE        17 // answer->text: instruction for single choice inputs (checkbox, radios), answer is a single choice. (unit: *)
#define QTYPE_SINGLESELECT        18 // answer->text: instruction for single choice inputs (select), answer is a single choice. (unit: *)
#define QTYPE_FIXEDPOINT_SEQUENCE 19 // answer->text (comma separated): instruction an ascending sequence of FIXEDPOINT values, labels are defined in q.choices. (unit: *)
#define QTYPE_DAYTIME_SEQUENCE    20 // answer->text (comma separated): instruction an ascending sequence of DAYTIME values (comma separated), labels are defined in q.choices. (unit: seconds)
#define QTYPE_DATETIME_SEQUENCE   21 // answer->text (comma separated): instruction an ascending sequence of DATETIME values (comma separated), labels are defined in q.choices (unit: seconds)
#define QTYPE_DURATION24          22 // answer->value: instruction time period in seconds maximum 24 hours (86400 seconds), TODO enable min_value and max_value support, which would make this type redundant in favour of a more generic QTYPE_DURATION type.
#define QTYPE_DIALOG_DATA_CRAWLER 23 // answer->text: instruction for a dialog to give consent to accessing external data requires two defined choices in the following order: [DENIED, GRANTED], (unit: id of the data crawler module)
#define QTYPE_SHA1_HASH           24 // submitted answer->text is converted and stored as sha1 hash
#define QTYPE_UUID                25
*/

/**
 * @typedef AnswerModel
 * @type {object}
 * @property {string} uid - answer id.
 * @property {string} text - answer, type: text.
 * @property {number} value - answer, type: number (FLOAT)
 * @property {number} lat - geolocation answer (latitude), type: number (FLOAT)
 * @property {number} lon - geolocation answer (longitude), type: number (FLOAT)
 * @property {number} time_begin - timeperiod answer (start) in seconds, type: number (INT)
 * @property {number} time_end - timeperiod answer (end) in seconds, type: number (INT)
 * @property {number} time_zone_delta - timeperiod timzone offset in seconds, type: number (INT)
 * @property {number} dst_delta - distance(?), type: number (INT)

 * @see backend/src/question_types.c
 * @see backend/src/deserialise_parse_field.c
 */

////
// Casters
// @see backend/src/deserialise_parse_field.c
////


/**
 * Parse and validate a number value
 * @param {any} val
 *
 * @returns {(number|Error)}
 */
const _number = function(val) {
    if (typeof val !== 'number' && typeof val !== 'string') {
       return new Error ('CSV serializer: Invalid number: wrong type.');
    }
    if (val === '') {
       return new Error ('CSV serializer: Invalid number: wempty string.');
    }

    const value = Number(val);
    if (isNaN(value) || !isFinite(value)) {
        return new Error ('CSV serializer: Invalid number value: is either NaN or Infinity.');
    }
    return value;
};

/**
 * Parse and validate a longitude position
 * @param {any} val
 *
 * @returns {(number|Error)}
 */
const _longitude = function(val) {
    const value =_number(val);
    if (value instanceof Error) {
        return value;
    }
    if(Math.abs(value) > 180) {
        return new Error ('CSV serializer: Invalid longitude value: out of bounds.');
    }
    return value;
};

/**
 * Parse and validate a latitude position
 * @param {any} val
 *
 * @returns {(number|Error)}
 */
const _latitude = function(val) {
    const value =_number(val);
    if (value instanceof Error) {
        return value;
    }
    if(Math.abs(value) > 90) {
        return new Error ('CSV serializer: Invalid latitude: out of bounds.');
    }
    return value;
};

/**
 * Parse and validate a timestamp value
 * @param {any} val
 *
 * @returns {(number|Error)}
 */
const _timestamp = function(val) {
    const value =_number(val);
    if (!isInteger(value)) { // checks also infini &, type
        return new Error ('CSV serializer: Invalid timestamp value: not an integer.');
    }

    if (value < 0) {
        return new Error ('CSV serializer: Invalid timestamp value: negative number.');
    }
    return value;
};

/**
 * Parse and validate a text value
 *
 * @returns {(number|Error)}
 */
const _text = function(val) {
    if (val === null || !isScalar(val)) {
        return new Error ('CSV serializer: Invalid text value: is either null or undefined.');
    }
    return val;
};

/**
 * Parse and validate a uuid pattern
 * valid uuids must be lowercase (see backend validator.c -> validate_session_id())
 * @param {any} val
 *
 * @returns {(string|Error)}
 */
const _uuid = function(val) {
    if (typeof val !== 'string'|| !val) {
        return new Error ('CSV serializer: Invalid text value: not a string or empty.');
    }

    const ex = new RegExp('^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$');
    if (!ex.test(val)) {
        return new Error ('CSV serializer: Invalid uuid value');
    }
    return val;
};

/**
 * Parse, validate and serialize an array of strings
 * @param {any} val
 *
 * @returns {(string|Error)}
 */
const _serializedStringArray = function(val) {
    if (!isArray(val)) {
        return new Error ('CSV serializer: Invalid value: expected array.');
    }

    let item;
    const ret = [];

    for (let i = 0; i < val.length; i += 1) {
        item = _text(val[i]);
        if(item instanceof Error) {
            return item;
        }
        // eslint-disable-next-line no-useless-escape
        ret[i] = (typeof item === 'string') ? item.replace(',' , '\,') : item;
    }
    return ret.toString();
};

/**
 * Parse and validate an ascending sequence of numbers
 * @param {any} val
 *
 * @returns {(string|Error)}
 */
const _serializedNumberArraySequence = function(val) {
    if (!isArray(val)) {
        return new Error ('CSV serializer: Invalid value: expected array.');
    }
    if (!val.length) {
        return new Error ('CSV serializer: Invalid value: emptyarray.');
    }

    let item;
    let prev;
    const ret = [];

    for (let i = 0; i < val.length; i += 1) {
        item = _number(val[i]);
        if(item instanceof Error) {
            return item;
        }

        if(i && item < prev) {
            return new Error (`CSV serializer: Invalid value for sequence item ${i}: smaller than previous item.`);
        }
        prev = item;
        ret.push(item);
    }
    return ret.toString();
};

/**
 * CSV Sanitize a value of any type
 * @param {*} val value to sanitize
 *
 * @returns {(string|number|boolean|null)}
 */
const sanitizeValue = function(value) {
    /* eslint-disable no-useless-escape */
    if (typeof value == 'string') {
        // Regex for sanitizing csv string values,
        // example build: console.log(new RegExp([':', '\'', '"'].join(''), 'g'));

        /* eslint-disable no-useless-escape */
        return value.trim()
        .replace(/\\:|'|"/g, '\$&')
        .replace(/(?:\r\n|\r|\n)/g, ' ');
        /* eslint-enable no-useless-escape */
    }
     /* eslint-enable no-useless-escape */
    return value;
};

/**
 * Provides default Model object
 * #448 remove 'unit' from public answer
 * @returns {AnswerModel}
 */
const model = function() {
    return {
        uid : '',               // id
        text : '',              // type: text
        value : 0.0,            // type: number (FLOAT)
        lat : 0.0,              // type: number (FLOAT)
        lon : 0.0,              // type: number (FLOAT)
        time_begin : 0,         // type: number (INT) UNIX timestamp in seconds
        time_end : 0,           // type: number (INT) UNIX timestamp in seconds
        time_zone_delta : 0,    // type: number (INT) seconds
        dst_delta : 0,          // type: number (INT) => unit
    };
};

const modelKeysSorted = Object.keys(model()).sort().toString();

/**
 * Create an initial answer
 * @returns {AnswerModel}
 */
const create = function(question) {
    if (question.default_value) {
        return setValue(question, question.default_value);
    }

    return Object.assign(model(), {
        uid : question.id,
    });
};

/**
* Get Proptypes schema
* #448 remove 'unit' from public answer
* @returns {PropTypes}
*/
const propTypes = function () {
    return PropTypes.shape({
        uid: PropTypes.string,
        text: PropTypes.string,
        value: PropTypes.number,
        lat: PropTypes.number,
        lon: PropTypes.number,
        time_begin: PropTypes.number,
        time_end: PropTypes.number,
        time_zone_delta: PropTypes.number,
        dst_delta: PropTypes.number,
    });
};

/**
 * Parses a comma-separated csv row from an mapped answer object
 */
const serialize = function(answer) {
    answer = answer || {};
    const { uid } = answer;

    if(Object.keys(answer).sort().toString() !== modelKeysSorted) {
        return new Error('CSV serializer: invalid answer object');
    }

    if(typeof uid !== 'string' || !uid) {
        return new Error('CSV serializer: Missing answer uid');
    }

    const values = Object.keys(answer).map(key => sanitizeValue(answer[key]));
    return values.join(':');
};

/**
 * Deserializes a answer string, public properties only!
 * @returns {Answer}
 */
const deserialize = function(fragment) {
    if (typeof fragment !== 'string') {
        return new Error('Answer csv fragment is not a string');
    }

    const answer = model();
    // filter out escaped column chars, no lookbehinds supported in javascript regex
    const parts = fragment.replace(/\\:/g, '[colon]').split(':').map(part => part.replace('[colon]', ':'));

    if (parts.length < Object.keys(answer).length) {
        return new Error(`Fragment parts don't match the required length of ${Object.keys(answer).length}, ${parts.length} given: ${fragment}`);
    }

    // TODO validate uid? no rules in backend
    if (!parts[0]) {
        return new Error(`Answer csv fragment is missing uid: ${fragment}`);
    }

    answer.uid              = _text(parts[0]);
    answer.text             = _text(parts[1]);
    answer.value            = _number(parts[2]);
    answer.lat              = _latitude(parts[3]);
    answer.lon              = _longitude(parts[4]);
    answer.time_begin       = _number(parts[5]);
    answer.time_end         = _number(parts[6]);
    answer.time_zone_delta  = _number(parts[7]);
    answer.dst_delta        = _number(parts[8]);

    const errors = Object.keys(answer).filter(key => answer[key] instanceof Error);
    if(errors.length) {
        return new Error(`Answer "${fragment}" serialisation failed with errors for following properties: [${errors.toString()}]`);
    }

    return answer;
};

/**
 * Parses a comma-separated csv row from an mapped answer object
 * @param {QuestionModel} question
 * @param {mixed} value form value
 *
 * @returns {AnswerModel|Error}
 */
const setValue = function(question, value) {
    question = question || {};
    const { id, type } = question;

    if (typeof id !== 'string' || !id) {
        return new Error('CSV serializer: question requires property "id"');
    }

    if (typeof type !== 'string' || !type) {
        return new Error('CSV serializer: question requires property "type"');
    }

    const answer = model();
    answer.uid = question.id;

    switch(type) {
        case 'INT':
        case 'FIXEDPOINT':
            // number
            answer.value = _number(value);
        break;

        case 'MULTICHOICE':
        case 'MULTISELECT':
            if (typeof value === 'string'){
                answer.text = _serializedStringArray(value.split(','));
                break;
            }
            answer.text = _serializedStringArray(value);
        break;

        case 'LATLON':
            // array [lat, lon]
            if (!isArray(value) || value.length !== 2) {
                answer.lat = new Error('CSV serializer: LATLON value requires an array of two numbers');
                break;
            }
            answer.lat = _latitude(value[0]);
            answer.lon = _longitude(value[1]);
        break;

        case 'DATETIME':
            // UNIX timestamp in seconds
            answer.time_begin = _timestamp(value);
        break;

        case 'DAYTIME':
            // number in seconds
            answer.time_begin = _timestamp(value);
        break;

        case 'TIMERANGE':
            // array of 2 numbers in seconds
            if (!isArray(value) || value.length !== 2) {
                answer.time_begin = new Error('CSV serializer: TIMERANGE value requires an array of two timestamps');
                break;
            } else {
                answer.time_begin = _timestamp(value[0]);
                answer.time_end = _timestamp(value[1]);
            }
        break;

        case 'FIXEDPOINT_SEQUENCE':
            // array of numbers
            answer.text = _serializedNumberArraySequence(value);
        break;

        case 'DAYTIME_SEQUENCE':
        case 'DATETIME_SEQUENCE':
            // #336, array of numbers in seconds
            answer.text = _serializedNumberArraySequence(value);
        break;

        case 'DURATION24':
            // #336, number (seconds)
            answer.value = _number(value);
        break;

        case 'TEXT':
        case 'HIDDEN':
        case 'TEXTAREA':
        case 'EMAIL':
        case 'CHECKBOX':
        case 'SINGLECHOICE':
        case 'SINGLESELECT':
        case 'DIALOG_DATA_CRAWLER':
        case 'SHA1_HASH':
            // string
            answer.text = _text(value);
        break;

        case 'UUID':
            // uuid
            answer.text = _uuid(value);
        break;

        default:
            answer.text = new Error(`CSV serializer: unsupported question type: ${type}`);
    }

    // scan answer properties for Errors
    for (let key in answer) {
        if (answer.hasOwnProperty(key)) {
            if (answer[key] instanceof Error) {
                return answer[key];
            }
        }
    }

    return answer;
};

/**
 * Parses a comma-separated csv row from an mapped answer object
 * @param {QuestionModel} question
 * @param {mixed} value form value
 *
 * @returns {AnswerModel|Error}
 */
const getValue = function(question, answer) {
    const { type } = question;

    if(!answer) {
        answer = create(question);
    }

    if(answer instanceof Error) {
        console.log(`Answer: question is uncaught Error: ${answer.message}`);
        answer = create(question);
    }

    switch(type) {
        case 'INT':
        case 'FIXEDPOINT':
            return answer.value;
        case 'MULTICHOICE':
        case 'MULTISELECT':
            return answer.text.split(',');
        case 'LATLON':
            return [answer.lat, answer.lon];
        case 'DATETIME':
            return answer.time_begin;
        case 'DAYTIME':
            return answer.time_begin;
        case 'TIMERANGE':
            return [answer.time_begin, answer.time_end];
        case 'FIXEDPOINT_SEQUENCE':
            return answer.text.split(',').map(part => parseFloat(part));
        case 'DAYTIME_SEQUENCE':
        case 'DATETIME_SEQUENCE':
            return answer.text.split(',').map(part => parseInt(part, 10));
        case 'QTYPE_DURATION24':
            return answer.value;
        case 'TEXT':
        case 'HIDDEN':
        case 'TEXTAREA':
        case 'EMAIL':
        case 'CHECKBOX':
        case 'SINGLECHOICE':
        case 'SINGLESELECT':
        case 'DIALOG_DATA_CRAWLER':
        case 'SHA1_HASH':
        case 'UUID': // TODO no component
            return answer.text;
        default:
            console.error(`Answer: unsupported question type: ${type}`);
            return null;
    }
};

const Answer = {
    model,
    create,
    propTypes,
    serialize,
    deserialize,
    setValue,
    getValue,
};

export { Answer as default, sanitizeValue };
