import Session, {
    SESSION_NULL,
    SESSION_NEW,
    SESSION_OPEN,
    SESSION_FINISHED,
    SESSION_CLOSED
} from '../Session';

import Q from '../Question';

// disable console warnings inside Session
beforeAll(() => {
    jest.spyOn(console, 'warn').mockImplementation(() => {});
    jest.spyOn(console, 'error').mockImplementation(() => {});
});

const now = Date.now();

// helpers

const create_question = function(uid, textval) {
        const q = Q.model();
        q.id = uid;
        q.type = 'TEXT';
        q.value = textval;
        return q;
};

const test_pristine = function(ses/*sion*/) {
    // not included: survey_id;
    // not included: session_id;

    expect(ses.status).toBe(0);
    expect(ses.message).toBe('');
    expect(ses.progress.toString()).toBe('-1,-1');
    expect(ses.next_questions.toString()).toBe('');

    expect(ses.session_state).toBe(SESSION_NULL);
    // not included: modified;
};

describe('Session', () => {

    test('constructor() defaults', () => {
        const ses = new Session();
        expect(ses.survey_id).toBe(null);
        expect(ses.session_id).toBe(null);
        test_pristine(ses);
        expect(ses.modified).toBe(0);
    });

    test('constructor() args', () => {
        const ses = new Session('surveyid', 'sessionid');
        expect(ses.survey_id).toBe('surveyid');
        expect(ses.session_id).toBe('sessionid');
        test_pristine(ses);
        expect(ses.modified).toBe(0);
    });

    test('merge(empty)', () => {
        let ses;

        ses = new Session('surveyid', 'sessionid');
        ses.merge();

        expect(ses.survey_id).toBe('surveyid');
        expect(ses.session_id).toBe('sessionid');
        test_pristine(ses);
        expect(ses.modified).toBe(0);

        ses = new Session('surveyid', 'sessionid');
        ses.merge(null);

        expect(ses.survey_id).toBe('surveyid');
        expect(ses.session_id).toBe('sessionid');
        test_pristine(ses);
        expect(ses.modified).toBe(0);
    });

    test('merge({vals})', () => {
        const ses = new Session('surveyid', 'sessionid');
        ses.merge({
            survey_id: 'surveyid',
            session_id: 'sessionid',
        });

        expect(ses.survey_id).toBe('surveyid');
        expect(ses.session_id).toBe('sessionid');
        test_pristine(ses);
        expect(ses.modified > now).toBe(true);
    });

    test('merge({unknown props})', () => {
        const ses = new Session('surveyid', 'sessionid');
        ses.merge({
            unknown: 'unknown'
        });

        expect(ses.survey_id).toBe('surveyid');
        expect(ses.session_id).toBe('sessionid');
        test_pristine(ses);
        expect(ses.modified > now).toBe(true);
    });

    test('merge({modified}) ignored', () => {
        const ses = new Session();

        ses.merge({
            survey_id: 'surveyid',
            session_id: 'sessionid',
            modified: 1,
        });

        expect(ses.survey_id).toBe('surveyid');
        expect(ses.session_id).toBe('sessionid');
        expect(ses.session_state).toBe(SESSION_NULL);
        expect(ses.modified > now).toBe(true);
    });

    test('merge({complete})', () => {
        const ses = new Session();

        const q1 = create_question('test1', 'val1');
        const q2 = create_question('test1', 'val2');
        const serialised_nq = JSON.stringify([q1, q2]);

        ses.merge({
            survey_id: 'surveyid',
            session_id: 'sessionid',
            status: 1,
            message: 'message',
            progress: [1, 1],
            next_questions: [q1, q2],
        });

        expect(ses.survey_id).toBe('surveyid');
        expect(ses.session_id).toBe('sessionid');
        expect(ses.status).toBe(1);
        expect(ses.message).toBe('message');
        expect(ses.progress.toString()).toBe('1,1');
        expect(JSON.stringify(ses.next_questions)).toBe(serialised_nq);
        expect(ses.session_state).toBe(SESSION_OPEN);
        expect(ses.modified > now).toBe(true);
    });

    test('trusted merge({invalid-properties}) DOES NOT VALIDATE properties', () => {
        const ses = new Session();

        ses.merge({
            survey_id: undefined,
            session_id: false,
            status: 'invalid',
            message: new Error(),
            progress: '[1,1]',
            next_questions: [null],
        });

        expect(ses.survey_id).toBe(undefined);
        expect(ses.session_id).toBe(false);
        expect(ses.status).toBe('invalid');
        expect(ses.message instanceof Error).toBe(true);
        expect(ses.progress).toBe('[1,1]');
        expect(JSON.stringify(ses.next_questions)).toBe(JSON.stringify([Q.model()]));
        expect(ses.modified > now).toBe(true);
    });

    test('getDefaultAnswers()', () => {
        const ses = new Session();

        const q1 = create_question('test1', 'val1');
        const q2 = create_question('test2', 'val2');
        const q3 = create_question('test3', 'val3');

        // no default for q1
        q2.default_value = 'default2';
        q3.default_value = 'default3';
        const serialised_nq = JSON.stringify([q1, q2, q3]);

        ses.merge({
            survey_id: 'surveyid',
            session_id: 'sessionid',
            status: 1,
            message: 'message',
            progress: [1, 1],
            next_questions: [q1, q2, q3],
        });

        expect(JSON.stringify(ses.next_questions)).toBe(serialised_nq);

        const answers = ses.getDefaultAnswers();
        expect(answers['test1']).toBe(undefined);
        expect(answers['test2']).toBe('test2:default2:0:0:0:0:0:0:0');
        expect(answers['test3']).toBe('test3:default3:0:0:0:0:0:0:0');
    });

    test('getQuestionIds()', () => {
        let ids;

        const ses = new Session();

        ids = ses.getQuestionIds();
        expect(ids.toString()).toBe('');

        // populate
        const q1 = create_question('test1', 'val1');
        const q2 = create_question('test2', 'val2');
        const serialised_nq = JSON.stringify([q1, q2]);

        ses.merge({
            survey_id: 'surveyid',
            session_id: 'sessionid',
            status: 1,
            message: 'message',
            progress: [1, 1],
            next_questions: [q1, q2],
        });

        ids = ses.getQuestionIds();
        expect(ids.toString()).toBe('test1,test2');
    });

});

