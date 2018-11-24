import SurveyManager from '../SurveyManager';

describe('SurveyManager', () => {

    test('defaults', () => {
        const that = new SurveyManager('test');

        expect(that.surveyID).toBe('test');
        expect(that.sessionID).toBe(null);
        expect(that.step).toBe(-1);
        expect(that.finished).toBe(false);
        // array and empty
        expect(JSON.stringify(that.questions)).toBe('[]');
    });

    xtest('requires surveyID param', () => {
        const that = new SurveyManager();
        // add?
    });

    test('defaults', () => {
        const that = new SurveyManager('test');

        expect(that.surveyID).toBe('test');
        expect(that.sessionID).toBe(null);
        expect(that.step).toBe(-1);
        expect(that.finished).toBe(false);
        // array and empty
        expect(JSON.stringify(that.questions)).toBe('[]');
    });

    test('init', () => {
        const that = new SurveyManager('test');
        that.init('123');
        expect(that.sessionID).toBe('123');
    });

    test('add', () => {
        const that = new SurveyManager('test');
        that.init('123');
        that.add([{ id: 1 }]);
        expect(JSON.stringify(that.questions)).toBe('[[{"id":1}]]');

        that.add([{ id: 2 }]);
        expect(JSON.stringify(that.questions)).toBe('[[{"id":1}],[{"id":2}]]');
    });

    xtest('add - merges double submissions of question ids', () => {
    });

    test('back', () => {
        const that = new SurveyManager('test');
        that.init('123');
        that.add([{ id: 1 }]);
        that.add([{ id: 2 }]);
        that.back();
        expect(JSON.stringify(that.questions)).toBe('[[{"id":1}]]');

    });

    test('current', () => {
        let curr;
        const that = new SurveyManager('test');
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

    test('merge - retain surveyID does\'t match', () => {
        const that = new SurveyManager('test');
        that.merge({
            surveyID: 'another',
        })

        expect(that.surveyID).toBe('test');
        expect(that.sessionID).toBe(null);
        expect(that.step).toBe(-1);
        expect(that.finished).toBe(false);
    });

    test('merge - no surveyID prop in cache', () => {
        const that = new SurveyManager('test');
        that.merge({
            sessionID: '123',
        })

        expect(that.surveyID).toBe('test');
        expect(that.sessionID).toBe(null);
        expect(that.step).toBe(-1);
        expect(that.finished).toBe(false);
    });

    test('merge - sessionID & use defaults if incomplete', () => {
        const that = new SurveyManager('test');
        that.init('123');
        that.merge({
            surveyID: 'test',
            sessionID: '321',
        })

        expect(that.surveyID).toBe('test');
        expect(that.sessionID).toBe('321');
        expect(that.step).toBe(-1);
        expect(that.finished).toBe(false);
    });

    test('merge - properties unkown to the instance', () => {
        const that = new SurveyManager('test');
        that.init('123');
        that.merge({
            surveyID: 'test',
            unknownProp: true,
        })

        expect(that.surveyID).toBe('test');
        expect(that).not.toContain('unknownProp');
        expect(that.sessionID).toBe('123');
    });

    test('merge - overwrite methods with values', () => {
        const that = new SurveyManager('test');
        that.merge({
            surveyID: 'test',
            init: true,
        })

        expect(that.surveyID).toBe('test');
        expect(typeof(that.init)).toBe('function');
    });

    test('merge - overwrite methods with functions', () => {
        const that = new SurveyManager('test');
        that.merge({
            surveyID: 'test',
            init: () => 'overwritten',
        })
        const undef = that.init('123');

        expect(that.sessionID).toBe('123');
        expect(undef).toBe(undefined);
    });

});
