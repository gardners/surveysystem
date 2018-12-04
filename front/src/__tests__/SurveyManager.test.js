import SurveyManager from '../SurveyManager';

describe('SurveyManager', () => {

    test('defaults', () => {
        const that = new SurveyManager('test', 'uri');

        expect(that.surveyID).toBe('test');
        expect(that.sessionID).toBe(null);
        expect(that.step).toBe(-1);
        expect(that.finished).toBe(false);
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
        expect(that.step).toBe(-1);
        expect(that.finished).toBe(false);
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

        expect(that.finished).toBe(true);
        expect(that.closed).toBe(true);
    });

    test('add', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');
        that.add([{ id: 1 }]);
        expect(JSON.stringify(that.questions)).toBe('[[{"id":1}]]');

        that.add([{ id: 2 }]);
        expect(JSON.stringify(that.questions)).toBe('[[{"id":1}],[{"id":2}]]');
    });

    test('add has no effect on closed suvey', () => {
        let did;
        const that = new SurveyManager('test', 'uri');
        that.init('123');

        // before close
        did = that.add([{ id: 1 }]);
        expect(did).toBe(true);
        expect(JSON.stringify(that.questions)).toBe('[[{"id":1}]]');

        that.close();

        // after close
        did = that.add([{ id: 2 }]);
        expect(did).toBe(false);
        expect(JSON.stringify(that.questions)).toBe('[[{"id":1}]]');
    });

    xtest('add - merges double submissions of question ids', () => {
    });

    test('back', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');
        that.add([{ id: 1 }]);
        that.add([{ id: 2 }]);
        that.back();
        expect(JSON.stringify(that.questions)).toBe('[[{"id":1}]]');
    });

    test('back  has no effect on closed suvey', () => {
        let did;
        const that = new SurveyManager('test', 'uri');
        that.init('123');
        that.add([{ id: 1 }]);
        that.add([{ id: 2 }]);

        // before close
        did = that.back();
        expect(did).toBe(true);
        expect(JSON.stringify(that.questions)).toBe('[[{"id":1}]]');

        that.close();

        // after close
        did = that.back();
        did = expect(did).toBe(false);
        expect(JSON.stringify(that.questions)).toBe('[[{"id":1}]]');
    });

    test('current', () => {
        let curr;
        const that = new SurveyManager('test', 'uri');
        that.init('123');

        curr = that.current();
        expect(JSON.stringify(curr)).toBe('[]');

        that.add([{ id: 1 }]);
        curr = that.current();
        expect(JSON.stringify(curr)).toBe('[{"id":1}]');

        that.add([{ id: 2 }]);
        curr = that.current();
        expect(JSON.stringify(curr)).toBe('[{"id":2}]');

        that.back();
        curr = that.current();
        expect(JSON.stringify(curr)).toBe('[{"id":1}]');

        that.back();
        curr = that.current();
        expect(JSON.stringify(curr)).toBe('[]');

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

    test('merge - retain unmatched defaults', () => {
        const that = new SurveyManager('test', 'uri');
        that.init('123');
        that.merge({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123'
        })

        expect(that.step).toBe(-1);
        expect(that.finished).toBe(false);
    });

    test('merge - replace', () => {
        const that = new SurveyManager('test', 'uri');
        that.merge({
            surveyID: 'test',
            endpoint: 'uri',
            sessionID: '123',
            step: 9,
            finished: true,
            questions: [[{ id: 1 }]]
        })

        expect(that.step).toBe(9);
        expect(that.finished).toBe(true);
        expect(JSON.stringify(that.questions)).toBe('[[{"id":1}]]');
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