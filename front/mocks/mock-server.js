const http = require('http');
const path = require('path');
const Url = require('url');
const querystring = require('querystring');
const fs = require('fs');

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

        request.on('end', () => {
            // xapi mock
            if (isXapi) {
                // legacy mode
                // @see https://github.com/adlnet/xAPI-Spec/blob/master/xAPI-Communication.md#alt-request-syntax
                var isLegacy = /application\/x-www-form-urlencoded/i.test(request.headers['content-type']);
                if (isLegacy) {
                    body = querystring.parse(body);
                    if (typeof body.content !== 'undefined') {
                        try {
                            body.content = JSON.parse(body.content);
                        } catch (e) {
                            response.statusCode = 500;
                            response.end('error parsing content: ' + e.messsage, 'utf-8');
                        }
                    }
                    body = JSON.stringify(body);
                }
                response.end(body, 'utf-8');
                return;
            }

            // default
            switch (name) {
                case '404':
                    response.statusCode = 404;
                    response.end(body, 'utf-8');
                    break;
                case '200':
                    response.statusCode = 200;
                    response.end(body, 'utf-8');
                    break;
                case '204':
                    response.statusCode = 204;
                    response.end('', 'utf-8');
                    break;
                case '400':
                    response.statusCode = 400;
                    response.end(body, 'utf-8');
                    break;
                default:
                    response.statusCode = 500;
                    response.end(body, 'utf-8');
            }

        });

    });

})();

const listen = (port) => {
    const url = `http://localhost:${port}`;
    server.listen(port);
    console.log(`Server running at ${url}`);
    return url;
};

const close = () => {
    server.close();
};

module.exports = {
    listen: listen,
    close: close
};
