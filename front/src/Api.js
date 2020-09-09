/**
 * @module APIhttp request API to surveysystem/backend
 */

// config
const BaseUri = process.env.REACT_APP_SURVEYAPI_ENDPOINT;

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
    createNewSession: function(surveyID) {
        const url = BaseUri + '/newsession';

        return fetch(`${url}?surveyid=${surveyID}`)
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
    nextQuestion: function(sessionID) {
        const url = BaseUri + '/nextquestion';

        return fetch(`${url}?sessionid=${sessionID}`)
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
    updateAnswer: function(sessionID, answer) {
        const url = BaseUri + '/updateanswer';

        return fetch(`${url}?sessionid=${sessionID}&answer=${answer}`)
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
    deleteAnswer: function(sessionID, questionID) {
        const url = BaseUri + '/delanswer';

        return fetch(`${url}?sessionid=${sessionID}&questionid=${questionID}`)
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
    deleteAnswerAndFollowing: function(sessionID, questionID) {
        const url = BaseUri + '/delanswerandfollowing';

        return fetch(`${url}?sessionid=${sessionID}&questionid=${questionID}`)
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
    finishSurvey: function(sessionID) {
        const url = BaseUri + '/analyse';

        return fetch(`${url}?sessionid=${sessionID}`)
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
