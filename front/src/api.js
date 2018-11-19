import {Configuration} from './conf/config';
import Log from './Log';

const Api = {

    createNewSession: function(surveyID){
        let apiConfig = Configuration.apiCalls.createNewSession
        let url = Configuration.serverBaseUrl().concat(apiConfig.path)

        Log.log("Getting the Survey with ID="+ surveyID+"...");
        Log.log("requesting with " + apiConfig.method + ' request : '+ url + ', with these parameters :');

        return fetch(`${url}?surveyid=${surveyID}`)
            .then(response => response.text())
        ;
    },

    nextQuestion: function(sessionID){
        let apiConfig = Configuration.apiCalls.nextQuestion
        let url = Configuration.serverBaseUrl().concat(apiConfig.path)
        apiConfig.params.sessionid = sessionID

        Log.log("requesting with " + apiConfig.method + ' request : '+ url + ', with these parameters :')

        return fetch(`${url}?sessionid=${sessionID}`)
            .then(response => response.json())
        ;
    },

    updateAnswer: function(sessionID, answer){
        let apiConfig = Configuration.apiCalls.updateAnswer
        let url = Configuration.serverBaseUrl().concat(apiConfig.path)

        Log.log("requesting with " + apiConfig.method + ' request : '+ url + ', with these parameters :')

        return fetch(`${url}?sessionid=${sessionID}&answer=${answer}`)
            .then(response => response.json())
        ;
    },

    deleteAnswer: function(sessionID, questionID){
        let apiConfig = Configuration.apiCalls.deleteAnswer
        let url = Configuration.serverBaseUrl().concat(apiConfig.path)

        Log.log("requesting with " + apiConfig.method + ' request : '+ url + ', with these parameters :')

        return fetch(`${url}?sessionid=${sessionID}&questionid=${questionID}`)
            .then(response => response.text())
        ;
    }
}

export default Api;
