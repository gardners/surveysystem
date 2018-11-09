export const Configuration = {
    //back end config
    serverUrl: 'http://localhost',
    serverPort :'80',
    serverBaseUrl : function(){
        let baseURL =this.serverUrl.concat(':',this.serverPort)
        return(baseURL)
    },

    //survey theme
    surveyTheme: 'bootstrap',

    //custom properties
    customProperties : [
        {
            'basetype' : 'questionbase',
            'name' : 'id',
            'type' : 'string',
            'description' : 'identifies a question between the back and front'
        },
        {
            'basetype' : 'questionbase',
            'name' : 'title_text',
            'type' : 'string',
            'description' : ''
        }
    ],


    apiCalls : {
        createNewSession : {
            method : 'get',
            path : '/surveyapi/newsession',
            params :
                {
                    surveyid : null
                }
        },
        nextQuestion : {
            method : 'get',
            path : '/surveyapi/nextquestion',
            params :
                {
                    sessionid : null
                }
        },
        updateAnswer : {
            method : 'get',
            path : '/surveyapi/updateanswer',
            params :
                {
                    sessionid : null,
                    answer : null
                }
        },
        deleteAnswer : {
            method : 'get',
            path : '/surveyapi/delanswer',
            params :
                {
                    sessionid : null,
                    questionid : null
                }
        }
    }




};