import Log from './Log';

/**
 * Builds a colon-separated sorted string of question ids from a questions entry. This allows to shallow compare querstions entries
 * @param {[object]} questions array of QuestionItem
 * @returns {string}
 */
const questionsID = function(questions) {
    return questions.map(q => q.id).sort().join(':');
};

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
        this.steps = 0;

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
            Log.warn(`SessionManager.merge: surveyID:sessionID supplied values doesn't match ${this.surveyID}:${this.sessionID}`);
            return false;
        }

        Object.keys(cached).forEach((key) => {
            const type = typeof(this[key]);
            if(type === 'undefined' || type === 'function') {
                return;
            }
            if(key === 'questions') {
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
    add(questions) {
        if(this.isClosed()) {
            return false;
        }

        if(!questions.length) {
            this.close();
        }

        // remove
        if(this.questions.length) {
            this.questions.slice(1);
        }

        this.questions.push(questions);
        return true;
    }

    /**
     * Clears this.questions and adds current set of QuestionItems
     * @param {[object]} questions array of QuestionItems, extracted from the backend response { next_questions: [ question1, question2 ...] }
     * @returns {boolean} whether question was added
     */
    reset(questions) {
        if(this.isClosed()) {
            return false;
        }

        if(!questions.length) {
            this.close();
        }

        // remove
        this.questions.splice(0, this.questions.length);
        this.questions.push(questions);
        return true;
    }

    /**
     * increments progress flags
     * @param {[object]} questions array of QuestionItems, extracted from the backend response { next_questions: [ question1, question2 ...] }
     * @returns {boolean} whether question was added
     */
    increment() {
        this.steps += 1;
        return this.steps;
    }

    /**
     * decrements progress flags
     * @param {[object]} questions array of QuestionItems, extracted from the backend response { next_questions: [ question1, question2 ...] }
     * @returns {boolean} whether question was added
     */
    decrement() {
        this.steps -= 1;
        return this.steps;
    }

    /**
     * return steps
     * @param {[object]} questions array of QuestionItems, extracted from the backend response { next_questions: [ question1, question2 ...] }
     * @returns {boolean} whether question was added
     */
    steps() {
        return this.steps;
    }

    /**
     * Returns the current set of QuestionItems
     * @returns {[object]} array of QuestionItems or empty array
     */
    current() {
        return (this.questions.length) ? this.questions[this.questions.length - 1] : [];
    }

    /**
     * Returns the previous set of QuestionItems
     * @returns {[object]} array of QuestionItems or empty array
     */
    previous() {
        return (this.questions.length) ? this.questions[0] : [];
    }

}

export { SurveyManager as default, questionsID };
