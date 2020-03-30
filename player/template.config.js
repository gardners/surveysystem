const assert = require('assert');

/**
 * Copy this file to ./configs/<MY NAME>.js and edit your survey settings
 */
module.exports = {

    //// survey server
    Api: {
        SURVEYID: 'foo',
        PROTOCOL: 'http',
        DOMAIN: 'localhost',
        PORT: 3098,

        // authentication: http basic or digest
        AUTH: null, // format: 'myuser:mypassword'
        USE_DIGEST: false,
    },

    //// run assertions on answer responses: { answer => callback(response) }
    assertions: {
        q1a: (response) => {
            const { next_questions } = response;
            const ids = next_questions.map(q => q.id);
            assert.ok(ids.indexOf('q1b' ) > -1);
        },
    },

    //// provide custom answers fo given question ids, overwrites the generic question for that question
    answers: {
        q1a: 'my custom name',
    },

};
