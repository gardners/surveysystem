import SurveyManager, { questionIDs, matchQuestionIds } from '../SurveyManager';
import Q from '../Question';

describe('questionIDs', () => {

    test('format and sorting', () => {
        let qid;

        qid = questionIDs([]);
        expect(qid).toBe('');

        qid = questionIDs([{id: 'A' }]);
        expect(qid).toBe('A');

        qid = questionIDs([{id: 'A' }, {id: 'A' }]);
        expect(qid).toBe('A:A');

        qid = questionIDs([{id: 'A' }, {id: 'B' }]);
        expect(qid).toBe('A:B');

        qid = questionIDs([{id: 'B' }, {id: 'A' }]);
        expect(qid).toBe('A:B');
    });

    test('equality', () => {
        let qid1;
        let qid2;

        qid1 = questionIDs([]);
        qid2 = questionIDs([]);
        expect(qid1).toBe(qid2);

        qid1 = questionIDs([{id: 'A' }]);
        qid2 = questionIDs([{id: 'A' }]);
        expect(qid1).toBe(qid2);

        qid1 = questionIDs([{id: 'A' }, {id: 'A' }]);
        qid2 = questionIDs([{id: 'A' }, {id: 'A' }]);
        expect(qid1).toBe(qid2);

        qid1 = questionIDs([{id: 'A' }, {id: 'B' }]);
        qid2 = questionIDs([{id: 'B' }, {id: 'A' }]);
        expect(qid1).toBe(qid2);
    });

    test('inequality', () => {
        let qid1;
        let qid2;

        qid1 = questionIDs([]);
        qid2 = questionIDs([{id: 'A' }]);
        expect(qid1).not.toBe(qid2);

        qid1 = questionIDs([{id: 'A' }]);
        qid2 = questionIDs([{id: 'A' }, {id: 'A' }]);
        expect(qid1).not.toBe(qid2);

        qid1 = questionIDs([{id: 'A' }, {id: 'B' }]);
        qid2 = questionIDs([{id: 'B' }, {id: 'C' }]);
        expect(qid1).not.toBe(qid2);
    });

});

describe('matchQuestionIds', () => {

    test('equality', () => {
        let match;

        match = matchQuestionIds([], []);
        expect(match).toBe(true);

        match = matchQuestionIds([{id: 'A' }], [{id: 'A' }]);
        expect(match).toBe(true);

        match = matchQuestionIds([{id: 'A' }, {id: 'B' }], [{id: 'A' }, {id: 'B' }]);
        expect(match).toBe(true);
    });

    test('inequality', () => {
        let match;

        match = matchQuestionIds([], [{id: 'A' }]);
        expect(match).toBe(false);

        match = matchQuestionIds([{id: 'A' }], []);
        expect(match).toBe(false);

        match = matchQuestionIds([{id: 'A' }], [{id: 'A' }, {id: 'B' }]);
        expect(match).toBe(false);

        match = matchQuestionIds([{id: 'A' }, {id: 'B' }], [{id: 'A' }, {id: 'C' }]);
        expect(match).toBe(false);
    });

    test('inequality(strict order)', () => {
        let match;

        match = matchQuestionIds([{id: 'A' }, {id: 'B' }], [{id: 'B' }, {id: 'A' }]);
        expect(match).toBe(false);

        // #379
        match = matchQuestionIds([], []);
        expect(match).toBe(true);
    });

});

