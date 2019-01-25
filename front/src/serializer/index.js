/**
 * @module surveyJS answer object to backend csv row parser
 *
 */
import { isArray } from '../Utils';

const { isFinite } = Number; //not Math!

/*
 Notes:
    @see backend/include/survey.h
    @see backend/src/deserialise_parse_field.c

 #define QTYPE_INT         1
 #define QTYPE_FIXEDPOINT  2
 #define QTYPE_MULTICHOICE 3
 #define QTYPE_MULTISELECT 4
 #define QTYPE_LATLON      5
 #define QTYPE_DATETIME    6
 #define QTYPE_TIMERANGE   7
 #define QTYPE_UPLOAD      8
 #define QTYPE_TEXT        9
 #define QTYPE_CHECKBOX    10
 #define QTYPE_HIDDEN      11
 #define QTYPE_TEXTAREA    12
 #define QTYPE_EMAIL       13
 #define QTYPE_PASSWORD    14
 #define QTYPE_UUID        15


    DESERIALISE_STRING(a->uid);
    DESERIALISE_STRING(a->text);
    DESERIALISE_LONGLONG(a->value);
    DESERIALISE_LONGLONG(a->lat);
    DESERIALISE_LONGLONG(a->lon);
    DESERIALISE_LONGLONG(a->time_begin);
    DESERIALISE_LONGLONG(a->time_end);
    DESERIALISE_INT(a->time_zone_delta);
    DESERIALISE_INT(a->dst_delta);
    DESERIALISE_STRING(a->unit);
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
 * @type {RegExp}
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
 * @property {string} unit - answer unit

 * @see backend/src/question_types.c
 * @see backend/src/deserialise_parse_field.c
 */

/**
 * @typedef QuestionModel
 * @type {object}
 * @property {string} id
 * @property {string} name
 * @property {string} title
 * @property {string} title_text
 * @property {string} type
 * @property {string} unit
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
        unit: '',               // type: text
    };
};

//<uid>:<value>:<TYPE>:<UNIT>
//questionid:value|value1:type:unit
//questionid:50.00|60.5:float:latlon
//questionid:value|value1:type:unit
//questionid:50.00|60.5:float:latlon

/**
 * Maps backend question types to corresponding model fields
 * @property {string} questionType
 * @returns {(string[]|Error} mergable subset of {AnswerModel} or Error
 */
const mapTypeToField = function(questionType) {
    switch(questionType) {

        case 'TEXT':
        case 'HIDDEN':
        case 'TEXTAREA':
        case 'CHECKBOX':
        case 'SINGLECHOICE':
        case 'SINGLESELECT':
            return (text) => {
                return {
                    text,
                };
            };

        case 'MULTICHOICE':
        case 'MULTISELECT':
            return (text) => {
                if (isArray(text)) {
                    text = text.map(v => v.replace(',' , '\,')).join(','); // eslint-disable-line no-useless-escape
                }

                return {
                    text,
                };
            };

        case 'INT':
        case 'FIXEDPOINT':
            return (value) => ({
                value,
            });

        case 'LATLON':
            return (lat, lon) => ({
                lat,
                lon,
            });

        case 'DATETIME': //TODO fcgimain.c puts it into "text" clarify
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

        case 'EMAIL':
            // TODO: validation
            return (text) => {
                return {
                    text,
                };
            };

        case 'PASSWORD':
            // TODO: validation
            return (text) => {
                return {
                    text,
                };
            };

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
 * @returns {(string|Error)}
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
const mergeAnswerValue = function(model, id, answer, questionType, unit) {

    const sanitized = sanitize(answer);

    let key = '';
    let value;

    // assign id
    model.uid = id;
    model.unit = unit;

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

        default:
            // nothing
    }

    // handle errors
    if (!key) {
        return new Error('CSV serializer: Unknown field key for questionType ' + questionType);
    }

    if (value instanceof Error) {
        return value;
    }

    // build
    model[key] = value;

    return model;
};

/**
 * Parses a csv row from an answer
 * example format of a row: "question1:gfdsg:0:0:0:0:0:0:0"
 * TODO: this function is now obsolete
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
 * Parses a csv row from an mapped answer object
 * example format of a row: "question1:gfdsg:0:0:0:0:0:0:0"
 *
 * @param {string} id question id
 * @param {object} answer answer object where keys are matching a model properites and values is the answer value
 * @returns {(string|Error)}  csv row or Error to be displayed
 */
const serializeAnswer = function (id, answer, type, unit) {
    let model = getModel();
    let key;

    if(isArray(answer)) {
        return new Error('Missing answer');
    }

    const empties = ['', null, undefined, NaN, Infinity];
    if(empties.indexOf(answer) > -1) {
        return new Error('Missing answer');
    }

    for (key in answer) {
        if(answer.hasOwnProperty(key)) {
            model = mergeAnswerValue(model, id, answer[key], key, unit);
            if(model instanceof Error) {
                return model;
            }
        }
    }

    return Object.values(model).join(CSV_SEPARATOR);
};

/**
 * Parses a csv row from a SurveyJS answer instance
 * example format of a row: "question1:gfdsg:0:0:0:0:0:0:0"
 *
 * @param {QuestionModel} question
 * @param {object} answer answer object where keys are matching a model properites and values is the answer value
 * @returns {(string|Error)}  csv row or Error to be displayed
 */
const serializeQuestionAnswer = function (element, question, ...values) {

    const { id, type, unit} = question;

    if(element && typeof element.validity !== 'undefined') {
        if (!element.validity.valid) {
            return new Error (element.validationMessage);
        }
    }

    const fn = mapTypeToField(type);

    if (fn instanceof Error) {
       return fn;
    }

    const answer = fn(...values);

    if (answer instanceof Error) {
       return answer;
    }

    return serializeAnswer(id, answer, type, unit);
};

export { serializeAnswer, serializeAnswerValue, mapTypeToField, serializeQuestionAnswer, CSV_SEPARATOR };
