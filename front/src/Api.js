/**
 * @module APIhttp request API to surveysystem/backend
 */

import { buildUrl } from './Utils';
import JwtAuth from './JwtAuth';


const {
    REACT_APP_SURVEYAPI_ENDPOINT,
    REACT_APP_SURVEYAPI_HEADERS,
} = process.env;

const BaseHeaders = {};
try {
    const cHeaders = JSON.parse(REACT_APP_SURVEYAPI_HEADERS || '{}');
    Object.assign(BaseHeaders, cHeaders);
} catch(e) {
    // nothing
}


class ApiError extends Error {
    constructor(message, response = {}) {
        super(message);
        this.status = response.status || null;
        this.statusText = response.statusText || null;
        this.url = response.url || null;
    }
}

/**
 * Constructs an url , based on REACT_APP_SURVEYAPI_ENDPOINT
 * @returns {string}
 */
const url = function(path, params) {
    return buildUrl(REACT_APP_SURVEYAPI_ENDPOINT, path, params);
}

/**
 * Constructs default request headers
 * @returns {object}
 */
const requestHeaders = function(headers) {
    headers = headers || {};

    const requestHeaders = {
        'If-Match': localStorage.getItem('ss_consinstency_hash'),
    };
    const authHeaders = JwtAuth.request_headers();

    // add consistency header
    Object.assign(headers, BaseHeaders, requestHeaders, authHeaders);

    return headers;
};

/**
 * Stores response ETag in localStorage (#268)
 * @returns {object}
 */
const cacheResponse = function(response) {
    if (!response || !response.ok) {
        return;
    }
    const etag = response.headers.get('ETag');
    if (etag) {
        localStorage.setItem('ss_consinstency_hash', etag);
    }
};

/**
 * Handles and formats response errors in a promise context
 * @param {Response} fetch response object @see https://developer.mozilla.org/en-US/docs/Web/API/Response
 * @returns {Promise} Promise object throwing an exception (being catched in flow)
 */
const responseError = function (response) {
    return response.text()
        .then((text) => {
            throw new ApiError(text.replace(/(<([^>]+)>)/ig, ''), response);
        });
};

/**
 * set up AbortController
 * #441
 */
let controller = null;
let signal = null;
if (typeof window.AbortController !== 'undefined') {
    controller = new AbortController();
    signal = controller.signal;
}

/**
 * Backend requests
 * @type {object}
 */
const Api = {

    /**
     * cancel running xhr requests using fetch api's AbortController
     * #441
     *
     * @returns {void}
     */
    cancelRequests: function() {
        if (controller) {
            controller.abort();
        }
    },

    /**
     * Request a new session
     * @returns {Promise}
     */
    createNewSession: function(survey_id) {
        const uri = url('/session', {
            'surveyid': survey_id
        });

        const opts =  {
            signal,
            headers: requestHeaders(),
        };

        return fetch(uri, opts)
            .then((response) => {
                if (!response.ok) {
                    return responseError(response);
                }
                cacheResponse(response);
                return response.text();
            })
            .then((txt) => {
                // some systems return the session id with 'application/json' and a json string '"aaaaaaaa-0000-0000-0000-aaaaaaaaaaaa"'
                return txt.replace(/^"+|"+$/g, '');
            })
            .catch((error) => {
                if(error && error.name === 'AbortError') {
                    return;
                }
                throw error;
            });
    },

    /**
     * Fetches next question(s)
     * @param {string} session_id
     *
     * @returns {Promise} deserialized json including next questions
     */
    nextQuestion: function(session_id) {
        const uri = url('/questions', {
            'sessionid': session_id,
        });

        const opts =  {
            signal,
            headers: requestHeaders(),
        };

        return fetch(uri, opts)
            .then((response) => {
                if (!response.ok) {
                    return responseError(response);
                }
                cacheResponse(response);
                return response.json();
            })
            .catch((error) => {
                if(error && error.name === 'AbortError') {
                    return;
                }
                throw error;
            });
    },

    /**
     * Add answers for previous question id sand receive next question(s)
     * @param {string} session_id
     * @param {string} question_id
     * @returns {Promise} deserialized json including next questions
     */
    addAnswers: function(session_id, answers) {
        const uri = url('/answers', {
            'sessionid': session_id,
        });

        const opts =  {
            method: 'POST',
            headers: requestHeaders({
                'Content-Type': 'text/csv',
            }),
            signal,
            body: answers.join('\n'),
        };

        return fetch(uri, opts)
            .then((response) => {
                if (!response.ok) {
                    return responseError(response);
                }
                cacheResponse(response);
                return response.json();
            })
            .catch((error) => {
                if(error && error.name === 'AbortError') {
                    return;
                }
                throw error;
            });
    },

    /**
     * Delete previous answers and receive next question(s) (#268)
     * Requies the last consistency checksum supplied by a next_questions response stored in localStorage
     * @param {string} session_id
     * @returns {Promise} deserialized json including next questions
     */
    deletePreviousAnswer: function(session_id) {
        const uri = url('/answers', {
            'sessionid': session_id,
        });

        const opts =  {
            method: 'DELETE',
            signal,
            headers: requestHeaders(),
        };

        return fetch(uri, opts)
            .then((response) => {
                if (!response.ok) {
                    return responseError(response);
                }
                cacheResponse(response);
                return response.json();
            })
            .catch((error) => {
                if(error && error.name === 'AbortError') {
                    return;
                }
                throw error;
            });
    },

    // TODO: this is a temporary implementation

    /**
     * Mark survey as finished and receive evaluation
     * @param {string} session_id
     * @returns {Promise} deserialized json with evaluationdata
     */
    finishSurvey: function(session_id) {
        const uri = url('/analysis', {
            'sessionid': session_id,
        });

        const opts =  {
            signal,
            headers: requestHeaders(),
        };

        return fetch(uri, opts)
            .then((response) => {
                if (!response.ok) {
                    return responseError(response);
                }
                cacheResponse(response);
                return response.json();
            })
            .catch((error) => {
                if(error && error.name === 'AbortError') {
                    return;
                }
                throw error;
            });
    },

};

Api.getAnalysis = Api.finishSurvey; //TODO tmp

export { Api as default, ApiError, responseError };
