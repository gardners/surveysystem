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
        }
    }




};