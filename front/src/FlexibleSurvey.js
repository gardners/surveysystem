import React from 'react';
import * as Survey from "survey-react";
import axios from 'axios';
import { Configuration } from './conf/config';
import LoadingSpinner from './LoadingSpinner';



// The most important class of the Application
// Represents the Flexible Survey
class FlexibleSurvey extends React.Component {

    //The ID of the survey to load from the server
    //the ID is retrieved from the url, that looks like : /survey/:id
    surveyID = this.props.match.params.id;
    //The ID of the session, as identified by the back end
    //TODO : get the session ID
    sessionID = 123456;
    //contains an array listing all question objects, by this way : [questionId => {question}, etc...]
    questions = [];
    //contains an array listing all added pages(and the question it contains) objects to the survey, by this way : [pageId => {page}, etc...]
    pages = [];
    //allows to know the number of pages currently in the survey. It is also used to set the pageId of a page
    // the stepId is incremented by 1 each time a new question is added to the survey
    stepID = 0;
    //stores the ID of the current question being answered by the user
    //easy access to the question
    currentQuestionBeingAnswered = null;
    //a "trick" used to always display the next button (instead of the end survey one) in the survey
    // an empty page is always inserted each time a new question is added
    //so, the current question will never be the last one (and so, the button next will be showed instead of end survey)
    // since its a trick, it would be better to change that to a cleaner way
    lastPage = null;
    //is set to true when the survey is changing its structure
    _surveyChangingStructure = false


    tmpSurvey = null



    //delete thant, only for tests
    testArray = [1,4,8,11];



    // Constructor, needed in every case in react
    // Also instantiate a new Survey (the not flexible one) and its style
    // loading is used to know if an ajax request is being made
    constructor(props) {
        console.info("function called : constructor")
        super(props);
        this.state = {
            survey : new Survey.Model(),
            loading : true
        };
        Survey.StylesManager.applyTheme(Configuration.surveyTheme);
    }


    updateThisSurvey(survey){
        console.info("function called : updatesurvey")
        console.log("updating the survey....")
        this.setState({
            survey : survey
        });
    }
    //instantiate the lastPage property, see above to more explanations
    initLastPage(){
        console.info("function called : initLastPage")
        this.tmpSurvey = this.state.survey;
        this.lastPage = new Survey.PageModel("lastPage");
        this.lastPage.addNewQuestion("text", "");
        this.updateThisSurvey(this.tmpSurvey)
    }


    //adds the needed EventListeners at the init of the survey. It set the functions to call when a button is pressed, a page is changed, etc.
    addEventListeners(){
        console.info("function called : addEventListeners")
        this.tmpSurvey = this.state.survey;

        //event listener when the user press next
        this.tmpSurvey.sendResultOnPageNext = true;
        this.tmpSurvey.onPartialSend.add(function (result, options){
            this.onNextButtonPressed(result, options)
        }.bind(this));

        //event listener when user press prev
        this.tmpSurvey.onCurrentPageChanged.add(function(sender, options){
             if(!options.oldCurrentPage || !options.newCurrentPage || this._surveyChangingStructure) {
                 console.log("the page is changing but no need to use the function goBackToPreviousStep()")
                 return
             }
             if(options.oldCurrentPage.visibleIndex > options.newCurrentPage.visibleIndex){
                 this.goBackToPreviousStep();
             }
         }.bind(this))


    }


    //get a question saved in the component's properties by its id
    getQuestionById(id){
        console.info("function called : getQuestionById")
        const tmpQuestions = this.questions;
        if (!(id in tmpQuestions)){
            console.error("There is no questions defined by the id "+id);
            return null;
        }
        return tmpQuestions[id];
    }

    getPageById(id){
        console.info("function called : getPageById")
        let tmpPages = this.pages
        if (!(id in tmpPages)){
            console.error("There is no page defined by the id "+id);
            return null;
        }
        return tmpPages[id];
    }

    //allows to add a new question at the end of the survey.
    //The question must already exist
    // workflow :
    // retrieve the question by it's ID
    // remove the lastPage from the survey (see lastPage property)
    // add a new page with the question at the end of the survey
    // add the lastPage at the end of the survey (see lastPage property)
    setNextQuestionOfSurvey(questionId){
        console.info("function called : setNextQuestionOfSurvey")
        console.log("adding the next question (id="+questionId+") at the end of survey...");
        const questionToAdd = this.getQuestionById(questionId);
        if (!questionToAdd){
            console.error("failed to get the question with id "+questionId+" !");
            return null;
        }
        this.tmpSurvey = this.state.survey;
        let tmpStepID = this.stepID;
        this.removeLastPage();
        let newPage = this.tmpSurvey.addNewPage(tmpStepID);
        newPage.addQuestion(questionToAdd);
        let res = this.saveNewPage(tmpStepID, newPage);
        if (!res){
            console.error("Setting of the next question failed : an already existing id (id="+tmpStepID+") was used as argument ");
            return null;
        }
        this.stepID = tmpStepID + 1;
        this.tmpSurvey = this.addLastPage(this.tmpSurvey);
        this.currentQuestionBeingAnswered  = questionId
        this.updateThisSurvey(this.tmpSurvey)
        console.log("question (id="+questionId+") added at the end of survey...");
        return true;
    }