describe('SurveyManager.merge', () => {

    test('merge - replace', () => {
        const that = new SurveyManager('test', 'uri');
        that.merge({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',
            questions: [[{ id: 1 }]]
        })

        expect(that).toMatchObject({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',
        });

        const m1 = Q.model();
        m1.id = 1;

        expect(JSON.stringify(that.questions)).toBe(JSON.stringify([[m1]]));
    });

    test('merge - dont include properties unkown to the instance', () => {
        const that = new SurveyManager('test', 'uri');
        that.merge({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',
            unknownProp: true,
        })

        expect(that).not.toContain('unknownProp');
    });

    test('merge - dont overwrite methods with values', () => {
        const that = new SurveyManager('test', 'uri');
        that.merge({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',
            init: true,
        })

        expect(that.surveyID).toBe('test');
        expect(typeof(that.init)).toBe('function');
    });

    test('merge - overwrite methods with functions', () => {
        const that = new SurveyManager('test', 'uri');
        that.merge({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',
            init: () => 'overwritten',
        })
        const undef = that.init('123');

        expect(that.sessionID).toBe('123');
        expect(undef).toBe(undefined);
    });

    // #332, #406
    test('merge - default status, message and progress', () => {
        const that = new SurveyManager('test', 'uri');
        that.merge({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',
        })
        expect(that).toMatchObject({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',
            status: 0,
            message: '',
            progress: [-1, -1],
        });
    });

    // #332, #406
    test('merge - custom  status, message and progress', () => {
        const that = new SurveyManager('test', 'uri');
        that.merge({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',

            status: 1,
            message: 'Hello',
            progress: [1, 2],
        })
        expect(that).toMatchObject({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',

            status: 1,
            message: 'Hello',
            progress: [1, 2],
        });
    });

});

describe('SurveyManager question format', () => {
    test('normalizes question objects', () => {
        let qid;

        const that = new SurveyManager('test', 'uri');
        that.init('123');

        that.add([{ id: 1 }]);
        const m1 = Q.model();
        m1.id = 1;

        expect(JSON.stringify(that.questions)).toBe(JSON.stringify([[m1]]));

        that.add([{ id: 2 }]);
        const m2 = Q.model();
        m2.id = 2;

        expect(JSON.stringify(that.questions)).toBe(JSON.stringify([[m1], [m2]]));
    });
});

