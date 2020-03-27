const request = require('request');

const stringify = function(val) {
    let ret;
    try {
        ret = JSON.stringify(val);
    } catch (e) {
        ret = (typeof val.toString === 'function') ? val.toString() : val;
    }
    return ret;
};

const Fetch = {
    SURVEYID: null,
    PROTOCOL: null,
    DOMAIN: null,
    PORT: null,
    AUTH: null,
    USE_DIGEST: false,

    raw: function(path, params = {}, headers = {}, options = {}) {

        const qs = Object.assign({
            surveyid: Fetch.SURVEYID,
        }, params);

        const uri = Fetch.PROTOCOL + '://' + ((Fetch.PORT) ? `${Fetch.DOMAIN}:${Fetch.PORT}` : Fetch.DOMAIN) + path;

        if (Fetch.AUTH) {
            const auth = Fetch.AUTH.split(':');
            options.auth = {
                user: auth[0],
                pass: auth[1],
                sendImmediately: !Fetch.USE_DIGEST, // digest auth
            };
        }

        const opts = Object.assign({
            method: 'GET',
            uri,
            qs,
            headers,
        }, options);

        // console.log(opts);
        return new Promise(((resolve, reject) => {
            request(opts, (err, res, body) => {
                if (err) {
                    reject(err);
                    return;
                }

                if (res.statusCode >= 299) {
                    reject(new Error(`[${res.statusCode}] ${res.statusMessage}: ${stringify(body)}`));
                    return;
                }

                resolve(body);
            });
        }));
    },

    json: function(path, params = {}, headers = {}) {
        return Fetch.raw(path, params, headers, { json: true });
    },
};

module.exports = Fetch;
