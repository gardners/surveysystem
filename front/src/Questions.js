import PropTypes from 'prop-types';
import Question from './Question';

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
const createQuestionGroups = function(questions) {
    if(!questions.length) {
        return [];
    }

    const map = {};
    // stores type, first type forces single array of questions into group
    let lastType = questions[0].type;

    questions.forEach(function(question) {
        const { id, type } = question;
        const gid = getGroupId(question.id);

        if (gid && type === lastType) {

            if(typeof map[gid] === 'undefined') {
                map[gid] = [];
            }

            map[gid].push(question);
            lastType = type;

            return;
        }

        map[id] = question;
        lastType = type;
    });

    return Object.keys(map).map(key => map[key]);
};

/**
 * Get Proptypes schema
 * @returns {PropTypes}
 */
const propTypes = function () {
    return PropTypes.arrayOf(
        // eslint-disable-next-line react/forbid-foreign-prop-types
        Question.propTypes(),
    );
};

export default { createQuestionGroups, getGroupId,  propTypes };
