const http = require('http');
const path = require('path');
const Url = require('url');
const querystring = require('querystring');
const fs = require('fs');

const Survey = require('./survey-model');

// survey

const SURVEY_DIR = './surveys';
let CURRENT = -1; // current survey index
let SURVEY_ID = '';

const server = (() => {
    return http.createServer(function(request, response) {

        const url = Url.parse(request.url, true);// with query parsing
        const extname = path.extname(url.pathname).toLowerCase();

        const name = path.basename(url.pathname, extname);
        const dirname = path.dirname(url.pathname);
        const { query } = url;

        const contentType = 'application/json';
        let body = '';

        // headers
        response.setHeader('content-type', contentType);

        // CORS headers
        response.setHeader('Access-Control-Allow-Origin', '*');
        response.setHeader('Access-Control-Allow-Methods', 'HEAD, GET, OPTIONS, POST, PUT, DELETE, PATCH');

        // errors
        request.on('error', (err) => {
            response.writeHead(500);
            response.end(`Sorry, error: [500] ${err.message} ..\n`);
            response.end();
        });

        request.on('data', (chunk) => {
            switch (request.method) {
                case 'POST':
                case 'PUT':
                case 'PATCH':
                    body += chunk.toString();
                    break;
            }
        });

        let payload = '';
        let res;

        request.on('end', () => {

            switch (dirname) {
                case '/surveyapi':
                    if(!SURVEY_ID) {
                        SURVEY_ID = Survey.parseSIDfromSession(query.sessionid);
                    }
                break;

                default:
                    // nothing
            }

            // console.log('current (before update)', CURRENT);

            switch (name.toLowerCase()) {

                // api
                // todo check session id

                case 'accesstest':
                    response.statusCode = 200;
                    response.statusText = 'OK';
                    response.end('', 'utf-8');
                    break;

                case 'newsession':
                    res = Survey.newsession(query.surveyid);
                    SURVEY_ID = query.surveyid;
                    CURRENT = res.current;

                    response.statusCode = res.statusCode;
                    response.statusText = res.statusText;
                    response.end(res.payload, 'utf-8');
                    break;

                case 'nextquestion':
                    res = Survey.nextquestion(SURVEY_ID, CURRENT);
                    CURRENT = res.current;

                    response.statusCode = res.statusCode;
                    response.statusText = res.statusText;
                    response.end(res.payload, 'utf-8');
                    break;

                case 'delanswer':
                    res = Survey.delanswer(SURVEY_ID, CURRENT);
                    CURRENT = res.current;

                    response.statusCode = res.statusCode;
                    response.statusText = res.statusText;
                    response.end(res.payload, 'utf-8');
                    break;

                case 'updateanswer':
                    res = Survey.updateanswer(SURVEY_ID, query.answer, CURRENT);
                    CURRENT = res.current;

                    response.statusCode = res.statusCode;
                    response.statusText = res.statusText;
                    response.end(res.payload, 'utf-8');
                    break;

                case 'analyse':
                    res = Survey.getevaluation(SURVEY_ID, CURRENT);
                    CURRENT = res.current;

                    response.statusCode = res.statusCode;
                    response.statusText = res.statusText;
                    response.end(res.payload, 'utf-8');
                    break;

                // status tests

                case '404':
                    response.statusCode = 404;
                    response.statusText = 'OK';
                    response.end(payload, 'utf-8');
                    break;

                case '200':
                    response.statusCode = 200;
                    response.statusText = 'OK';
                    response.end(payload, 'utf-8');
                    break;

                case '204':
                    response.statusCode = 204;
                    response.statusText = 'No Content';
                    response.end('', 'utf-8');
                    break;

                case '400':
                    response.statusCode = 400;
                    response.statusText = 'Bad Request';
                    response.end(payload, 'utf-8');
                    break;

                case '401':
                    response.statusCode = 401;
                    response.statusText = 'Unauthorized';
                    response.end(payload, 'utf-8');
                    break;

                default:
                    response.statusCode = 500;
                    response.statusText = 'Internal Server Error';
                    response.end(payload, 'utf-8');
            }

        });

    });

})();

const listen = (port) => {
    const url = `http://localhost:${port}`;
    server.listen(port);
    console.log(`Mock server running at ${url}`);
    return url;
};

const close = () => {
    server.close();
};

module.exports = {
    listen: listen,
    close: close
};
