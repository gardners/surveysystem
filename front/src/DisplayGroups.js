/**
 * Extract the group id from a question id
 * a group flag inside an id string
 */
const getGroupId = function(id) {
    // TODO optimize for performance
     const parts = id.split('__');
     if (parts.length > 1) {
         parts.pop();
    }
     return parts.join('__').trim();
};

/**
 * Create an array of display groups consisting of questions.
 * The membership of a question to a group is evaluated by a delimiter pattern within the question id.
 * @param {object[]} Array of question objects
 *
 * @returns {array[]} two dimensional arraus of groups and containing question objects
 */

const makeDisplayGroups = function(questions) {
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

export { makeDisplayGroups, getGroupId };
