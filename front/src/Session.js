import LocalStorage from './storage/LocalStorage';

import Answer from './Answer';
import { normalizeQuestions } from './Question';

const { REACT_APP_SURVEY_CACHEKEY } = process.env;

/**
 * compares two question sets by question ids, the exact order is required
 */
const load_cached_session = function() {
    if (!REACT_APP_SURVEY_CACHEKEY) {
        console.error('load_cached_session() env "REACT_APP_SURVEY_CACHEKEY" not set!');
        return null;
    }

    const cached = LocalStorage.get(REACT_APP_SURVEY_CACHEKEY);

    if (!cached) {
        return null;
    }

    if (!cached.survey_id || cached.cached_id || isNaN(cached.session_state)) {
        console.warn('load_cached_session() invalid cache survey_id and cached_id are required');
        return null;
    }

    const session = new Session();
    session.merge(cached);

    return session;
};

/**
 * compares two question sets by question ids, the exact order is required
 */
const save_cached_session = function(session) {
    if (!session) {
        return false;
    }

    const { session_id, survey_id, session_state, modified } = session;
    if (!survey_id || !session_id || isNaN(session_state)) {
        return false;
    }

    const cached = {
        survey_id,
        session_id,
        session_state,
        modified,
    };

    LocalStorage.set(REACT_APP_SURVEY_CACHEKEY, cached);
    if(!cached) {
        return false;
    }

    return true; //for promise chains
};

/**
 * compares two question sets by question ids, the exact order is required
 */
const delete_cached_session = function() {
    LocalStorage.delete(REACT_APP_SURVEY_CACHEKEY);
    return true; //for promise chains
};

/**
 * session states
 * @type {object}
 * @see survey.h  session_state
 */

export const SESSION_NULL      = 0; // init state; see constructor
export const SESSION_NEW       = 1;
export const SESSION_OPEN      = 2;
export const SESSION_FINISHED  = 3;
export const SESSION_CLOSED    = 4;

/**
 * @module Session is a simple state controller for survey progess.
 * There is not much to it as the state is controlled remotely by the backend
 */
class Session {

    /**
     * @constructor
     * @param {string} survey_id The survey_id returned from backend. Note that there is no logic attached to the property, it just houses it for convenience
     * @param {string} session_id The session id returned from backend, or stored in local storage
     * @returns {void}
     */
    constructor(survey_id, session_id) {
        // meta data
        this.survey_id = survey_id || null;
        this.session_id = session_id || null;

        // nextquestion response properties
        this.status = 0;
        this.message = '';
        this.progress = [-1, -1];
        this.next_questions = [];

        // control vars
        this.session_state = SESSION_NULL;
        this.modified = 0;
    }

    /**
     * check if session is open or new
     * @returns {boolean}
     */
    isOpen() {
        return (this.session_state >= SESSION_NEW && this.session_state <= SESSION_OPEN);
    }

    /**
     * check if session is finished
     * @returns {boolean}
     */
    isFinished() {
        return this.session_state === SESSION_FINISHED;
    }

    /**
     * check if session is finished or closed
     * @returns {boolean}
     */
    isCompleted() {
        return this.session_state >= SESSION_FINISHED;
    }

    /**
     * check if session is closed
     * @returns {boolean}
     */
    isClosed() {
        return this.session_state === SESSION_CLOSED;
    }

    /**
     * check if session is at the first question
     * @returns {boolean}
     */
    isFirst() {
        if (this.session_state < SESSION_OPEN || this.session_state >= SESSION_FINISHED) {
            return false;
        }
        return this.progress[0] === 0;
    }

    /**
     * close a session (before fetching analysis)
     * @returns {boolean}
     */
    close() {
        if (this.session_state < SESSION_FINISHED) {
            console.error("trying to close an unfinished session");
            return false;
        }
        this.session_state = SESSION_CLOSED;
        return true;
    }

    /**
     * Merges a cached session object
     * @param {object} cached deserialized JSON
     * @returns {boolean} flag indicating if cache matched survey an was merged
     */
    merge(values) {

        if(!values) {
            return;
        }

        let count = 0;
        const keys = Object.keys(values);

        keys.forEach((key) => {
            const type = typeof(this[key]);
            if(type === 'undefined' || type === 'function') {
                return;
            }
            if (key === 'next_questions') {
                this[key] = normalizeQuestions(values[key]);

                // an empty next_questions array means that the survey is closed
                if(!values[key].length) {
                    this.session_state = SESSION_FINISHED;
                } else {
                    this.session_state = SESSION_OPEN;
                }

                count++;
                return;
            }
            this[key] = values[key];

            count++;
        });

        this.modified = Date.now();
        return count;
    }

    getDefaultAnswers() {
        const answers = {};
        this.next_questions.forEach(question => {
            if (question.default_value) {
                const answerObj = Answer.create(question);
                const serialized = Answer.serialize(answerObj);
                if(serialized instanceof Error) {
                    return;
                }
                answers[question.id] = serialized;
            }
        });
        return answers;
    };

    getQuestionIds() {
        return this.next_questions.map(q => q.id);
    };

}

export {
    Session as default,
    load_cached_session,
    save_cached_session,
    delete_cached_session,
};
