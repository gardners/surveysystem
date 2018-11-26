/**
 * @module surveyJS answer object to backend csv row parser
 *
 */

const { isFinite } = Number; //not Math!

/*
 Notes:
    @see backend/src/question_types.c
    @see backend/src/deserialise_parse_field.c

    #include "question_types.h"
    char *question_type_names[1+NUM_QUESTION_TYPES+1]={
    "start of list",
    "INT",
    "FIXEDPOINT",
    "MULTICHOICE",
    "MULTISELECT",
    "LATLON",
    "DATETIME",
    "TIMERANGE",
    "UPLOAD",
    "TEXT",
    "UUID",
    "end of list"};

    DESERIALISE_STRING(a->uid);
    DESERIALISE_STRING(a->text);
    DESERIALISE_LONGLONG(a->value);
    DESERIALISE_LONGLONG(a->lat);
    DESERIALISE_LONGLONG(a->lon);
    DESERIALISE_LONGLONG(a->time_begin);
    DESERIALISE_LONGLONG(a->time_end);
    DESERIALISE_INT(a->time_zone_delta);
    DESERIALISE_INT(a->dst_delta);
*/

/**
 * Csv separator
 * @const
 * @type {string}
 */
const CSV_SEPARATOR = ':';

/**
 * Regex for sanitizing csv string values
 * @const
 * @type {sRegExp}
 */
const escPattern = new RegExp(`${CSV_SEPARATOR}'"`, 'g');
// example build: console.log(new RegExp([CSV_SEPARATOR, '\'', '"'].join(''), 'g'));


////
// Defaults
////

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

/**
 * Provides default Model object
 * returns {AnswerModel}
 */
const getModel = function() {
    return {
        uid : '',               // id
        text : '',              // type: text
        value : 0.0,            // type: number (FLOAT)
        lat : 0.0,              // type: number (FLOAT)
        lon : 0.0,              // type: number (FLOAT)
        time_begin : 0,         // type: number (INT)
        time_end : 0,           // type: number (INT)
        time_zone_delta : 0,    // type: number (INT)
        dst_delta : 0,          // type: number (INT)
    };
};

////
// Sanitizers
////

/**
 * CSV Sanitize a value of any type
 * @param {*} val value to sanitize
 *
 * @returns {(string|number|boolean|null)}
 */
const sanitize = function(val) {
    const type = typeof val;
    if( val === null || type === 'number' || type === 'boolean') {
        return val;
    }
    if(type !== 'string') {
        // TODO !isScalar => throw
        return JSON.stringify(val);
    }
    return val
        .trim()
        .replace(escPattern, '\\$&')
     ;
};

////
// Casters
// @see backend/src/deserialise_parse_field.c
////

/**
 * parse and validate string
 * @param {string} val already sanitized string
 *
 * @returns {(number|Error)}
 */
const castFloat = function(val) {
    const value = parseFloat(val);

    if(isNaN(value)) {
        return new Error ('CSV serializer: Invalid value: is not a Number');
    }

    if(!isFinite(value)) {
        return new Error ('CSV serializer: Invalid value: is infinite');
    }
    return value;
};

/**
 * Parse and validate string
 * @param {string} val already sanitized string
 *
 * @returns {(sting|Error)}
 */
const castString = function(text) {
    if (!text) {
        return new Error('CSV serializer: Submitted answer (type: text) is empty)');
    }
    return text;
};

/**
 * Parse and validate comma separated geolocation string into object ready to be merged in to model
 * @param {string} val already sanitized string
 *
 * @returns {(Array|Error)} Error on invalid value or an array of format [lat, lon]
 */
const castGeoLocObject = function(val) {
    if(typeof val !== 'string') {
        return new Error ('CSV serializer: Invalid geolocation data type');
    }

    const values = val.split(',');
    if(values.length !== 2) {
        return new Error ('CSV serializer: Invalid geolocation data');
    }
    const lat = parseFloat(values[0]);
    const lon = parseFloat(values[1]);

    if(isNaN(lat) || isNaN(lon)) {
        return new Error ('CSV serializer: Invalid geolocation data, lat or lon is not a Number');
    }

    return {
        lat,
        lon,
    };
};

/**
 * parse and validate comma separated geolocation string into object ready to be merged in to model
 * @param {string} val already sanitized string
 *
 * @returns {(number|Error)} UNIX timestamp or error
 */
// const castDateTime = function(val) {
//     return new Error ('TODO, CSV serializer: not implemented');
// };

////
// Serializers
////

/**
 * Parses a csv row from a SurveyJS answer instance
 * example format of a row: "question1:gfdsg:0:0:0:0:0:0:0"
 *
 * @param {string} id question id
 * @param {*} answer answer value to submit
 * @param {string} questionType
 * @returns {{string|Error)}  csv row or Error to be displayed
 */
const serializeAnswer = function(id, answer, questionType) {

    const model = getModel();
    const sanitized = sanitize(answer);

    let key;
    let value;

    // assign id
    model.uid = id;

    // cast values, define keys
    switch (questionType) {
        case 'number':
            key = 'value';
            value = castFloat(sanitized);
        break;

        // @see customQuestions/*.js
        case 'geolocation':
            key = '<geoloc>';
            value = castGeoLocObject(sanitized);
        break;

        // @TODO
        // case 'timepicker':
        //     value = castDateTime(sanitized);
        // break;

        default:
            key = 'text';
            value = castString(sanitized);
    }

    // handle errors
    if (!key) {
        return new Error('CSV serializer: Unknown field key');
    }

    if (value instanceof Error) {
        return value;
    }

    // build
    if(key === '<geoloc>') {
        // complex
        Object.assign(model, value);
    } else {
        // scalars
        model[key] = value;
    }

    return Object.values(model).join(CSV_SEPARATOR);
};

export { serializeAnswer, CSV_SEPARATOR };
