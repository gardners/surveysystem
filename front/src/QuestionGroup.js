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
 * Create an array of display groups consisting of questions.
 * The membership of a question to a group is evaluated by a delimiter pattern within the question id.
 * @param {object[]} Array of question objects
 *
 * @returns {array[]} two dimensional arraus of groups and containing question objects
 */
const createQuestionGroups = function(questions) {
    const groups = [];

    let lastid = '';
    questions.forEach((q) => {
        const id = getGroupId(q.id);

        // first
        if(id && id !== lastid) {
            groups.push([q]);
            lastid = id;
            return;
        }

        //match - middle
        if(id && id === lastid) {
            groups[groups.length - 1].push(q);
            lastid = id;
            return;
        }

        // anything else
        groups.push([q]);

        // cache id
        lastid = id;
    });

    return groups;
};

/**
 * Define the matrix status of a question within a question group
 *  - Within questionGroups there can be matrix questions.
 *  - A matrix of questions question is a sequence of group members with the same question type
 *
 * @param {number} index index of question in questionGroup
 * @param {array} questionGroup array of question objects
 * A matrix member has at least one sibling
 * - First member: no left sibling [false, true]
 * - last member; no right sibling [true, false]
 * - inner member: left and right siblings [true, true]
 * - "lost" member [false ,false] discards cases where a group id was submitted but the member is only a single item
 *
 * @returns {boolean[]} [ leftSibling, rightSibling ] array with exact two booleans, representing the existance of left sibling and right sibling,
 */
const matrixState = function(index, questionGroup) {
    const question = questionGroup[index];
    const { type } = question;

    const left = typeof questionGroup[index - 1] !== 'undefined' && questionGroup[index + 1].type === type;
    const right = typeof questionGroup[index + 1] !== 'undefined' && questionGroup[index + 1].type === type;

    return [
        left,
        right,
    ];
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

export { createQuestionGroups, getGroupId, matrixState, propTypes };
