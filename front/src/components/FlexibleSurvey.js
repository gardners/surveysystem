import React from 'react';
import * as Survey from "survey-react";
import axios from 'axios';
import { Configuration } from '../conf/config';
import LoadingSpinner from './LoadingSpinner';
import geolocationQuestion from '../customQuestions/geolocationQuestion'


// Represents the Flexible Survey
class FlexibleSurvey extends React.Component {

    //The ID of the survey to load from the server
    //the ID is retrieved from the url, that looks like : /survey/:id
    surveyID = this.props.match.params.id
    //The ID of the session, as identified by the back end
    //the session id generated by the server, and used by the front and back during the whole survey process
    sessionID = null
    //contains an array listing all question objects, by this way : [questionId => {question}, etc...]
    questions = []
    //contains an array listing all added pages(and the question it contains) objects to the survey, by this way : [pageId => {page}, etc...]
    //a page must at least have 1 question, and as many as wanted
    //a new page is created each time the back send the next question(s) to answer
    pages = []
    //allows to know the number of pages currently in the survey. It is also used to set the pageId of a page
    //the stepId is incremented by 1 each time a new set of questions is added to the survey
    stepID = 0
    // contains the question ID that are asked currently to the user
    //used for logs
    currentQuestionsBeingAnswered = []
    //when there are no more questions to ask, set to true
    _surveyCompleted = false



    // Constructor, needed in every case in React
    // survey is the react component from surveyjs. It is the backbone of this front app
    // loading is used to know if an ajax request is being made. If so, a loading spinner is shown while loading == true
    constructor(props) {
        super(props);
        this.state = {
            survey : new Survey.Model(),
            loading : true
        };
    }




    //put here the configuration of the survey, such as the theme, event listeners, etc.
    configureSurvey(){
        Survey.StylesManager.applyTheme(Configuration.surveyTheme)
        Survey.CustomWidgetCollection.Instance.addCustomWidget(geolocationQuestion, "customtype")
        this.configureNextAndPreviousButtons()
        this.addCustomProperties()
    }

    //adds the listeners to next and previous buttons
    //it also configures the diplayed text on the completeText button
    configureNextAndPreviousButtons(){
        let tmpSurvey = this.state.survey
        tmpSurvey.completeText = "Next"

        //eventlistener when user presses prev
        tmpSurvey.onCurrentPageChanged.add(function(sender, options){
            if(!options.oldCurrentPage || !options.newCurrentPage) return; //showing first page
            if(options.oldCurrentPage.visibleIndex > options.newCurrentPage.visibleIndex){
                this.goBackToPreviousStep();
            }
        }.bind(this));

        //eventlistener when user presses next
        //it is not really the next button, but complete button, with a display text set at 'Next', since the current displayed page is always the last one.
        tmpSurvey.onCompleting.add(function(sender, options){
            if (this._surveyCompleted){
                options.allowComplete = true
            } else {
                options.allowComplete = false
                this.onNextButtonPressed(sender, options)
            }
        }.bind(this));

        this.setState({
            survey : tmpSurvey
        });
    }

    //you may want, while creating a new survey, add some custom properties in the question, such as their ID.
    // to declare these new properties, go to the config file. this function will automatically import them when the survey is built
    addCustomProperties(){

        for (let id in Configuration.customProperties){
            let customProperty = Configuration.customProperties[id]
            Survey.JsonObject.metaData.addProperty(customProperty.basetype, { name: customProperty.name, type: customProperty.type });
        }

    }

    //get a question saved in the component's properties by its id
    getQuestionById(id){
        const tmpQuestions = this.questions;
        if (!(id in tmpQuestions)){
            console.error("There is no questions defined by the id "+id);
            return null;
        }
        return tmpQuestions[id];
    }

    //get a page saved in the component's properties by its id
    getPageById(id){
        let tmpPages = this.pages
        if (!(id in tmpPages)){
            console.error("There is no page defined by the id "+id);
            return null;
        }
        return tmpPages[id];
    }

