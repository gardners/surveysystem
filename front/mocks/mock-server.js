const http = require('http');
const path = require('path');
const Url = require('url');
const querystring = require('querystring');
const fs = require('fs');

// survey
const Survey = require('./survey-model');
const SURVEY_SESSION_ID = Date.now().toString();
const SURVEY_DIR = './survey';
const SURVEY_COUNT = Survey.countAnswers(SURVEY_DIR);

let count = 0;

const server = (() => {
    return http.createServer(function(request, response) {

        const url = Url.parse(request.url);
        const extname = path.extname(url.pathname).toLowerCase();

        const name = path.basename(url.pathname, extname);
        const dirname = path.dirname(url.pathname);
        const { query } = url.query;

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
        let answer;

        request.on('end', () => {

            // default
            switch (name) {

                // api
                // todo check session id

                case 'newsession':
                    response.statusCode = 200;
                    response.statusText = 'OK';
                    response.end(SURVEY_SESSION_ID, 'utf-8');
                    count = 0;
                    break;

                case 'nextquestion':
                    answer = Survey.getAnswer(count, SURVEY_DIR),

                    response.statusCode = answer.statusCode;
                    response.statusText = answer.statusText;
                    response.end(answer.payload, 'utf-8');
                    count += (count < SURVEY_COUNT - 1) ? 1 : 0;
                    break;

                case 'delanswer':
                    if(count) {
                        count -= 1;
                    }
                    answer = Survey.getAnswer(count, SURVEY_DIR),

                    response.statusCode = answer.statusCode;
                    response.statusText = answer.statusText;
                    response.end(answer.payload, 'utf-8');
                    break;

                case 'updateanswer':
                    answer = Survey.getAnswer(count, SURVEY_DIR),

                    response.statusCode = answer.statusCode;
                    response.statusText = answer.statusText;
                    response.end(answer.payload, 'utf-8');
                    count += (count < SURVEY_COUNT - 1) ? 1 : 0;
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

        //console.log('count', count);

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
