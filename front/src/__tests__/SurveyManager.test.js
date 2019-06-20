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
    });

});

describe('SurveyManager.merge', () => {

    test('merge - replace', () => {
        const that = new SurveyManager('test', 'uri');
        that.merge({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',
            closed: true,
            questions: [[{ id: 1 }]]
        })

        expect(that).toMatchObject({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',
            closed: true,
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
        expect(that.closed).toBe(false);
        // array and empty
        expect(JSON.stringify(that.questions)).toBe('[]');
    });

    xtest('requires surveyID param', () => {
        const that = new SurveyManager();
        // add?
    });

    xtest('requires endpoint param', () => {
        const that = new SurveyManager('test');
        // add?
    });

    test('defaults', () => {
        const that = new SurveyManager('test', 'uri');
        expect(that.surveyID).toBe('test');
        expect(that.endpoint).toBe('uri');
        expect(that.sessionID).toBe(null);
        expect(that.questions.length).toBe(0);
        expect(that.closed).toBe(false);
        // array and empty
        expect(JSON.stringify(that.questions)).toBe('[]');
    });

    test('init', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');
        expect(that.sessionID).toBe('123');
    });

    test('close', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');
        that.close();
        expect(that.closed).toBe(true);
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

    test('add has no effect on closed suvey', () => {
        let did;
        const that = new SurveyManager('test', 'uri');
        that.init('123');

        // before close
        did = that.add([{ id: 1 }]);
        expect(did).toBe(true);
        expect(that.questions.length).toBe(1);
        expect(that.questions[0].length).toBe(1);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });

        that.close();

        // after close
        did = that.add([{ id: 2 }]);
        expect(did).toBe(false);
        expect(that.questions.length).toBe(1);
        expect(that.questions[0].length).toBe(1);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });
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
        that.add([{ id: 2 }]);

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

    test('reset() has no effect on closed suvey', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');
        that.add([{ id: 1 }]);
        that.add([{ id: 2 }]);

        expect(that.questions.length).toBe(2);
        expect(that.questions[0].length).toBe(1);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });
        expect(that.questions[1].length).toBe(1);
        expect(that.questions[1][0]).toMatchObject({ id: 2 });

        that.close();
        that.reset();

        expect(that.questions.length).toBe(2);
        expect(that.questions[0].length).toBe(1);
        expect(that.questions[0][0]).toMatchObject({ id: 1 });
        expect(that.questions[1].length).toBe(1);
        expect(that.questions[1][0]).toMatchObject({ id: 2 });
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

    test('currentInversed()', () => {
        let inversed;

        const that = new SurveyManager('test', 'uri');
        that.init('123');
        that.add([{ id: 'something' }]);
        that.add([{ id: 1 }, { id: 2 }, { id: 3 }]);

        inversed = that.currentInversed();
        expect(inversed === that.current()).toBe(false);
        expect(inversed.length).toBe(3);
        expect(inversed[0]).toMatchObject({ id: 3 });
        expect(inversed[1]).toMatchObject({ id: 2 });
        expect(inversed[2]).toMatchObject({ id: 1 });

        // add another set

        that.add([{ id: 3 }, { id: 4 }, { id: 5 }]);
        inversed = that.currentInversed();
        expect(inversed === that.current()).toBe(false);
        expect(inversed.length).toBe(3);
        expect(inversed[0]).toMatchObject({ id: 5 });
        expect(inversed[1]).toMatchObject({ id: 4 });
        expect(inversed[2]).toMatchObject({ id: 3 });

        // reset

        that.reset();
        inversed = that.currentInversed();
        expect(inversed === that.current()).toBe(false);
        expect(inversed.length).toBe(3);
        expect(inversed[0]).toMatchObject({ id: 3 });
        expect(inversed[1]).toMatchObject({ id: 2 });
        expect(inversed[2]).toMatchObject({ id: 1 });
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

});

describe('SurveyManager.merge', () => {

    test('merge - replace, normalize questions', () => {
        const that = new SurveyManager('test', 'uri');
        that.merge({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',
            closed: true,
            questions: [[{ id: 1 }]]
        })

        expect(that).toMatchObject({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',
            closed: true,
        });

        const m1 = Q.model();
        m1.id = 1;

        expect(that.closed).toBe(true);
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
        expect(that.closed).toBe(false);
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

});