    //accepts a list of question IDs, and add the corresponding questions in a new page, at the end of the survey
    setNextQuestionOfSurvey(questionIds){
        let tmpSurvey = this.state.survey
        let tmpStepID = this.stepID
        let newPage = tmpSurvey.addNewPage(tmpStepID);
        for (let id in questionIds){
            let questionId = questionIds[id]
            console.log("adding the next question (id="+questionId+") at the end of survey...");
            const questionToAdd = this.getQuestionById(questionId);
            if (!questionToAdd){
                console.error("failed to get the question with id "+questionId+" !");
                return null;
            }
            newPage.addQuestion(questionToAdd);
            console.log("question (id="+questionId+") added at the end of survey...");
        }
        let res = this.saveNewPage(tmpStepID, newPage);
        if (!res){
            console.error("Setting of the next question failed : an already existing id (id="+tmpStepID+") was used as argument ");
            return null;
        }
        this.currentQuestionsBeingAnswered[this.stepID]  = questionIds
        this.stepID = tmpStepID + 1;
        this.setState({
            survey : tmpSurvey
        });
        return true
    }

    //TODO : comment it later
    deleteAnswersFromServer(){
        console.log('deleting answers from server...')
        let currentQuestions = this.getCurrentQuestions()
        this.setState({ loading: true }, () => {
            for (let id in currentQuestions) {
                let questionToDelete = currentQuestions[id]
                axios({ //sending by get
                    method: 'get',
                    url: Configuration.serverUrl + ':' + Configuration.serverPort + '/surveyapi/delanswer',
                    params : {
                        sessionid : this.sessionID,
                        questionid : questionToDelete.id
                    }
                })
                    .then(response => console.log(response)) //waiting the confirmation that the server received it
            }
        });
        this.setState({ //stopping the loading screen
            loading: false
        })
    }

    //TODO: not used yet in the new version, document it when it's done
    goBackToPreviousStep(){
        console.log('Going back to previous step...');
        this.deleteAnswersFromServer()
        this._surveyCompleted = false
        this.deleteMostRecentPageOfSurvey()
        this.pages = this.pages.slice(0, -1);
        this.stepID = this.stepID -1;
        console.log('Done going back to the previous step...');

    }

    //TODO: not used yet in the new version, document it when it's done
    deleteMostRecentPageOfSurvey(){
        let pageToDelete = this.getCurrentPage()
        let tmpSurvey = this.state.survey
        tmpSurvey.removePage(pageToDelete)
        this.setState({
            survey: tmpSurvey
        })
    }


    //allows to get the root element of the json received by the server
    //when receiving the survey from the server, they must be encapsulated in a root element, like data (by default with a nodejs server) or questions (or whatever you want)
    getRootFromJson(json){
        console.log('getting the root element from the json...')
        let root
        let cpt = 0
        for (let key in json){
            root = key
            cpt++
        }
        if(cpt > 1){
            console.error("ERROR : the json sent must be encapsulated in a UNIQUE root element(that contains all the data)")
            return null
        }
        if(!root){
            console.error("ERROR : the json received from the server is empty !")
            return null
        }
        return root
    }

    //TODO: not used yet in the new version, document it when it's done
    finishSurvey(){
        let tmpSurvey = this.state.survey
        tmpSurvey.completeText = "Complete"
        this._surveyCompleted = true
        this.setState({
            survey : tmpSurvey
        })
    }

    //TODO: not used yet in the new version, document it when it's done
    isThereAnotherQuestion(questionId){
        if  (questionId === "stop"){
            return false
        }
        return true
    }


    //TODO : partly done for the moment, wait until its finished before commenting
    processQuestionReceived(data){
        console.log("A question was received from the server")
        console.log("Data received by the server :")
        console.log(data)
        let questionIdToAdd = this.deserialize(data)
        console.log("These questions will be added at the end of the survey :")
        console.log(questionIdToAdd)
        this.setNextQuestionOfSurvey(questionIdToAdd)
        this.state.survey.nextPage()


        // let nextQuestionId = data.nextQuestionId
        // console.log("the next question id is " + nextQuestionId)
        // if (this.isThereAnotherQuestion(nextQuestionId)) {
        //     this.setNextQuestionOfSurvey(nextQuestionId)
        //     this.state.survey.nextPage()
        // } else{
        //     this.finishSurvey();
        // }

    }



    // transforms the json list of questions into objects, used later to manipulate the survey.
    // at this point, the questions are not added in the survey, they just "exist" as objects
    // warn : each question MUST have a unique id property
    deserialize(json){
        console.log("starting the deserialization...");

        let questionIds = [];
        const root = this.getRootFromJson(json)
        if (!root){
            console.error("ERROR : the json data received by the server is not adequate")
            return null
        }
        const questions = json[root];
        let cpt =0;

        for ( let key in questions){
            console.log("deserializing question number " + cpt);
            let tmpJsonQuestion = questions[key];
            let questionId = this.getJsonAttribute("id", tmpJsonQuestion)
            questionIds.push(questionId)
            let newQuestion = this.createQuestionObjectFromJson(tmpJsonQuestion);
            this.saveNewQuestion(questionId, newQuestion);
            cpt++;
        }

        console.log("deserialization complete of the " + cpt + " incoming questions")
        return(questionIds)
    }


