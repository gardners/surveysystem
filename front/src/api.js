/**
 * @module APIhttp request API to surveysystem/backend
 */

import { Configuration } from './conf/config';
import Log from './Log';

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
        const apiConfig = Configuration.apiCalls.createNewSession;
        const url = Configuration.serverBaseUrl().concat(apiConfig.path);

        Log.log('Getting the Survey with ID=' + surveyID + '...');
        Log.log('requesting with ' + apiConfig.method + ' request : ' + url + ', with these parameters :');

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
        const apiConfig = Configuration.apiCalls.nextQuestion;
        const url = Configuration.serverBaseUrl().concat(apiConfig.path);
        apiConfig.params.sessionid = sessionID;

        Log.log('requesting with ' + apiConfig.method + ' request : ' + url + ', with these parameters :');

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
        const apiConfig = Configuration.apiCalls.updateAnswer;
        const url = Configuration.serverBaseUrl().concat(apiConfig.path);

        Log.log('requesting with ' + apiConfig.method + ' request : ' + url + ', with these parameters :');

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
        const apiConfig = Configuration.apiCalls.deleteAnswer;
        const url = Configuration.serverBaseUrl().concat(apiConfig.path);

        Log.log('requesting with ' + apiConfig.method + ' request : ' + url + ', with these parameters :');

        return fetch(`${url}?sessionid=${sessionID}&questionid=${questionID}`)
            .then((response) => {
                if (!response.ok) {
                    return responseError(response);
                }
                return response.json();
            });
    }
};

export default Api;
