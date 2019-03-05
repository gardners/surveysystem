/**
 * Copy this file to config.js and edit your survey settings
 */
module.exports = {

    //// survey server
    Api: {
        SURVEYID: 'foo',
        PROTOCOL: 'http',
        DOMAIN: 'localhost',
        PORT: 3098,
    },

    //// provide custom answers fo given question ids, overwrites the generic question for that question
    answers: {
        q1a: 'my custom name',
    },

};
