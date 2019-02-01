/**
 * Copy this file to config.js and edit your survey settings
 */
module.exports = {

    //// survey server
    Api: {
        surveyid: 'foo',
        host: 'localhost',
        port: 3098,
    },

    //// provide custom answers fo given question ids, overwrites the generic question for that question
    answers: {
        q1a: 'my custom name',
    },

};
