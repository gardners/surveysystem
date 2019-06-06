import PropTypes from 'prop-types';

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
            choices: (!withChoices) ? PropTypes.array : PropTypes.array.isRequired,
            unit: PropTypes.string.isRequired,
        });
    },
};

/**
* Normalize a question object, received via REST api
* @see survey.h:: struct question
* @param {QuestionModel} question
*
* @returns {QuestionModel}
*/
const normalizeQuestion = function(question) {
    return Object.assign({
        id: "",
        name: "",
        title: "",
        description: "",
        type: "",
        default_value: "",
        unit: "",
    }, question);
};

/**
* Normalize an array of question objects, received via REST api
* @see survey.h:: struct question
* @param {QuestionModel[]} questions
*
* @returns {QuestionModel[]}
*/
const normalizeQuestions = function(questions) {
    return questions.map(q => normalizeQuestion(q));
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

    return (!choices) ? 'TYPE' : 'CHOICES'
};

export { Question as default, getGroupId, mapQuestionGroups, findQuestionGroupCommons, normalizeQuestion, normalizeQuestions };
