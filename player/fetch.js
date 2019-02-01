const http = require('http');

const Fetch = {
    SURVEYID: null,
    HOST: null,
    PORT: null,

    raw: function(path, params = {}, headers = {}) {

        const q = Object.assign({
            surveyid: Fetch.SURVEYID,
        }, params);

        let query = Object.keys(q).reduce((acc, key) => {
            const part = encodeURIComponent(key) + '=' + encodeURIComponent(q[key]);
            const r = acc + ((acc) ? '&' + part : part);
            return r;
        }, '');

        if (query) {
            query = `?${query}`;
        }

        const options = {
            host: Fetch.HOST,
            port: Fetch.PORT,
            path: `${path}${query}`,
            headers,
        };

        return new Promise(((resolve, reject) => {

            http.get(options, (res) => {
                // Continuously update stream with data
                let body = '';

                res.on('data', (chunk) => {
                    body += chunk;
                });

                res.on('end', () => {
                    const { statusCode, statusMessage } = res;

                    if (statusCode >= 299) {
                        reject(new Error(`[${statusCode}] ${statusMessage}: ${body}`));
                        return;
                    }
                    resolve(body);
                });

                res.on('error', (error) => {
                    reject(error);
                });

            });

        }));
    },

    json: function(path, params = {}, headers = {}) {
        const jsonHeaders = Object.assign(headers, {
            'Content-Type': 'application/json',
        });

        return Fetch.raw(path, params, jsonHeaders)
            .then(body => JSON.parse(body));
    },
};

module.exports = Fetch;