describe('SurveyManager', () => {

    test('defaults', () => {
        const that = new SurveyManager('test', 'uri');

        expect(that.surveyID).toBe('test');
        expect(that.sessionID).toBe(null);
        expect(that.questions.length).toBe(0);

        // #332, 3406
        expect(that.status).toBe(0);
        expect(that.message).toBe('');
        expect(that.progress.toString()).toBe('-1,-1');

        // array and empty
        expect(JSON.stringify(that.questions)).toBe('[]');

        // #379
        expect(that.isFinished()).toBe(false);
    });

    xtest('requires surveyID param', () => {
        const that = new SurveyManager();
        // add?
    });

    xtest('requires endpoint param', () => {
        const that = new SurveyManager('test');
        // add?
    });

    test('init', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');
        expect(that.sessionID).toBe('123');
        // #332, #306
        expect(that.status).toBe(0);
        expect(that.message).toBe('');
        expect(that.progress.toString()).toBe('-1,-1');
    });

    // #379
    test('fisFinished', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');
        expect(that.isFinished()).toBe(false);
    });

    test('add', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');
        that.add([{ id: 1 }]);
        expect(that.questions.length).toBe(1);
        expect(that.questions[0].length).toBe(1);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });

        that.add([{ id: 2 }]);
        expect(that.questions.length).toBe(2);
        expect(that.questions[0].length).toBe(1);
        expect(that.questions[1].length).toBe(1);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });
        expect(that.questions[1][0]).toMatchObject({ id: 2});
    });

    // #332, #406
    test('add with status and message', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');

        // no status, messages, progress
        that.add([{ id: 1 }]);
        expect(that.questions.length).toBe(1);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });

        expect(that.status).toBe(0);
        expect(that.message).toBe('');
        expect(that.progress.toString()).toBe('-1,-1');

        // with status
        that.add([{ id: 2 }], 1);
        expect(that.questions.length).toBe(2);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });
        expect(that.questions[1][0]).toMatchObject({ id: 2});

        expect(that.status).toBe(1);
        expect(that.message).toBe('');
        expect(that.progress.toString()).toBe('-1,-1');

        // with status and message
        that.add([{ id: 3 }], 1, 'Hello');
        expect(that.questions.length).toBe(3);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });
        expect(that.questions[1][0]).toMatchObject({ id: 2});
        expect(that.questions[2][0]).toMatchObject({ id: 3});

        expect(that.status).toBe(1);
        expect(that.message).toBe('Hello');
        expect(that.progress.toString()).toBe('-1,-1');

        // with status, message and progress
        that.add([{ id: 4 }], 1, 'Hello', [1, 2]);
        expect(that.questions.length).toBe(4);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });
        expect(that.questions[1][0]).toMatchObject({ id: 2});
        expect(that.questions[2][0]).toMatchObject({ id: 3});
        expect(that.questions[3][0]).toMatchObject({ id: 4});

        expect(that.status).toBe(1);
        expect(that.message).toBe('Hello');
        expect(that.progress.toString()).toBe('1,2');
    });

    test('add has no effect if question ids match (double submission)', () => {
        let that;
        let did;

        // single question
        that = new SurveyManager('test', 'uri');
        that.init('123');

        did = that.add([{ id: 1 }]);
        expect(did).toBe(true);

        did = that.add([{ id: 1 }]);
        expect(did).toBe(false);

        expect(that.questions.length).toBe(1);
        expect(that.questions[0].length).toBe(1);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });

        // multi question
        that = new SurveyManager('test', 'uri');
        that.init('123');

        did = that.add([{ id: 1 }, { id: 2 }]);
        expect(did).toBe(true);

        did = that.add([{ id: 1 }, { id: 2 }]);
        expect(did).toBe(false);

        expect(that.questions.length).toBe(1);
        expect(that.questions[0].length).toBe(2);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });
        expect(that.questions[0][1]).toMatchObject({ id: 2 });

        // !!!multi question single ids are not checked!
        that = new SurveyManager('test', 'uri');
        that.init('123');

        did = that.add([{ id: 1 }, { id: 2 }]);
        expect(did).toBe(true);

        did = that.add([{ id: 1 }, { id: 3 }]);
        expect(did).toBe(true);

        expect(that.questions.length).toBe(2);
        expect(that.questions[0].length).toBe(2);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });
        expect(that.questions[0][1]).toMatchObject({ id: 2 });

        expect(that.questions[1].length).toBe(2);
        expect(that.questions[1][0]).toMatchObject({ id: 1 });
        expect(that.questions[1][1]).toMatchObject({ id: 3 });
    });

    xtest('add - merges double submissions of question ids', () => {
    });

    test('reset()', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');
        that.add([{ id: 1 }]);
        that.add([{ id: 2 }], 1, 'Hello');

        expect(that.questions.length).toBe(2);
        expect(that.questions[0].length).toBe(1);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });
        expect(that.questions[1].length).toBe(1);
        expect(that.questions[1][0]).toMatchObject({ id: 2 });

        that.reset();

        expect(that.questions.length).toBe(1);
        expect(that.questions[0].length).toBe(1);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });
    });

    // #332, #406
    test('reset() - reset status, message and progress to defaults', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');
        that.add([{ id: 1 }]);
        that.add([{ id: 2 }], 1, 'Hello', [1, 2]);

        expect(that.questions.length).toBe(2);;
        expect(that.questions[0][0]).toMatchObject({ id: 1 });
        expect(that.questions[1][0]).toMatchObject({ id: 2 });

        expect(that.status).toBe(1);
        expect(that.message).toBe('Hello');
        expect(that.progress.toString()).toBe('1,2');

        that.reset();

        expect(that.questions.length).toBe(1);
        expect(that.questions[0].length).toBe(1);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });

        expect(that.status).toBe(0);
        expect(that.message).toBe('');
        expect(that.progress.toString()).toBe('-1,-1');
    });

    test('current', () => {
        let curr;
        const that = new SurveyManager('test', 'uri');
        that.init('123');

        curr = that.current();
        expect(JSON.stringify(curr)).toBe('[]');

        that.add([{ id: 1 }]);
        curr = that.current();
        expect(curr.length).toBe(1);
        expect(curr[0]).toMatchObject({ id: 1 });

        that.add([{ id: 2 }]);
        curr = that.current();
        expect(curr.length).toBe(1);
        expect(curr[0]).toMatchObject({ id: 2 });

        that.reset();
        curr = that.current();
        expect(curr.length).toBe(1);
        expect(curr[0]).toMatchObject({ id: 1 });

        that.reset();
        curr = that.current();
        expect(JSON.stringify(curr)).toBe('[]');

        that.add([{ id: 3 }, { id: 4 }]);
        that.add([{ id: 5 }, { id: 6 }]);
        curr = that.current();
        expect(curr.length).toBe(2);
        expect(curr[0]).toMatchObject({ id: 5 });
        expect(curr[1]).toMatchObject({ id: 6 });
    });

});

