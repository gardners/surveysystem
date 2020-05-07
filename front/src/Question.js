import PropTypes from 'prop-types';

import { sanitizeValue } from './Answer';
import { isArray } from './Utils';

////
/// Question model (nextquestions json)
////

/**
 * @typedef QuestionModel
 * @type {object}
 * @property {string} id - required
 * @property {string} name - required
 * @property {string} description - required
 * @property {string} type - required
 * @property {string} default_value - required
 * @property {string[]|number[]} choices - optional
 * @property {string} unit - required
 */

/**
 * Create an empty question object,
 * @see survey.h  struct question
 *
 * @returns {QuestionModel}
 */
const model = function() {
    return {
        id: "",
        name: "",
        title: "",
        description: "",
        type: "",
        default_value: "",
        min_value: "",
        max_value: "",
        choices: [],
        unit: "",
    };
};

/**
 * Normalize a question object, received via REST api
 * @see survey.h struct question
 * @param {object} raw question object
 *
 * @returns {QuestionModel}
 */
const normalize = function(raw) {
    const q = Object.assign(model(), raw);
    return q;
};

////
/// Question serialization model (experimental)
////

/**
 * @typedef QuestionSerializationModel
 * @type {object}
 * @property {string} id - required
 * @property {string} title - required
 * @property {string} description - required
 * @property {string} type - required
 * @property {number} flags - required
 * @property {string} default_value - required
 * @property {number} min_value - required
 * @property {number} max_value - required
 * @property {number} decimal_places - required
 * @property {number} num_choices - required
 * @property {string[]|number[]} choices - optional
 * @property {string} unit - required
 */

/**
 * Create an empty  question serialization object,
 * @see survey.h  struct question
 *
 * @returns {QuestionModel}
 */
const serializationModel = function() {
    return {
        uid: '',
        question_text: '',
        question_html: '',
        type: '',
        flags: 0,
        default_value: '',
        min_value: 0,
        max_value: 0,
        decimal_places: 2,
        num_choices: 0,
        choices: [],
        unit: '',
    };
};

/**
 * Serialize a question object
 * @param {object} question raw question object
 */
const serialize = function(question) {
    const model = serializationModel();

    const { choices } = question;

    model.uid = sanitizeValue(question.id);
    model.question_text = sanitizeValue(question.title);
    model.question_html = sanitizeValue(question.description);
    model.type = sanitizeValue(question.type);
    // model.flags = 0; // use default  TODO not supplied by backend yet
    model.default_value = sanitizeValue(question.default_value);
    model.min_value = (typeof question.min_value === 'number') ? question.min_value : Number(question.min_value);
    model.max_value = (typeof question.max_value === 'number') ? question.max_value : Number(question.max_value);
    // model.decimal_places = 0; // use default  TODO not supplied by backend yet
    model.num_choices = (isArray(choices)) ? choices.length : choices.split(',').length;
    model.choices = (isArray(choices)) ? sanitizeValue(choices.join(',')) : sanitizeValue(choices);
    model.unit = sanitizeValue(question.unit);

    return Object.keys(model).map(key => sanitizeValue(model[key])).join(':');
};

////
// Question module
////

/**
* Get Proptypes schema
* @see survey.h:: struct question
* @returns {PropTypes}
*/
const Question = {
    propTypes:  function (withChoices = false) {
        return PropTypes.shape({
            id: PropTypes.string.isRequired,
            name: PropTypes.string.isRequired,
            title: PropTypes.string.isRequired,
            description: PropTypes.string.isRequired,
            type: PropTypes.string.isRequired,
            default_value: PropTypes.oneOfType([
                PropTypes.string,
                PropTypes.number
            ]).isRequired,
            min_value: PropTypes.string,
            max_value: PropTypes.string,
            choices: (!withChoices) ? PropTypes.array : PropTypes.array.isRequired,
            unit: PropTypes.string.isRequired,
        });
    },
    valuePropTypes: function() {
        return PropTypes.oneOfType([
            PropTypes.string,
            PropTypes.number,
            PropTypes.array,
            PropTypes.bool,
        ]);
    },
    model,
    normalize,
    serialize,
};

/**
* Normalize an array of question objects, received via REST api
* @see survey.h:: struct question
* @param {QuestionModel[]} questions
*
* @returns {QuestionModel[]}
*/
const normalizeQuestions = function(questions) {
    return questions.map(q => Question.normalize(q));
};

/**
 * Extract the group id from a question id
 * a group flag inside an id string
 * @param {string} id
 *
 * @returns {string} group id
 */
const getGroupId = function(id) {
     let  gid = '';
    // TODO optimize for performance
     const parts = id.trim().split('__');
     if (parts.length > 1) {
         gid = parts.pop().trim();
     }
     return gid;
};

/**
 * Create an array of questions or display groups (array) of questions with the same type.
 * The potential membership of a question to a group is evaluated by a delimiter pattern within the question id.
 * Another parameter for a group is that the current question type matches the previous.
 * @param {object[]} Array of question objects
 *
 * @returns {array} array where members are either single questions or an array of questions
 */
const mapQuestionGroups = function(questions) {
    if(!questions.length) {
        return [];
    }

    const mapped = [];
    let lastGid = '';

    questions.forEach(function(question, index) {
        const { id } = question;
        const gid = getGroupId(id);

        if (!gid) {
            mapped.push(question);
            lastGid = '';
            return;
        }

        // create new group, either if lastGid is empty or gid != lastgid
        if(gid !== lastGid) {
            mapped.push([]);
        }
        // append question to group
        mapped[mapped.length - 1].push(question);
        lastGid = gid;
    });

    return mapped;
};

/**
 * Finds common qualities within in a question group
 *  - "NONE": no common type or choices, inline display
 *  - "TYPE": same question types inline display, TODO
 *  - "CHOICES"; same question types AND same choices: checkbox matrix
 * @param {object[]}  of questionsgroup
 * @returns {string} commons flag
 */
const findQuestionGroupCommons = function(group) {
    const { length } = group;

    if(length < 2) {
        return 'NONE';
    }

    let start = (group[0].type === 'HIDDEN') ? 1 : 0;
    let type = group[start].type;
    let choices = (typeof group[start].choices !== 'undefined') ? group[start].choices.toString() : '';

    for (let i =  start; i < length; i += 1) {
        // different question types
        if (group[i].type !== type) {
            return 'NONE';
        }

        // if first question has choices compare member choices
        if (choices)  {
            // this should be theroetically redundant as this would be a different type
            // but it prevents breaking
            if (typeof group[i].choices === 'undefined') {
                return 'TYPE';
            }

            if (group[i].choices.toString() !== choices) {
                return 'TYPE';
            }
        }
    }

    if (type === 'DAYTIME') {
        return 'DAYTIME_SEQUENCE';
    }

    // #339 CHECKBOX commons, display only single checkbox per row
    if (type === 'CHECKBOX') {
        return 'CHECKBOX';
    }

    return (!choices) ? 'TYPE' : 'CHOICES'
};

export { Question as default, getGroupId, mapQuestionGroups, findQuestionGroupCommons, normalizeQuestions };