    goBackToPreviousStep(){
        console.info("function called : goBackToPreviousStep")
        console.log('Going back to previous step...');
        this.deleteMostRecentPageOfSurvey()
        this.pages = this.pages.slice(0, -1);
        this.stepID = this.stepID -1;
        console.log('Done going back to the previous step...');

    }

    //function used when the user presses the previous button
    // it deletes the lastPage
    // then deletes the most recent question that was sent by the server
    // then adds again the lastPage
    deleteMostRecentPageOfSurvey(){
        console.info("function called : deleteMostRecentPageOfSurvey")
        let pageToDelete = this.getPageById(this.stepID-1)
        this.tmpSurvey = this.state.survey
        this.removeLastPage(this.tmpSurvey,this.stepID)
        this.tmpSurvey.removePage(pageToDelete)
        this.addLastPage(this.tmpSurvey)
        this.updateThisSurvey(this.tmpSurvey)
    }


    //allows to get the root element of the json received by the server
    //when receiving the survey from the server, they must be encapsulated in a root element, like data (by default with a nodejs server) or questions (or whatever you want

    getRootFromJson(json){
        console.info("function called : getRootFromJson")
        console.log('getting the root element from the json...')
        console.log("INFO : the received json file is :")
        console.log(json)
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

    isThereAnotherQuestion(questionId){
        console.info("function called : isThereAnotherQuestion")
        if  (questionId === "stop"){
            return false
        }
        return true
    }

    finishSurvey(){
        console.info("function called : finishSurvey")
        this.tmpSurvey = this.state.survey
        this.tmpSurvey = this.removeLastPage(this.tmpSurvey, this.stepID)
        this.updateThisSurvey(this.tmpSurvey)

    }

    //function to parse the json containing the next questionID, and add the question to the survey
    processQuestionIdReceived(data){
        console.info("function called : processQuestionIdReceived")
        console.log("Next question id received from the server...")
        let nextQuestionId = data.nextQuestionId
        console.log("the next question id is " + nextQuestionId)
        if (this.isThereAnotherQuestion(nextQuestionId)) {
            this.setNextQuestionOfSurvey(nextQuestionId)
        } else{
            this.finishSurvey();
        }

    }

    // transforms the json list of questions into objects, used later to manipulate the survey.
    // at this point, the questions are not added in the survey, they just "exist"
    // warn : each question MUST have a unique id attribute
    deserialize(json){
        console.info("function called : deserialize")
        console.log("starting the deserialization...");

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
            let questionId = this.getJsonAttribute("id", tmpJsonQuestion);
            let jsonQuestion = this.removeJsonAttribute("id",tmpJsonQuestion);
            let newQuestion = this.createQuestionObjectFromJson(jsonQuestion);
            this.saveNewQuestion(questionId, newQuestion);
            cpt++;
        }
        console.log("deserialization complete");
    }


    // allows to get attributes in question objects, like the id (a question shouldn't have an id attribute in surveyjs)
    // can be also used to get any attribute of any object
    // returns the attribute
    // warn : the attribute should be a direct attribute of the object (doesn't work with attributes of children)
    getJsonAttribute(attribute, jsonObject){
        console.info("function called : getJsonAttribute")
        console.log("getting attribute "+ attribute + "...");
        if (!jsonObject.hasOwnProperty(attribute)){
            console.error("the attribute " + attribute + " doesn't exist ! You MUST give an id attribute to each question in the JSON");
            return null;
        }
        console.log("attribute " + attribute + " get");
        return jsonObject[attribute];

    }


    // allows to remove attributes in question objects, like the id (a question shouldn't have an id attribute in surveyjs)
    // can be also used to remove any attribute of any object
    // returns a new object
    // warn : the attribute should be a direct attribute of the object (doesn't work with attributes of children)
    removeJsonAttribute(attribute, jsonObject){
        console.info("function called : removeJsonAttribute")
        console.log("removing attribute "+ attribute + "...");
        let res = jsonObject;
        if (!res.hasOwnProperty(attribute)){
            console.error("the attribute " + attribute + " doesn't exist ! You MUST give an id attribute to each question in the JSON");
        } else {
            delete res[attribute];
            console.log("attribute " + attribute + " removed");
        }
        return res;
    }


    //saves a new question into the Component properties
    // at this stage, the question is bound with an id, and only "exists" : it is not part of the survey yet
    saveNewQuestion(id, question){
        console.info("function called : saveNewQuestion")
        console.log("saving question with id="+id+"...");
        let tmpQuestions = this.questions;
        if (id in tmpQuestions){
            console.error("You want to add a question with the same id(id="+id+") as an already existing question !");
            return null;
        }
        tmpQuestions[id] = question;
        this.questions = tmpQuestions;
        console.log("question with id="+id+" saved");
    }


