/**
 * @module APIhttp request API to surveysystem/backend
 */

// config
const BaseUri = process.env.REACT_APP_API_ENDPOINT;

/**
 * handles and ormats response errors in a promise context
 * @param {Response} fetch response object @see https://developer.mozilla.org/en-US/docs/Web/API/Response
 * @returns {Promise} Promise object throwing an exception (being catched in flow)
 */
const responseError = function (response) {
    const { status, statusText } = response;
    return response.text()
        .then((text) => {
            throw new Error(`[${status}: ${statusText}], reason: ${text}`)
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
        const url = BaseUri + '/surveyapi/newsession';

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
        const url = BaseUri + '/surveyapi/nextquestion';

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
     * @returns {Promise} deserialized json including next questions
     */
    updateAnswer: function(sessionID, answer) {
        const url = BaseUri + '/surveyapi/updateanswer';

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
        const url = BaseUri + '/surveyapi/delanswer';

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
        const url = BaseUri + '/surveyapi/analyse';

        // TODO: remove if implemented on backend
        if(process.env.NODE_ENV !== 'development') {

            return Promise.resolve({
                TODO: 'TODO'
            });
        }

        return fetch(`${url}?sessionid=${sessionID}`)
            .then((response) => {
                if (!response.ok) {
                    return responseError(response);
                }
                return response.json();
            });
    },

};

Api.getAnalysis = Api.finishSurvey; //TODO tmp

export { Api as default, BaseUri };
