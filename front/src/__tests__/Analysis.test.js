import { normalizeInsight, normalizeCondition, normalizeAnalysis } from '../Analysis';

const conditionReference = function() {
    return {
        'Code': '',
        'Classification': '',
        'Category': '',
        'Condition': '',
        'Learn More': '',
        'Tips': '',
        'Recommendation': '',
        'Description': '',
        'Insights': [],
    };
};

const analysisReference = function() {
     return {
        version: '',
        created: '',
        survey_id: '',
        session_id: '',
        conditions: [],
    };
};

describe('normalizeInsight', () => {
    test('Insight, empty', () => {
        let e;
        e = normalizeInsight([]);

        expect(Array.isArray(e)).toBe(true);
        expect(e.length).toBe(2);

        expect(e[0]).toBe('');
        expect(e[1]).toBe('');
    });

    test('Insight, populated', () => {
        let e;
        e = normalizeInsight(['TEST_KEY', 'TEST_CONTENT']);

        expect(Array.isArray(e)).toBe(true);
        expect(e.length).toBe(2);

        expect(e[0]).toBe('TEST_KEY');
        expect(e[1]).toBe('TEST_CONTENT');
    });

    test('Insight, incomplete', () => {
        let e;
        e = normalizeInsight(['TEST_KEY']);

        expect(Array.isArray(e)).toBe(true);
        expect(e.length).toBe(2);

        expect(e[0]).toBe('TEST_KEY');
        expect(e[1]).toBe('');
    });
});

describe('normalizeCondition', () => {

    test('Condtion', () => {
        let e;
        e = normalizeCondition({});
        expect(e).toEqual(conditionReference());

        e = normalizeCondition(conditionReference());
        expect(e).toEqual(conditionReference());
    });

    test('Condition.Code', () => {
        let e;
        e = normalizeCondition({ 'Code': 'test' });
        expect(e['Code']).toBe('test');
    });

    test('Condition.Classification', () => {
        let e;
        e = normalizeCondition({ 'Classification': 'test' });
        expect(e['Classification']).toBe('test');
    });

    test('Condition.Category', () => {
        let e;
        e = normalizeCondition({ 'Category': 'test' });
        expect(e['Category']).toBe('test');
    });

    test('Condition.Condition', () => {
        let e;
        e = normalizeCondition({ 'Condition': 'test' });
        expect(e['Condition']).toBe('test');
    });

    test('Condition.Learn More', () => {
        let e;
        e = normalizeCondition({ 'Learn More': 'test' });
        expect(e['Learn More']).toBe('test');
    });

    test('Condition.Tips', () => {
        let e;
        e = normalizeCondition({ 'Tips': 'test' });
        expect(e['Tips']).toBe('test');
    });

    test('Condition.Recommendation', () => {
        let e;
        e = normalizeCondition({ 'Recommendation': 'test' });
        expect(e['Recommendation']).toBe('test');
    });

    test('Condition.Description', () => {
        let e;
        e = normalizeCondition({ 'Description': 'test' });
        expect(e['Description']).toBe('test');
    });

    test('Condition.Insights, empty', () => {
        let e;
        e = normalizeCondition({ 'Insights': [] });
        expect(Array.isArray(e['Insights'])).toBe(true);
        expect(e['Insights'].length).toBe(0);
    });

    test('Condition.Insights, populated', () => {
        let e;
        e = normalizeCondition({
            'Insights': [
                ['TEST_KEY', 'TEST_CONTENT']
            ]
        });
        expect(Array.isArray(e['Insights'])).toBe(true);
        expect(e['Insights'].length).toBe(1);

        expect(Array.isArray(e['Insights'][0])).toBe(true);
        expect(e['Insights'][0].length).toBe(2);

        expect(e['Insights'][0][0]).toBe('TEST_KEY');
        expect(e['Insights'][0][1]).toBe('TEST_CONTENT');
    });

});


describe('normalizeAnalysis', () => {

    test('Analysis', () => {
        let e;
        e = normalizeAnalysis({});
        expect(e).toEqual(analysisReference());

        e = normalizeAnalysis(analysisReference());
        expect(e).toEqual(analysisReference());
    });

    test('Analysis.version', () => {
        let e;
        e = normalizeAnalysis({ version: 'TEST' });
        expect(e.version).toBe('TEST');
    });

    test('Analysis.created', () => {
        let e;
        e = normalizeAnalysis({ created: 'TEST' });
        expect(e.created).toBe('TEST');
    });

    test('Analysis.survey_id', () => {
        let e;
        e = normalizeAnalysis({ survey_id: 'TEST' });
        expect(e.survey_id).toBe('TEST');
    });

    test('Analysis.session_id', () => {
        let e;
        e = normalizeAnalysis({ session_id: 'TEST' });
        expect(e.session_id).toBe('TEST');
    });

    test('Analysis.conditions, empty', () => {
        let e;
        e = normalizeAnalysis({ conditions: [] });
        expect(e.conditions).toEqual([]);
    });

    test('Analysis.conditions, populated', () => {
        let e;
        e = normalizeAnalysis({ conditions: [ conditionReference() ] });
        expect(e.conditions).toEqual([ conditionReference() ]);
    });
});