describe('SurveyManager.canMerge', () => {

    test('bad: no curr surveyID', () => {
        const that = new SurveyManager();
        const res = that.canMerge({
            endpoint: 'uri',
            surveyID: 'another',
        })

        expect(res).toBe(false);
    });

    test('bad: no curr endpoint', () => {
        const that = new SurveyManager('test');
        const res = that.canMerge({
            endpoint: 'uri',
            surveyID: 'another',
        })

        expect(res).toBe(false);
    });

    test('bad: no cached surveyID', () => {
        const that = new SurveyManager('test');
        const res = that.canMerge({
            endpoint: 'uri',
            step: 1,
        })

        expect(res).toBe(false);
    });

    test('bad: no cached endpoint', () => {
        const that = new SurveyManager('test', 'uri');
        const res = that.canMerge({
            surveyID: 'test',
            step: 1,
        })

        expect(res).toBe(false);
    });

    test('bad: neither have surveyIDs', () => {
        const that = new SurveyManager();
        const res = that.canMerge({
            step: 1,
        })

        expect(res).toBe(false);
    });

    test('bad: neither have endpoints', () => {
        const that = new SurveyManager('test');
        const res = that.canMerge({
            surveyID: 'test',
        })

        expect(res).toBe(false);
    });

    test('bad: surveyIDs dont match', () => {
        const that = new SurveyManager('test');
        that.init('123');
        const res = that.canMerge({
            surveyID: 'another',
        })

        expect(res).toBe(false);
    });

    test('bad: endpoints dont match', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');
        const res = that.canMerge({
            surveyID: 'test',
            endpoint: 'another uri',
        })

        expect(res).toBe(false);
    });

    //

    test('good: no curr sessionID', () => {
        const that = new SurveyManager('test', 'uri');
        const res = that.canMerge({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: 'another',
        })

        expect(res).toBe(true);
    });

    test('bad: neither have sessionIDs', () => {
        const that = new SurveyManager('test', 'uri');
        const res = that.canMerge({
            step: 1,
        })

        expect(res).toBe(false);
    });

    test('bad: current sessionID', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');
        const res = that.canMerge({
            surveyID: 'test',
            sessionID: 'another',
        })

        expect(res).toBe(false);
    });

    test('bad: no cached sessionID', () => {
        const that = new SurveyManager('test', 'uri');
        const res = that.canMerge({
            surveyID: 'test',
        })

        expect(res).toBe(false);
    });

    // #332, #406
    test('different status, message and progress values have no impact', () => {
        const that = new SurveyManager('test', 'uri');
        // defaults
        expect(that.status).toBe(0);
        expect(that.message).toBe('');
        expect(that.progress.toString()).toBe('-1,-1');

        const res = that.canMerge({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: 'another',
            status: '1',
            message: 'hello',
            prongress: [1, 2],
        })

        expect(res).toBe(true);
    });

});