describe ('Session session_state and status methods', () => {

    test('merge({vals}) SESSION_NULL', () => {
        const ses = new Session();

        ses.merge({
            survey_id: 'surveyid',
            session_id: 'sessionid',
        });

        expect(ses.survey_id).toBe('surveyid');
        expect(ses.session_id).toBe('sessionid');
        expect(JSON.stringify(ses.next_questions)).toBe('[]');

        expect(ses.session_state).toBe(SESSION_NULL);

        expect(ses.isOpen()).toBe(false);
        expect(ses.isFinished()).toBe(false);
        expect(ses.isClosed()).toBe(false);
    });

    test('merge({vals}) SESSION_NEW (partially implemented)', () => {
        const ses = new Session();

        ses.merge({
            survey_id: 'surveyid',
            session_id: 'sessionid',
        });
        ses.session_state = SESSION_NEW;

        expect(ses.isOpen()).toBe(true);
        expect(ses.isFinished()).toBe(false);
        expect(ses.isClosed()).toBe(false);
    });

    test('merge({vals}) SESSION_OPEN', () => {
        const ses = new Session();

        const q = create_question('test1', 'val1');
        const serialised_nq = JSON.stringify([q]);

        ses.merge({
            survey_id: 'surveyid',
            session_id: 'sessionid',
            next_questions: [q],
        });

        expect(ses.survey_id).toBe('surveyid');
        expect(ses.session_id).toBe('sessionid');
        expect(JSON.stringify(ses.next_questions)).toBe(serialised_nq);

        expect(ses.session_state).toBe(SESSION_OPEN);

        expect(ses.isOpen()).toBe(true);
        expect(ses.isFinished()).toBe(false);
        expect(ses.isClosed()).toBe(false);
    });

    test('merge({vals}) SESSION_FINISHED', () => {
        const ses = new Session();

        ses.merge({
            survey_id: 'surveyid',
            session_id: 'sessionid',
            next_questions: [],
        });

        expect(ses.survey_id).toBe('surveyid');
        expect(ses.session_id).toBe('sessionid');
        expect(JSON.stringify(ses.next_questions)).toBe('[]');

        expect(ses.session_state).toBe(SESSION_FINISHED);

        expect(ses.isOpen()).toBe(false);
        expect(ses.isFinished()).toBe(true);
        expect(ses.isClosed()).toBe(false);
    });

    test('merge({vals}) SESSION_CLOSED', () => {
        const ses = new Session();

        ses.merge({
            survey_id: 'surveyid',
            session_id: 'sessionid',
            next_questions: [],
        });
        ses.close(); // !!

        expect(ses.survey_id).toBe('surveyid');
        expect(ses.session_id).toBe('sessionid');
        expect(JSON.stringify(ses.next_questions)).toBe('[]');

        expect(ses.session_state).toBe(SESSION_CLOSED);

        expect(ses.isOpen()).toBe(false);
        expect(ses.isFinished()).toBe(false);
        expect(ses.isClosed()).toBe(true);
    });

    test('SESSION_CLOSED cannot regress (#408)', () => {

        const ses = new Session();
        ses.merge({
            survey_id: 'surveyid',
            session_id: 'sessionid',
            next_questions: [],
        });
        ses.close(); // !!
        expect(ses.session_state).toBe(SESSION_CLOSED);

        ses.merge({
            survey_id: 'surveyid',
            session_id: 'sessionid',
            next_questions: [],
        });
        expect(ses.session_state).toBe(SESSION_CLOSED);

    });

    test('close({vals}) SESSION_CLOSED', () => {

        const ses = new Session();

        ses.close();
        expect(ses.session_state).toBe(SESSION_NULL);

        ses.session_state = SESSION_NEW;
        ses.close();
        expect(ses.session_state).toBe(SESSION_NEW);

        ses.session_state = SESSION_OPEN;
        ses.close();
        expect(ses.session_state).toBe(SESSION_OPEN);

        ses.session_state = SESSION_FINISHED;
        ses.close();
        expect(ses.session_state).toBe(SESSION_CLOSED);

        ses.session_state = SESSION_CLOSED;
        ses.close();
        expect(ses.session_state).toBe(SESSION_CLOSED);
    });
});