    // allows to get attributes in question objects, like the id (a question shouldn't have an id attribute in surveyjs)
    // can be also used to get any attribute of any object
    // returns the attribute
    // warn : the attribute should be a direct attribute of the object (doesn't work with attributes of children)
    getJsonAttribute(attribute, jsonObject){
        console.log("getting attribute "+ attribute + "...");
        if (!jsonObject.hasOwnProperty(attribute)){
            console.error("the attribute " + attribute + " doesn't exist ! You MUST give an id attribute to each question in the JSON");
            return null;
        }
        console.log("attribute " + attribute + " get");
        return jsonObject[attribute];

    }

    //saves a new question into the Component properties
    // at this stage, the question is bound with an id, and only "exists" : it is not part of the survey yet
    saveNewQuestion(id, question){
        console.log("saving question with id="+id+"...");
        let tmpQuestions = this.questions;
        tmpQuestions[id] = question;
        this.questions = tmpQuestions;
        console.log("question with id="+id+" saved");
    }


    //saves a new page into the array of pages
    // its ID is the current stepID value
    saveNewPage(id, page){
        console.log("saving page with id="+id+"...");
        let tmpPages = this.pages;
        if (id in tmpPages){
            console.error("You want to add a page with the same id(id="+id+") as an already existing page !");
            return null;
        }

        tmpPages[id] = page;
        this.pages = tmpPages;
        console.log("page with id="+id+" saved");
        return true;
    }

    //converts a json formatted question into a Question class of surveyjs
    createQuestionObjectFromJson(questionJson) {
        if(!questionJson.type){
            console.error("no type found in question");
            return null;
        }
        const question = Survey.JsonObject.metaData.createClass(questionJson.type);
        if(!question){
            console.error("failed to create the question object from the json");
            return null;
        }
        Survey.JsonObject.prototype.toObject(questionJson, question);
        return question;
    }

    //checks that the user answered the question before submitting it (i.e its not an empty question)
    isAnswerEmpty(answer){
        if (!answer){
            return true
        }
        return false
    }

    //TODO : comment that
    getCurrentPage(){
        return this.state.survey.currentPage
    }

    // TODO : comment that
    getCurrentQuestions(){
        return this.state.survey.currentPage.questions
    }

    //returns the questions Ids and their answers
    // returned format : [{id : "ID of the question', answer : "user's answer"}, ... ]
    getAnswersOfCurrentPage(){
        let answers = []
        let cpt =0
        console.log('Getting answers from the current page...')
        const questionsList = this.getCurrentQuestions()
        for (let id in questionsList){
            let question = questionsList[id]
            if (!this.isAnswerEmpty(question.value)){
                answers[cpt] = {
                    id : question.id,
                    answer : question.value,
                    question : question
                }
                cpt++
            } else {
                console.error('Please answer the question')
                return null
            }
        }
        return answers
    }

    //TODO : this function must be completed and improved
    defineAnswerType(answer){
        if(!answer){
            console.error('The answer type cannot be determined since the answer is null or undefined !')
            return null
        }
        let questionType = null
        let question = answer.question
        if (question.getType() === 'geolocation'){
            questionType = 'geolocation'
        }
        else if (question.inputType === 'number'){
            questionType = 'number'
        }
        else{
            questionType = 'text'
        }
        console.log('The answer type of question #' + answer.id + ' is : ' + questionType)
        return questionType
    }


