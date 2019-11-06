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

    //// provide custom answers fo given question ids, overwrites the generic question for that question
    answers: {
        q1a: 'my custom name',
    },

};