    //saves a new page into the array of pages
    // its ID is the current stepID value
    saveNewPage(id, page){
        console.info("function called : saveNewPage")
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

    //adds a phantom page at the end of the survey, see lastPage property doc for more details
    addLastPage(survey){
        console.info("function called : addLastPage")
        survey.addPage(this.lastPage);
        return survey;
    }

    //removes the last page at the end of the survey, see lastPage property doc for more details
    removeLastPage(){
        console.info("function called : removeLastPage")
        this._surveyChangingStructure = true
        console.log("removing last page...");
        if (this.stepID !== 0){
            this.tmpSurvey.removePage(this.lastPage);
            console.log("last page removed !");
        } else {
            console.log("there is no last page to remove, stepID=0 !");
        }
        console.log("removal of last page finished...");
        this._surveyChangingStructure = false

    }

    //converts a json formatted question into a Question class of surveyjs
    createQuestionObjectFromJson(questionJson) {
        console.info("function called : createQuestionObjectFromJson")
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

    //get the last question that was answered by the user in a json format
    // BE CAREFUL : IT ONLY WORKS FOR SIMPLE ANSWERS(no matrix, nested answers, etc)
    //TODO : improve it later
    getLastAnswer(data){
        console.info("function called : getLastAnswer")
        console.log("Getting the latest answer...")
        if (Object.keys(data).length === 0 && data.constructor === Object){
            console.error("ERROR : The data of the last answer is empty !");
            return null;
        }
        let lastKey
        for(let key in data){
            if(data.hasOwnProperty(key)){
                lastKey = key;
            }
        }
        const result = {
            id : this.currentQuestionBeingAnswered,
            key : lastKey,
            value : data[lastKey]
        }
        console.log("Done ! key = "+ result.key + " value = "+ result.value)
        return result
    }

    //TODO : add number serialization
    //TODO : escape the ":"
    //IMPORTANT : this function will probably be not complete enough when te project will evolve
    //serialize the json to a csv format for the back end
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
    serializeToCSV(jsonObject){
        console.log("Serializing the answer to CSV...")
        let csvResult = ""
        csvResult = csvResult.concat(jsonObject.id, ':',jsonObject.value, ':0', ':0:0:0:0:0:0')
        console.log("Serialization done ! output = " + csvResult)
        return csvResult
    }


    //function called when the user press Next button
    onNextButtonPressed(result, options){
        console.info("function called : onNextButtonPressed")
        console.log("NEXT button pressed")
        const lastAnswer = this.getLastAnswer(result.data)
        if(!lastAnswer){
            console.error("ERROR : no answer found ! ")
            return null
            //TODO later : when an answer is empty, make an appropriate function to handle it. NOTE : if you make a question mandatory, there is no need to test that the lastAnswer is empty, it will never be
        }
        //format it to csv
        //const csvBody = this.serializeToCSV(lastAnswer)
        this.setState({ loading: true }, () => {
            axios({ //sending by post
                method: 'post',
                url: Configuration.serverUrl + ':' + Configuration.serverPort + '/addAnswer/session/' + this.sessionID,
                data: lastAnswer
            })
            .then(response => console.log(response)) //waiting the confirmation that the server received it
            .then(response => axios.get(Configuration.serverUrl + ':' + Configuration.serverPort + '/nextQuestion/session/' + this.sessionID)) //ask next question
            .then(response => this.processQuestionIdReceived(response.data))
            .then(response => this.setState({ //stopping the loading screen
                loading: false
            }));
        });
    }

    //asking the first question of the survey when it is initializing
    askFirstQuestion(){
        console.info("function called : askFirstQuestion")
        console.log("Asking the first question...");
        axios.get(Configuration.serverUrl + ':' + Configuration.serverPort + '/nextQuestion/session/' + this.sessionID)
            .then(response => this.processQuestionIdReceived(response.data))
    }



    // init function that retrieves the survey from the back end
    // then deserialize them (JSON to Object)
    // then instantiate the last page (see doc at the beginning)
    // then adds an event handler on the Next button
    // it is done asynchronously !!!!
    // axios is the library for requests
    // a loading screen is showed while the the ajax request is not finished
    init(){
        console.info("function called : init")
        console.log("Getting the Survey with ID="+ this.surveyID+"...");
        this.setState({ loading: true }, () => {
            axios.get(Configuration.serverUrl + ':' + Configuration.serverPort + '/survey/' + this.surveyID + '/newSession')
                .then(response => this.deserialize(response.data))
                .then(response => this.initLastPage())
                .then(response => this.addEventListeners())
                .then(response => this.askFirstQuestion())
                .then(response => this.setState({
                    loading: false
                }));
        });
    }

    //this function is fired when the page is loaded
    componentDidMount(){
        console.info("function called : componentDidMount")

        this.init();
    }


    //{this.state.loading ? <LoadingSpinner /> : <Survey.Survey model={this.state.survey}/>}
    // if an ajax request is loading, a spinner is shown. If a question is available, the survey is shown
    render(){
        console.info("function called : render")
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