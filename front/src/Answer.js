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
 #define QTYPE_FIXEDPOINT   2
 #define QTYPE_MULTICHOICE  3 // instruction for multi choice inputs (select), answer is a single choice or comma separated list
 #define QTYPE_MULTISELECT  4 // instruction for multi choice inputs (select), answer is a single choice or comma separated list
 #define QTYPE_LATLON       5
 #define QTYPE_DATETIME     6
 #define QTYPE_DAYTIME      7 // instruction for time of a generic day in seconds since midnight (answer->value)
 #define QTYPE_TIMERANGE    8 // instruction for time range within a generic day (answer->time_begin, answer->time_end)
 #define QTYPE_UPLOAD       9
 #define QTYPE_TEXT         10
 #define QTYPE_CHECKBOX     11 // instruction for single html checkbox, requires two defined choices in the following order: [OFF-value, ON-value]
 #define QTYPE_HIDDEN       12 // instruction for hidden input (pure textslide) answer is default value or default value
 #define QTYPE_TEXTAREA     13 // instruction for textarea.
 #define QTYPE_EMAIL        14 // instruction for email input
 #define QTYPE_PASSWORD     15 // instruction for (html) password input, this type can be used to mask any user input
 #define QTYPE_SINGLECHOICE 16 // instruction for single choice inputs (checkbox, radios), answer is a single choice
 #define QTYPE_SINGLESELECT 17 // instruction for single choice inputs (select), answer is a single choice
 #define QTYPE_UUID         18
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
 * @property {number} flag - Bitmask ANSWERED|DELETED

 * @see backend/src/question_types.c
 * @see backend/src/deserialise_parse_field.c
 */


////
// Casters
// @see backend/src/deserialise_parse_field.c
////


/**
 * Parse and validate a number
 * @param {any} val
 *
 * @returns {(number|Error)}
 */
const _number = function(val) {
    if (typeof val !== 'number') {
       return new Error ('CSV serializer: Invalid number: wrong type.');
    }

    const value = Number(val);
    if (isNaN(value) || !isFinite(value)) {
        return new Error ('CSV serializer: Invalid number value: is either NaN or Infinity.');
    }
    return value;
};

/**
 * Parse and validate a number
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
 * Parse and validate a number
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
 * Parse and validate a number
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
 * Parse and validate a number
 * @param {any} val
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
 * Parse and validate a number
 * @param {any} val
 *
 * @returns {(number|Error)}
 */
const _textFromArray = function(val) {
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
        ret[i] = (typeof item === 'string') ? item.replace(',' , '\,') : item;
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
    if (typeof value == 'string') {
        // Regex for sanitizing csv string values,
        // example build: console.log(new RegExp([CSV_SEPARATOR, '\'', '"'].join(''), 'g'));
        return value.trim().replace(new RegExp(`:'"`, 'g'), '\\$&');
    }
    return value;
};

/**
 * Provides default Model object
 * @returns {AnswerModel}
 */
const model = function() {
    return {
        uid : '',               // id
        value : 0.0,            // type: number (FLOAT)
        text : '',              // type: text
        lat : 0.0,              // type: number (FLOAT)
        lon : 0.0,              // type: number (FLOAT)
        time_begin : 0,         // type: number (INT) UNIX timestamp in seconds
        time_end : 0,           // type: number (INT) UNIX timestamp in seconds
        time_zone_delta : 0,    // type: number (INT) seconds
        dst_delta : 0,          // type: number (INT) => unit
        unit: '',               // type: text
        flag: 0,                // bitmask
    };
};

const modelKeysSorted = Object.keys(model()).sort().toString();

/**
* Get Proptypes schema
* @returns {PropTypes}
*/
const propTypes = function () {
    return PropTypes.shape({
        uid: PropTypes.string,
        value: PropTypes.number,
        text: PropTypes.string,
        lat: PropTypes.number,
        lon: PropTypes.number,
        time_begin: PropTypes.number,
        time_end: PropTypes.number,
        time_zone_delta: PropTypes.number,
        dst_delta: PropTypes.number,
        unit: PropTypes.string,
        flag: PropTypes.number,
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
            // number
            answer.value = _number(value);
        break;

        case 'MULTICHOICE':
        case 'MULTISELECT':
            answer.text = _textFromArray(value);
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
        case 'QTYPE_UUID': // TODO no component
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

export default {
    model,
    propTypes,
    serialize,
    setValue,
};
