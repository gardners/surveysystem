import Log from './Log';

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

        // progress
        this.step = -1;
        this.finished = false; // all questions are answered
        this.closed = false; // all questions are answered && survey is closed

        // questions
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
        this.finished = true;
        this.closed = true;
    }

    /**
     * check if suvey is finished
     * @returns {boolean}
     */
    isFinished() {
        return (this.finished && this.closed);
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

        if (this.closed){
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

            this[key] = cached[key];
        });

        return true;
    }

    /**
     * Adds a new set of QuestionItems and sets progress flags
     * @param {[object]} questions array of QuestionItems, extracted from the backend response { next_questions: [ question1, question2 ...] }
     * @returns {boolean} whether question was added
     */
    add(questions) {
        if(this.closed) {
            return false;
        }
        // TODO double submissions of current
        if(!questions.length) {
            this.finished = true;
        }
        this.questions.push(questions);
        this.step += 1;
        return true;
    }

    /**
     * Returns the current question, which is always the last entry in this.questions
     * @returns {[object]} array of QuestionItems or empty array
     */
    current() {
        const { length } = this.questions;
        return (length) ? this.questions[length - 1] : [];
    }

    /**
     * Removes current (last) QuestionSet and sets progress flags
     * @returns {boolean} whether step back was performed
     */
    back() {
        if(this.closed) {
            return false;
        }
        this.questions.splice(-1, 1);
        this.step -= 1
        this.finished = false;
        return true;
    }
}

export default SurveyManager;
