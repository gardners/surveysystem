const request = require('request');

const Fetch = {
    SURVEYID: null,
    PROTOCOL: null,
    DOMAIN: null,
    PORT: null,

    raw: function(path, params = {}, headers = {}, options = {}) {

        const qs = Object.assign({
            surveyid: Fetch.SURVEYID,
        }, params);

        //let query = Object.keys(qs).reduce((acc, key) => {
        //    const part = encodeURIComponent(key) + '=' + encodeURIComponent(q[key]);
        //    const r = acc + ((acc) ? '&' + part : part);
        //    return r;
        //}, '');

        //if (query) {
        //    query = `?${query}`;
        //}

        const uri = Fetch.PROTOCOL + '://' + ((Fetch.PORT) ? `${Fetch.DOMAIN}:${Fetch.PORT}` : Fetch.DOMAIN) + ((path) ? `/${path}` : '');

        const opts = Object.assign({
            method: 'GET',
            uri,
            qs,
            headers,
        }, options);

        return new Promise(((resolve, reject) => {
            request(opts, (err, response, body) => {
                if (err) {
                    reject(err);
                    return;
                }
                console.log('statusCode:', response && response.statusCode); // Print the response status code if a response was received
                console.log('body:', body); // Print the HTML for the Google homepage.
                resolve(body);
            });
        }));
    },

    json: function(path, params = {}, headers = {}) {
        return Fetch.raw(path, params, headers, { json: true });
    },
};

module.exports = Fetch;
