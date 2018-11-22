const http = require('http');
const path = require('path');
const Url = require('url');
const querystring = require('querystring');
const fs = require('fs');

const SURVEY_SESSION_ID = Date.now().toString();
let count = 0;

const server = (() => {
    return http.createServer(function(request, response) {

        const url = Url.parse(request.url);
        const extname = path.extname(url.pathname).toLowerCase();

        const name = path.basename(url.pathname, extname);
        const dirname = path.dirname(url.pathname);

        const contentType = 'application/json';

        let body = '';

        response.setHeader('content-type', contentType);

        // include some request data into response
        response.setHeader('x-req-content-type', request.headers['content-type'] || '');
        response.setHeader('x-req-search', url.search);
        response.setHeader('x-req-method', request.method);
        response.setHeader('x-req-content-length', request.headers['content-length'] || '');

        // CORS
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
        request.on('end', () => {

            // default
            switch (name) {

                // api
                // todo check session id

                case 'newsession':
                    response.statusCode = 200;
                    response.end(SURVEY_SESSION_ID, 'utf-8');
                    break;

                case 'nextquestion':
                    response.statusCode = 200;
                    response.end(fs.readFileSync(path.resolve() +'/questionTypes.json'), 'utf-8');
                    count += 1;
                    break;

                case 'delanswer':
                    response.statusCode = 200;
                    response.end(fs.readFileSync(path.resolve() +'/questionTypes.json'), 'utf-8');
                    if(count) {
                        count -= 1;
                    }
                    break;

                case 'updateanswer':
                    response.statusCode = 200;
                    response.end(fs.readFileSync(path.resolve() +'/questionTypes.json'), 'utf-8');
                    count += 1;
                    break;

                // status tests

                case '404':
                    response.statusCode = 404;
                    response.end(payload, 'utf-8');
                    break;

                case '200':
                    response.statusCode = 200;
                    response.end(payload, 'utf-8');
                    break;

                case '204':
                    response.statusCode = 204;
                    response.end('', 'utf-8');
                    break;

                case '400':
                    response.statusCode = 400;
                    response.end(payload, 'utf-8');
                    break;

                default:
                    response.statusCode = 500;
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
