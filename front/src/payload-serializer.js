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
 * @returns {AnswerModel}
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

/**
 * Maps backend question types to corresponding model fields
 * @property {string} questionType
 * @returns {string[]}
 */
const mapTypeToField = function(questionType) {
    switch(questionType) {

        case 'TEXT':
        case 'MULTICHOICE':
        case 'MULTISELECT':
        case 'text': // TODO, legacy
        case 'radiobuttons': // TODO, legacy
        case 'checkbox': // TODO, legacy
            return (text) => {
                if (Object.prototype.toString.call(text) === '[object Array]') {
                    text = text.map(v => v.replace(',' , '\,')).join(',');
                }

                return {
                    text,
                };
            };

        case 'INT':
        case 'FIXEDPOINT':
        case 'fixedpoint': // TODO, legacy
            return (value) => ({
                value,
            });

        case 'LATLON':
            return (lat, lon) => ({
                lat,
                lon,
            });

        case 'DATETIME': //TODO fcgimain.c puts it into "text" clarify
        case 'date': // TODO, legacy
            return (time_begin, time_zone_delta) => ({
                time_begin,
                time_zone_delta,
            });

        case 'TIMERANGE':
            return (time_begin, time_end, time_zone_delta = 0) => ({
                time_begin,
                time_end,
                time_zone_delta,
            });

        // case 'UPLOAD': //TODO

        default:
         // nothing
    }

    return new Error(`Unkown question type: ${questionType}`);
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
const castInt = function(val) {
    const value = parseInt(val, 10);

    if(isNaN(value)) {
        return new Error ('CSV serializer: Invalid value: is not a Number');
    }

    if(!isFinite(value)) {
        return new Error ('CSV serializer: Invalid value: is infinite');
    }
    return value;
};

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
 * Merges (mutates) an answer value in a model
 *
 * @param {AnswerModel} model
 * @param {string} id question id
 * @param {*} answer answer value to submit
 * @param {string} questionType
 * @returns {AnswerModel}  csv row or Error to be displayed
 */
const mergeAnswerValue = function(model, id, answer, questionType) {

    const sanitized = sanitize(answer);

    let key = '';
    let value;

    // assign id
    model.uid = id;

    // cast values, define keys
    switch (questionType) {
        case 'text':
            key = questionType;
            value = castString(sanitized);
        break;

        case 'value':
            key = 'value';
            value = castFloat(sanitized);
        break;

        case 'lat':
            key = questionType;
            value = castFloat(sanitized);
        break;

        case 'lon':
            key = questionType;
            value = castFloat(sanitized);
        break;

        case 'time_begin':
            key = questionType;
            value = castFloat(sanitized);
        break;

        case 'time_end':
            key = questionType;
            value = castFloat(sanitized);
        break;

        case 'time_zone_delta':
            key = questionType;
            value = castInt(sanitized);
        break;

        case 'dst_delta':
            key = questionType;
            value = castInt(sanitized);
        break;

        // @TODO
        // case 'timepicker':
        //     value = castDateTime(sanitized);
        // break;
    }

    // handle errors
    if (!key) {
        return new Error('CSV serializer: Unknown field key');
    }

    if (value instanceof Error) {
        return value;
    }

    // build
    model[key] = value;

    return model;
};

/**
 * Parses a csv row from a SurveyJS answer instance
 * example format of a row: "question1:gfdsg:0:0:0:0:0:0:0"
 *
 * @param {string} id question id
 * @param {*} answer answer value to submit
 * @param {string} questionType
 * @returns {{string|Error)}  csv row or Error to be displayed
 */
const serializeAnswerValue = function(id, answer, questionType) {
    const model = mergeAnswerValue(getModel(), id, answer, questionType);
    return (model instanceof Error) ? model : Object.values(model).join(CSV_SEPARATOR);
};

/**
 * Parses a csv row from a SurveyJS answer instance
 * example format of a row: "question1:gfdsg:0:0:0:0:0:0:0"
 *
 * @param {string} id question id
 * @param {object} answer answer object where keys are matching a model properites and values is the answer value
 * @returns {{string|Error)}  csv row or Error to be displayed
 */
const serializeAnswer = function (id, answer) {
    let model = getModel();
    let key;
    let res;

    if(Object.prototype.toString.call(answer) !== '[object Object]') {
        return new Error('Missing answer');
    }

    for (key in answer) {
        if(answer.hasOwnProperty(key)) {
            model = mergeAnswerValue(model, id, answer[key], key);
            if(model instanceof Error) {
                return model;
            }
        }
    }

    return Object.values(model).join(CSV_SEPARATOR);
};

export { serializeAnswer, serializeAnswerValue, mapTypeToField, CSV_SEPARATOR };