    //TODO : not finished yet properly, wait until commenting
    // the answer has these fields :
    // DESERIALISE_STRING(a->uid);
    // DESERIALISE_STRING(a->text);
    // DESERIALISE_LONGLONG(a->value);
    // DESERIALISE_LONGLONG(a->lat);
    // DESERIALISE_LONGLONG(a->lon);
    // DESERIALISE_LONGLONG(a->time_begin);
    // DESERIALISE_LONGLONG(a->time_end);
    // DESERIALISE_INT(a->time_zone_delta);
    // DESERIALISE_INT(a->dst_delta);
    serializeToCSV(lastAnswers){
        console.log("Serializing" + lastAnswers.length + " answers to CSV...")
        let lastAnswersCSV = []
        for(let id in lastAnswers){
            let cptForLogs = parseInt(id) + 1
            let lastAnswer = lastAnswers[id]
            let lastAnswerCSV = ""
            let questionType = this.defineAnswerType(lastAnswer)
            lastAnswerCSV = lastAnswerCSV.concat(lastAnswer.id, ':',lastAnswer.answer, ':0', ':0:0:0:0:0:0')
            console.log("Serialization of answer " + cptForLogs + "/" + lastAnswers.length + " done ! output = " + lastAnswerCSV)
            lastAnswersCSV[id] = lastAnswerCSV
        }
        return lastAnswersCSV
    }

    //TODO : change to POST
    // an answer is sent only if the user answered it
    sendAnswersToServer(lastAnswersCSV){
        console.log("sending answers to server...")

        for (let id in lastAnswersCSV){
            let answerToSend = lastAnswersCSV[id]
            console.log('requested URL : GET ' + Configuration.serverUrl + ':' + Configuration.serverPort + '/surveyapi/updateanswer?sessionid=' + this.sessionID + '&answer=' + answerToSend)
            this.setState({ loading: true }, () => {
                axios({ //sending by get
                    method: 'get',
                    url: Configuration.serverUrl + ':' + Configuration.serverPort + '/surveyapi/updateanswer',
                    params : {
                        sessionid : this.sessionID,
                        answer : answerToSend
                    }
                })
                    .then(response => console.log(response)) //waiting the confirmation that the server received it
                    .then(response => this.setState({ //stopping the loading screen
                        loading: false
                    }));
            });
        }

    }

    //TODO : function not finished yet, comment it later
    //function called when the user press Next button
    onNextButtonPressed(result, options){
        console.log("NEXT button pressed")
        //retrieve the last answer
        const lastAnswers = this.getAnswersOfCurrentPage()
        console.log(lastAnswers)
        if(!lastAnswers){
            return null
        }
        //format it to csv
        const lastAnswersCSV = this.serializeToCSV(lastAnswers)
        console.log("This data will be sent to the server :")
        console.log(lastAnswersCSV)
        //sending each answer, one by one
        this.sendAnswersToServer(lastAnswersCSV)
        //asking the next question
        this.askNextQuestion()
    }


    //sends a request to the server to get the next set of questions to display
    askNextQuestion(){
        console.log("Asking the first question...");
        console.log('URL used : ' + Configuration.serverUrl + ':' + Configuration.serverPort + '/surveyapi/nextquestion?sessionid=' + this.sessionID)
        axios({ //sending by get
            method: 'get',
            url: Configuration.serverUrl + ':' + Configuration.serverPort + '/surveyapi/nextquestion',
            params : {
                sessionid : this.sessionID,
            }
        })
            .then(response => this.processQuestionReceived(response.data))
    }


    getSessionIdFromServer(response){
        console.log("getting the session ID from the server...")
        console.log("session ID received : " + response)
        if (!response){
            console.error("the server sent nothing instead of the session id ! ")
            return null
        }
        this.sessionID = response
        return true
    }


    // function launched once at the beginning, that sets up the survey and ask to create a new session with a given survey id
    init(){
        console.log("version 7")
        console.log("Getting the Survey with ID="+ this.surveyID+"...");
        console.log("requesting " + Configuration.serverUrl + ':' + Configuration.serverPort + '/surveyapi/newsession?surveyid=' + this.surveyID)
        this.setState({ loading: true }, () => {
            axios({ //sending by get
                method: 'get',
                url: Configuration.serverUrl + ':' + Configuration.serverPort + '/surveyapi/newsession',
                params : {
                    surveyid : this.surveyID,
                }
            })
                .then(response => this.getSessionIdFromServer(response.data))
                .then(response => this.configureSurvey())
                .then(response => this.askNextQuestion())
                .then(response => this.setState({
                    loading: false
                }));
        });
    }


    //this function is fired when the page is loaded
    componentDidMount(){
        this.init();
    }



    // if an ajax request is loading, a spinner is shown. If a question is available, the survey is shown
    render(){
        console.log(this.pages)
        return(
            <div className="jumbotron jumbotron-fluid">
                <div className="container">
                    {this.state.loading ? <LoadingSpinner /> : <Survey.Survey model={this.state.survey}/>}
                </div>
            </div>
        );
    }
}

export default FlexibleSurvey;