describe('SurveyManager.merge', () => {

    test('merge - replace, normalize questions', () => {
        const that = new SurveyManager('test', 'uri');
        that.merge({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',
            questions: [[{ id: 1 }]]
        })

        expect(that).toMatchObject({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',
        });

        const m1 = Q.model();
        m1.id = 1;

        expect(that.isFinished()).toBe(false);
        expect(JSON.stringify(that.questions)).toBe(JSON.stringify([[m1]]));
    });


    test('merge - retain unmatched defaults', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');
        that.merge({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123'
        });
        expect(that.isFinished()).toBe(false);
    });

    test('merge - dont include properties unkown to the instance', () => {
        const that = new SurveyManager('test', 'uri');
        that.merge({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',
            unknownProp: true,
        })

        expect(that).not.toContain('unknownProp');
    });

    test('merge - dont overwrite methods with non-function values', () => {
        const that = new SurveyManager('test', 'uri');
        that.merge({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',
            init: true,
        })

        expect(that.surveyID).toBe('test');
        expect(typeof(that.init)).toBe('function');
    });

    test('merge - overwrite methods with functions', () => {
        const that = new SurveyManager('test', 'uri');
        that.merge({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',
            init: () => 'overwritten',
        })
        const undef = that.init('123');

        expect(that.sessionID).toBe('123');
        expect(undef).toBe(undefined);
    });

    // #332, 406
    test('merge - status, message  and progress are merged in', () => {
        const that = new SurveyManager('test', 'uri');
        // defaults
        expect(that.status).toBe(0);
        expect(that.message).toBe('');
        expect(that.progress.toString()).toBe('-1,-1');

        that.merge({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',
            status: 1,
            message: 'Hello',
            progress: [1, 2],
        })
        that.init('123');

        expect(that.sessionID).toBe('123');
        expect(that.status).toBe(1);
        expect(that.message).toBe('Hello');
        expect(that.progress.toString()).toBe('1,2');
    });

});

describe('SurveyManager.replaceCurrent', () => {
    let res;

    // #379
    test('replaceCurrent - mark session as finished on receiving and empty question set', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');
        that.add([{ id: 1 }]);

        res = that.replaceCurrent([]);

        expect(that.questions.length).toBe(1);
        expect(that.isFinished()).toBe(true);
        expect(that.questions[0].length).toBe(0);

        expect(res).toBe(true);
    });

    test('replaceCurrent - replace (empty session)', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');

        res = that.replaceCurrent([{ id: 1 }]);

        expect(that.questions.length).toBe(1);
        expect(that.questions[0].length).toBe(1);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });

        expect(res).toBe(true);
    });

    test('replaceCurrent - replace (1)', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');
        that.add([{ id: 1 }]);

        res = that.replaceCurrent([{ id: 2 }]);

        expect(that.questions.length).toBe(1);
        expect(that.questions[0].length).toBe(1);
        expect(that.questions[0][0]).toMatchObject({ id: 2 });

        expect(res).toBe(true);
    });

    test('replaceCurrent - replace (2)', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');
        that.add([{ id: 1 }]);
        that.add([{ id: 2 }]);

        res = that.replaceCurrent([{ id: 3 }]);

        expect(that.questions.length).toBe(2);
        expect(that.questions[0].length).toBe(1);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });
        expect(that.questions[1].length).toBe(1);
        expect(that.questions[1][0]).toMatchObject({ id: 3 });

        expect(res).toBe(true);
    });

    // #332, #406
    test('replaceCurrent - status', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');

        res = that.replaceCurrent([{ id: 1 }], 1);

        expect(that.questions.length).toBe(1);
        expect(that.questions[0].length).toBe(1);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });

        expect(that.status).toBe(1);
        expect(that.message).toBe('');
        expect(that.progress.toString()).toBe('-1,-1');

        expect(res).toBe(true);
    });

    test('replaceCurrent - status and message', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');

        res = that.replaceCurrent([{ id: 1 }], 1, 'Hello');

        expect(that.questions.length).toBe(1);
        expect(that.questions[0].length).toBe(1);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });

        expect(that.status).toBe(1);
        expect(that.message).toBe('Hello');
        expect(that.progress.toString()).toBe('-1,-1');

        expect(res).toBe(true);
    });

    test('replaceCurrent - status, message and progress', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');

        res = that.replaceCurrent([{ id: 1 }], 1, 'Hello', [1, 2]);

        expect(that.questions.length).toBe(1);
        expect(that.questions[0].length).toBe(1);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });

        expect(that.status).toBe(1);
        expect(that.message).toBe('Hello');
        expect(that.progress.toString()).toBe('1,2');

        expect(res).toBe(true);
    });

});
