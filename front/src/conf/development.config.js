export const Configuration = {
    //back end config
    serverUrl: 'http://localhost',
    //serverPort: '80',
    serverPort: '3099',
    serverBaseUrl: function() {
        const baseURL = this.serverUrl.concat(':', this.serverPort);
        return (baseURL);
    },

    // survey meta data
    surveyTheme: 'bootstrap',
    surveyProvider: 'Bar',
    surveys: ['foo', 'sleepcompanion'], // first survey is default survey

    //custom properties
    customProperties: [
        {
            basetype: 'questionbase',
            name: 'id',
            type: 'string',
            description: 'identifies a question between the back and front'
        },
        {
            basetype: 'questionbase',
            name: 'title_text',
            type: 'string',
            description: ''
        }
    ],


    apiCalls: {
        createNewSession: {
            method: 'get',
            path: '/surveyapi/newsession',
            params:
                {
                    surveyid: null
                }
        },
        nextQuestion: {
            method: 'get',
            path: '/surveyapi/nextquestion',
            params:
                {
                    sessionid: null
                }
        },
        updateAnswer: {
            method: 'get',
            path: '/surveyapi/updateanswer',
            params:
                {
                    sessionid: null,
                    answer: null
                }
        },
        deleteAnswer: {
            method: 'get',
            path: '/surveyapi/delanswer',
            params:
                {
                    sessionid: null,
                    questionid: null
                }
        }
    }


};
