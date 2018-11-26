/**
 * @module Maps backend properties to SurveyJS properites
 */

/**
 * Maps question types (keys) to SurveyJS Model types (values)
 * @type {object}
 * @see https://github.com/surveyjs/surveyjs/tree/master/src question_{type}.ts::getType()
 */
const FieldTypes = Object.freeze({
    // API : SurveyJS
    'geolocation' : 'geolocation',
    'value': 'number',
    'text': 'text',
    'radiogroup': 'radiogroup', //@see https://github.com/surveyjs/surveyjs/blob/master/src/question_radiogroup.ts
});

/**
 * Maps question type to SurveyJS Model type, returns "text" type if not found
 * @param {string} type question.type
 * @returns {string} surveyjs model type property or "text" as default
 */
const mapQuestionType = function(field) {
    return FieldTypes[field] || 'text';
};

export {
    FieldTypes,
    mapQuestionType,
}
