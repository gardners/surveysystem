import axios from 'axios';
import { resolve } from './resolve.js';
import {Configuration} from "../conf/config";

import Log from '../Log';
//tuto : https://javascript.info/async-await
// and : https://stackoverflow.com/questions/49500379/typical-file-structure-in-reactjs-application-grouping-api-calls-in-api-js

const api = {

    createNewSession : async function(surveyID){
        let apiConfig = Configuration.apiCalls.createNewSession
        let url = Configuration.serverBaseUrl().concat(apiConfig.path)
        apiConfig.params.surveyid = surveyID

        Log.log("Getting the Survey with ID="+ surveyID+"...");
        Log.log("requesting with " + apiConfig.method + ' request : '+ url + ', with these parameters :');

        for (const [param, value] of Object.entries(apiConfig.params)) {
            Log.log(`${param}=${value}`)
        }

        let resolved = await resolve(
            axios({
                method: apiConfig.method,
                url: url,
                params : apiConfig.params
            })
        )
        return resolved
    },

    nextQuestion : async function(sessionID){
        let apiConfig = Configuration.apiCalls.nextQuestion
        let url = Configuration.serverBaseUrl().concat(apiConfig.path)
        apiConfig.params.sessionid = sessionID

        Log.log("requesting with " + apiConfig.method + ' request : '+ url + ', with these parameters :')

        for (const [param, value] of Object.entries(apiConfig.params)) {
            Log.log(`${param}=${value}`)
        }


        let resolved = await resolve(
            axios({
                method: apiConfig.method,
                url: url,
                params : apiConfig.params
            })
        )
        return resolved
    },

    updateAnswer : async function(sessionID, answer){
        let apiConfig = Configuration.apiCalls.updateAnswer
        let url = Configuration.serverBaseUrl().concat(apiConfig.path)
        apiConfig.params.sessionid = sessionID
        apiConfig.params.answer = answer

        Log.log("requesting with " + apiConfig.method + ' request : '+ url + ', with these parameters :')

        for (const [param, value] of Object.entries(apiConfig.params)) {
            Log.log(`${param}=${value}`)
        }


        let resolved = await resolve(
            axios({
                method: apiConfig.method,
                url: url,
                params : apiConfig.params
            })
        )
        return resolved
    },

    deleteAnswer : async function(sessionID, questionID){
        let apiConfig = Configuration.apiCalls.deleteAnswer
        let url = Configuration.serverBaseUrl().concat(apiConfig.path)
        apiConfig.params.sessionid = sessionID
        apiConfig.params.questionid = questionID

        Log.log("requesting with " + apiConfig.method + ' request : '+ url + ', with these parameters :')

        for (const [param, value] of Object.entries(apiConfig.params)) {
            Log.log(`${param}=${value}`)
        }


        let resolved = await resolve(
            axios({
                method: apiConfig.method,
                url: url,
                params : apiConfig.params
            })
        )
        return resolved
    }
}

export default api;
