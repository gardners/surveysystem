/**
 * @module surveyJS answer object to backend csv row parser
 *
 */
import PropTypes from 'prop-types';

import { isArray, isScalar } from './Utils';
const { isFinite, isInteger } = Number;

/*
 Notes:
 @see backend/include/survey.h
 @see backend/src/serialisers.c

 #define QTYPE_INT          1
#define QTYPE_FIXEDPOINT          2
#define QTYPE_MULTICHOICE         3  // answer->text (comma separated): instruction for multi choice inputs (checkbox), answer is a single choice or comma separated list.
#define QTYPE_MULTISELECT         4  // answer->text (comma separated): instruction for multi choice inputs (select), answer is a single choice or comma separated list. (unit: *)
#define QTYPE_LATLON              5  // answer->lat & answer->lon: instruction for geographic coordinate ([-90 +90] & [-180 +180]). (unit: degrees)
#define QTYPE_DATETIME            6  // answer->time_begin: instruction for UNIX datetime (+-). (unit: seconds)
#define QTYPE_DAYTIME             7  // answer->time_begin: instruction for time of a generic day in seconds since midnight. (unit: seconds)
#define QTYPE_TIMERANGE           8  // answer->time_begin & answer->time_end: instruction for time range within a generic day. (unit: seconds)
#define QTYPE_UPLOAD              9
#define QTYPE_TEXT                10 // answer->text: instruction for generic text. (unit: *)
#define QTYPE_CHECKBOX            11 // answer->text: instruction for single html checkbox, requires two defined choices in the following order: [OFF-value, ON-value]. (unit: *)
#define QTYPE_HIDDEN              12 // answer->text: instruction for hidden input (pure textslide) answer is default value or default value. (unit: *)
#define QTYPE_TEXTAREA            13 // answer->text: instruction for textarea. (unit: *)
#define QTYPE_EMAIL               14 // answer->text: instruction for email input. (unit: *)
#define QTYPE_PASSWORD            15 // answer->text: instruction for (html) password input, this type can be used to mask any user input. (unit: *)
#define QTYPE_SINGLECHOICE        16 // answer->text: instruction for single choice inputs (checkbox, radios), answer is a single choice. (unit: *)
#define QTYPE_SINGLESELECT        17 // answer->text: instruction for single choice inputs (select), answer is a single choice. (unit: *)
#define QTYPE_FIXEDPOINT_SEQUENCE 18 // answer->text (comma separated): instruction an ascending sequence of FIXEDPOINT values, labels are defined in q.choices. (unit: *)
#define QTYPE_DAYTIME_SEQUENCE    19 // answer->text (comma separated): instruction an ascending sequence of DAYTIME values (comma separated), labels are defined in q.choices. (unit: seconds)
#define QTYPE_DATETIME_SEQUENCE   20 // answer->text (comma separated): instruction an ascending sequence of DATETIME values (comma separated), labels are defined in q.choices (unit: seconds)
#define QTYPE_UUID 21
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
 * @property {string} unit - answer unit

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
    if (!isInteger(val)) { // checks also infini &, type
        return new Error ('CSV serializer: Invalid timestamp value: not an integer.');
    }

    if (val < 0) {
        return new Error ('CSV serializer: Invalid timestamp value: negative number.');
    }
    return val;
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
    return (val === null) ? '' : val;
};

/**
 * Parse, validate and serialize an array of strings
 * @param {any} val
 *
 * @returns {(number|Error)}
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
 * @returns {(number|Error)}
 */
const _serializedNumberArraySequence = function(val) {
    if (!isArray(val)) {
        return new Error ('CSV serializer: Invalid value: expected array.');
    }

    let item;
    let prev = 0;
    const ret = [];

    for (let i = 0; i < val.length; i += 1) {
        item = _number(val[i]);
        if(item instanceof Error) {
            return item;
        }

        if(item < prev) {
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
        .replace(/\:|'|"/g, '\\$&')
        .replace(/(?:\r\n|\r|\n)/g, ' ');
        /* eslint-enable no-useless-escape */
    }
     /* eslint-enable no-useless-escape */
    return value;
};

/**
 * Provides default Model object
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
        unit: '',               // type: text
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
        unit: question.unit,
    });
};

/**
* Get Proptypes schema
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
        unit: PropTypes.string,
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
    answer.unit = question.unit || ''; // might be overwritten below

    switch(type) {
        case 'INT':
        case 'FIXEDPOINT':
        case 'DURATION24':
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
            } else {
                answer.lat = _latitude(value[0]);
                answer.lon = _longitude(value[1]);
                answer.unit = 'degrees';
            }
        break;

        case 'DATETIME':
            // UNIX timestamp in seconds
            answer.time_begin = _timestamp(value);
            answer.unit = 'seconds';
        break;

        case 'DAYTIME':
            // number in seconds
            answer.time_begin = _timestamp(value);
            answer.unit = 'seconds';
        break;

        case 'TIMERANGE':
            // number in seconds
            if (!isArray(value) || value.length !== 2) {
                answer.time_begin = new Error('CSV serializer: TIMERANGE value requires an array of two timestamps');
                break;
            } else {
                answer.time_begin = _timestamp(value[0]);
                answer.time_end = _timestamp(value[1]);
                answer.unit = 'seconds';
            }
        break;

        case 'DAYTIME_SEQUENCE':
            // array of numbers in seconds
            answer.text = _serializedNumberArraySequence(value);
        break;

        case 'UPLOAD':
            answer.text = new Error(`CSV serializer: unsupported question type: ${type}`);
        break;

        case 'TEXT':
        case 'HIDDEN':
        case 'TEXTAREA':
        case 'EMAIL':
        case 'PASSWORD':
        case 'CHECKBOX':
        case 'SINGLECHOICE':
        case 'SINGLESELECT':
        case 'DIALOG_DATA_CRAWLER':
        case 'UUID': // TODO no component
            // string
            answer.text = _text(value);
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
        case 'TEXT':
        case 'HIDDEN':
        case 'TEXTAREA':
        case 'EMAIL':
        case 'PASSWORD':
        case 'CHECKBOX':
        case 'SINGLECHOICE':
        case 'SINGLESELECT':
        case 'DIALOG_DATA_CRAWLER':
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
    setValue,
    getValue,
};

export { Answer as default, sanitizeValue };
