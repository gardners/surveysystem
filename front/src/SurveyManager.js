import { normalizeQuestions } from './Question';

/**
 * @module The survey manager keeps track and manipulates of the question history. The module is designed to be used by LocalStorage.
 * Thus, answers are not considered here due privacy concerns.
 */

/**
 * Builds a colon-separated sorted string of question ids from a questions entry. This allows to shallow compare querstions entries
 * @param {[object]} questions array of QuestionItem
 * @returns {string}
 */
const questionIDs = function(questions) {
    return questions.map(q => q.id).sort().join(':');
};

/**
 * compares two question sets by question ids, the exact order is required
 */
const matchQuestionIds = function(first, second) {
    if(first.length !== second.length) {
        return false;
    }

    for (let i = 0; i < first.length; i +=1) {
        if(first[i].id !== second[i].id) {
            return false;
        }
    }
    return true;
};

// /**
//  * create an array of question objects with the minimal required properties from an api response.
//  * Since we store the history locally it makes no sense to store everything
//  * @param {[object]} questions array of json response next_question objects
//  * @returns {[object]}
//  */
//
// const processQuestions = function(questions) {
//     return questions.map(question => ({
//         id: question.id,
//     }));
// };

/**
 * @module SurveyManager Simple state controller for survey progess.
 * There is not much to it as the state is controlled remotely by the backendx
 */
class SurveyManager {

    /**
     * @constructor
     * @param {string} surveyID The surveyID returned from backend. Note that there is no logic attached to the property, it just houses it for convenience
     * @param {string} endpoint survey api endpoint
     * @returns {void}
     */
    constructor(surveyID, endpoint) {
        // meta data
        this.surveyID = surveyID || null;
        this.sessionID = null;
        this.endpoint = endpoint;
        this.closed = false; // all questions are answered && survey is closed

        this.created = Date.now();
        this.modified = 0;

        // #333 add status, message from last response
        this.status = 0;
        this.message = '';

        // questions
        // this is an array of question sets with the length of 2 [previousquestions, currentquestions]
        this.questions = []; // QuestionSets
    }

    /**
     * Initializes session
     * @param {string} sessionID The sessionID returned from backend. Note that ther is no logic attached to the property, it just houses it for convenience
     * @returns {[object]} array of QuestionItems or empty array
     */
    init(sessionID) {
        this.sessionID = sessionID;
    }

    /**
    * Finalizes session
    * @returns {void}
    */
    close() {
        this.closed = true;
        return this.closed;
    }

    /**
    * check if suvey is finished
    * @returns {boolean}
    */
    isClosed() {
        return this.closed;
    }

    /**
     * Validates if a cache object can be merged
     * @param {object} cached deserialized JSON
     * @returns {boolean} flag indicating if cache can be merged
     */
    canMerge(cached) {

        if(!cached) {
            return false;
        }

        const cachedSurveyID = cached.surveyID || null;
        const cachedSessionID = cached.sessionID || null;
        const cachedEndpoint = cached.endpoint || null;

        // this without sessionID
        // cache with sessionID
        // cache with surveyID

        // this with endpoint
        // cache with endpoint
        // this endpoint and cache endpoint match

        // surveyIDs match
        // !closed

        // keeping code readable
        if (!this.surveyID || this.surveyID !== cachedSurveyID){
            return false;
        }

        if (!cachedSessionID || this.sessionID){
            return false;
        }

        if (!cachedEndpoint || !this.endpoint){
            return false;
        }

        if (cachedEndpoint !== this.endpoint){
            return false;
        }

        if (this.isClosed()){
            return false;
        }

        return true;
    }

    /**
     * Merges a cached session object
     * @param {object} cached deserialized JSON
     * @returns {boolean} flag indicating if cache matched survey an was merged
     */
    merge(cached) {
        if(!this.canMerge(cached)) {
            console.warn(`SessionManager.merge: surveyID:sessionID supplied values doesn't match ${this.surveyID}:${this.sessionID}`);
            return false;
        }

        Object.keys(cached).forEach((key) => {
            const type = typeof(this[key]);
            if(type === 'undefined' || type === 'function') {
                return;
            }
            if (key === 'questions') {
                this.questions = cached.questions.map(q => normalizeQuestions(q));
                return;
            }
            this[key] = cached[key];
        });

        return true;
    }

    /**
     * Registers the current set of QuestionItems
     * @param {[object]} questions array of QuestionItems, extracted from the backend response { next_questions: [ question1, question2 ...] }
     * @returns {boolean} whether question was added
     */
    add(questions, status, message) {
        if(this.isClosed()) {
            return false;
        }

        if(!questions.length) {
            this.close();
            return false;
        }

        // deal with cases like browser refresh..
        if (matchQuestionIds(this.current(), questions)) {
            return false;
        }

        // #333 add status, message from last response
        this.status = status || 0;
        this.message = message || '';

        this.questions.push(normalizeQuestions(questions));

        this.modified = Date.now();
        return true;
    }

    /**
     * Replace current questions with new questions
     * This should be only used in conjunction with Api.delanswerandfollwing() responses,
     * Typically the question matches the same as the current() question set (after calling reset() before), but now contains default_values.
     * However due to the dynamic nature of the survey it have to be that way which may be a problem. Thus we hard replace instead try to merge. We have to trust the controller script here to handle this responsibly
     *
     * @param {[object]} questions array of QuestionItems, extracted from the backend response { next_questions: [ question1, question2 ...] }
     * @returns {boolean} whether question was areplaced
     */
    replaceCurrent(questions, status, message) {
        if(this.isClosed()) {
            return false;
        }

        if(!questions.length) {
            this.close();
            return false;
        }

        // #333 add status, message from last response
        this.status = status || 0;
        this.message = message || '';

        if (!this.questions.length) {
            this.questions.push(normalizeQuestions(questions));
        } else {
            this.questions[this.questions.length - 1] = normalizeQuestions(questions);
        }

        this.modified = Date.now();
        return true;
    }

    /**
     * Removes last set of questions, clears status and message
     * @returns {boolean} whether question was added
     */
    reset() {
        if (this.isClosed()) {
            return false;
        }

        if (!this.questions.length) {
            return false;
        }

        // #333 add status, message
        this.status =  0;
        this.message = '';

        this.questions.pop();

        this.modified = Date.now();
        return true;
    }

    /**
     * Returns the current set of QuestionItems
     * @returns {[object]} array of QuestionItems or empty array
     */
    current() {
        return (this.questions.length) ? this.questions[this.questions.length - 1] : [];
    }

}

export { SurveyManager as default, questionIDs, matchQuestionIds };
