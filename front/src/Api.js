/**
 * @module APIhttp request API to surveysystem/backend
 */

import { serializeParams } from './Utils';
import { request_headers } from './OAuth2';

const BaseUri = process.env.REACT_APP_SURVEYAPI_ENDPOINT.replace(/\/$/, "");

const url = function(path, params) {
  path = path || '';
  params = params || null;

  let q = '';
  if (params) {
    q = '?' + serializeParams(params);
  }

  return (!path) ? `${BaseUri}${q}` : `${BaseUri}${path}${q}`;
};

class ApiError extends Error {
    constructor(message, response = {}) {
        super(message);
        this.status = response.status || null;
        this.statusText = response.statusText || null;
        this.url = response.url || null;
    }
}

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
 * Backend requests
 * @type {object}
 */
const Api = {

    /**
     * Request a new session
     * @returns {Promise}
     */
    createNewSession: function(surveyid) {
        const uri = url('/newsession', {
            surveyid
        });

        const opts =  {
            headers: request_headers(),
        };

        return fetch(uri, opts)
            .then((response) => {
                if (!response.ok) {
                    return responseError(response);
                }
                return response.text();
            });
    },

    /**
     * Fetches next question(s)
     * @param {string} surveyID
     *
     * @returns {Promise} deserialized json including next questions
     */
    nextQuestion: function(sessionid) {
        const uri = url('/nextquestion', {
            sessionid,
        });

        const opts =  {
            headers: request_headers(),
        };

        return fetch(uri, opts)
            .then((response) => {
                if (!response.ok) {
                    return responseError(response);
                }
                return response.json();
            });
    },

    /**
     * Send answer to current question and receive next question(s)
     * @param {string} surveyID
     * @param {string} serialized answer
     * @returns {Promise} deserialized json including next questions
     */
    updateAnswer: function(sessionid, answer) {
        const uri = url('/updateanswer', {
            sessionid,
            answer,
        });

        const opts =  {
            headers: request_headers(),
        };

        return fetch(uri, opts)
            .then((response) => {
                if (!response.ok) {
                    return responseError(response);
                }
                return response.json();
            });
    },

    /**
     * Delete answer with acurrent id and receive next question(s)
     * @param {string} surveyID
     * @returns {Promise} deserialized json including next questions
     */
    deleteAnswer: function(sessionid, questionid) {
        const uri = url('/delanswer', {
            sessionid,
            questionid,
        });

        const opts =  {
            headers: request_headers(),
        };

        return fetch(uri, opts)
            .then((response) => {
                if (!response.ok) {
                    return responseError(response);
                }
                return response.json();
            });
    },

    /**
     * Delete all previous answers until the specified question id
     * @param {string} surveyID
     * @returns {Promise} deserialized json including next questions
     */
    deleteAnswerAndFollowing: function(sessionid, questionid) {
        const uri = url('/delanswer', {
            sessionid,
            questionid,
        });

        const opts =  {
            headers: request_headers(),
        };

        return fetch(uri, opts)
            .then((response) => {
                if (!response.ok) {
                    return responseError(response);
                }
                return response.json();
            });
    },

    // TODO: this is a temporary implementation

    /**
     * Mark survey as finished and receive evaluation
     * @param {string} surveyID
     * @returns {Promise} deserialized json with evaluationdata
     */
    finishSurvey: function(sessionid) {
        const uri = url('/analyse', {
            sessionid,
        });

        const opts =  {
            headers: request_headers(),
        };

        return fetch(uri, opts)
        .then((response) => {
            if (!response.ok) {
                return responseError(response);
            }
            return response.json();
        });
    },

    /**
     * Send multiple answers to current questions and receive next question(s)
     * @param {string} surveyID
     * @param {[string]} serialized answer
     * @returns {Promise} deserialized json including next questions
     */
    updateAnswers: function(sessionID, answers) {
        return Promise.all(
            answers.map(fragment => Api.updateAnswer(sessionID, fragment))
        )
    },

    /**
     * Send multiple answers to current questions and receive next question(s)
     * @param {string} surveyID
     * @param {[string]} serialized answer
     * @returns {Promise} deserialized json including next questions
     */
    updateAnswers_SEQUENTIAL: function(sessionID, answers, responses = []) {
        const next = responses.length;
        return Api.updateAnswer(sessionID, answers[next])
        .then((response) => {
            responses.push(response);
            if (responses.length < answers.length) {
                return Api.updateAnswers_SEQUENTIAL(sessionID, answers, responses);
            }
            return responses;
        });
    },

    /**
     * Send multiple answers to current questions and receive next question(s)
     * @param {string} surveyID
     * @param {[string]} serialized answer
     * @returns {Promise} deserialized json including next questions
     */
    deleteAnswers: function(sessionID, answers) {
        return Promise.all(
            answers.map(fragment => Api.deleteAnswer(sessionID, fragment))
        )
    },

    /**
     * Delete mutiple answers to current questions and new receive next question(s)
     * @param {string} surveyID
     * @param {[string]} serialized answer
     * @returns {Promise} deserialized json including next questions
     */
    deleteAnswers_SEQUENTIAL: function(sessionID, questionIDs, responses = []) {
        const next = responses.length;
        return Api.deleteAnswer(sessionID, questionIDs[next])
        .then((response) => {
            responses.push(response);
            if(responses.length < questionIDs.length) {
                return Api.deleteAnswers_SEQUENTIAL(sessionID, questionIDs, responses);
            }
            return responses;
        });
    },

};

Api.getAnalysis = Api.finishSurvey; //TODO tmp

export { Api as default, ApiError, BaseUri,responseError };
