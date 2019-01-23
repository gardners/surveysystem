import PropTypes from 'prop-types';

/**
* Get Proptypes schema
* @returns {PropTypes}
*/
const Question = {
    propTypes:  function (withChoices = false) {
        return PropTypes.shape({
            id: PropTypes.string.isRequired,
            name: PropTypes.string.isRequired,
            title: PropTypes.string.isRequired,
            title_text: PropTypes.string.isRequired,
            type: PropTypes.string.isRequired,
            choices: (!withChoices) ? PropTypes.array : PropTypes.array.isRequired,
            unit: PropTypes.string.isRequired,
        });
    },
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
        const { id, type } = question;
        const gid = getGroupId(question.id);

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
 *  - "CHOICES"; same question ypes AND same choices: checkbox matrix
 * @param {object[]}  of questionsgroup
 * @returns {string} commons flag
 */
const findQuestionGroupCommons = function(group) {
    const { length } = group;

    if(!length) {
        return 'NONE';
    }

    let type = group[0].type;
    let choices = (typeof group[0].choices !== 'undefined') ? group[0].choices.toString() : '';

    for (let i = 1; i < length; i += 1) {
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

    return (!choices) ? 'TYPE' : 'CHOICES'
};

export { Question as default, getGroupId, mapQuestionGroups, findQuestionGroupCommons };
