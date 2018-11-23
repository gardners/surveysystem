/**
 * @module SurveyManager Simple state controller for survey progess.
 * There is not much to it as the state is controlled remotely by the backendx
 */
class SurveyManager {

    /**
     * @constructor
     * @param {string} surveyID The surveyID returned from backend. Note that ther is no logic attached to the property, it just houses it for convenience
     * @returns {void}
     */
    constructor(surveyID) {
        this.surveyID = surveyID;
        this.sessionID = null;
        this.step = -1;
        this.finished = false;
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
     * Returns the current question, which is always the last entry in this.questions
     * @returns {[object]} array of QuestionItems or empty array
     */
    current() {
        const { length } = this.questions;
        return (length) ? this.questions[length - 1] : [];
    }

    /**
     * Adds a new set of QuestionItems and sets progress flags
     * @param {[object]} questions array of QuestionItems, extracted from the backend response { next_questions: [ question1, question2 ...] }
     * @returns {void}
     */
    add(questions) {
        console.log(questions);
        if(!questions.length) {
            this.finished = true;
            return;
        }
        this.questions.push(questions);
        this.step += 1;
    }

    /**
     * Removes current (last) QuestionSet and sets progress flags
     * @returns {void}
     */
    back() {
        this.questions.splice(-1, 1);
        this.step -= 1
        this.finished = false;
    }
}

export default SurveyManager;
