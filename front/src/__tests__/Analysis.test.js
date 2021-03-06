import { normalizeEvaluation, normalizeAnalysis } from '../Analysis';

const evaluationReference = function() {
     return {
        category: '',
        classification: '',
        displayResults: {
            additionalInsights: [],
            conditions: {
                condition: '',
                subcondition: '',
                mainText: '',
                learnMore: '',
                mainRecommendation: '',
                mandatoryTips: '',
                additionalInsights: [],
            }
        },
        rank: 0,
        recommendation: '',
        riskRating: 0
    };
};

const analysisReference = function() {
     return {
        version: '',
        created: '',
        constraints: [],
        evaluations: [],
    };
};

describe('normalizeEvaluation', () => {

    test('deal with incomplete data', () => {
        let e;
        e = normalizeEvaluation(null);
        expect(e).toEqual(evaluationReference());

        e = normalizeEvaluation({ 'category': 'test' });
        expect(e.category).toBe('test');
        e.category = '';
        expect(e).toEqual(evaluationReference());
    });

    test('displayResults', () => {
        let e;
        e = normalizeEvaluation({
            category: '',
            classification: '',
            /* displayResults: {} */
            rank: 0,
            recommendation: '',
            riskRating: 0
        });
        expect(e).toEqual(evaluationReference());

        e = normalizeEvaluation({
            category: '',
            classification: '',
            displayResults: {},
            rank: 0,
            recommendation: '',
            riskRating: 0
        });
        expect(e).toEqual(evaluationReference());
    });

    test('displayResults.conditions', () => {
        let e;
        e = normalizeEvaluation({
            category: '',
            classification: '',
            displayResults: {
                conditions: {},
            },
            rank: 0,
            recommendation: '',
            riskRating: 0
        });
        expect(e).toEqual(evaluationReference());
    });

    test('displayResults.conditions.condition', () => {
        let e;
        e = normalizeEvaluation({
            category: '',
            classification: '',
            displayResults: {
                conditions: {
                    condition: 'test',
                }
            },
            rank: 0,
            recommendation: '',
            riskRating: 0
        });
        expect(e.displayResults.conditions.condition).toBe('test');
        e.displayResults.conditions.condition = '';
        expect(e).toEqual(evaluationReference());
    });

    test('displayResults.conditions.additionalInsights', () => {
        let e;
        e = normalizeEvaluation({
            category: '',
            classification: '',
            displayResults: {
                additionalInsights: [{
                    displayName: 'test-key',
                    /* displayText: 'test-value', */
                }]
            },
            rank: 0,
            recommendation: '',
            riskRating: 0
        });

        expect(e.displayResults.additionalInsights.length).toBe(1);
        expect(e.displayResults.additionalInsights[0]).toEqual({
            displayName: 'test-key',
            displayText: '',
        });
        e.displayResults.additionalInsights = [];
        expect(e).toEqual(evaluationReference());
     });

    test('displayResults.conditions.additionalInsights (incomplete)', () => {
        let e;
        e = normalizeEvaluation({
            category: '',
            classification: '',
            displayResults: {
                additionalInsights: [{
                    displayName: 'test-key',
                    displayText: 'test-value',
                }]
            },
            rank: 0,
            recommendation: '',
            riskRating: 0
        });

        expect(e.displayResults.additionalInsights.length).toBe(1);
        expect(e.displayResults.additionalInsights[0]).toEqual({
            displayName: 'test-key',
            displayText: 'test-value',
        });
        e.displayResults.additionalInsights = [];
        expect(e).toEqual(evaluationReference());
    });
});

describe('normalizeAnalysis', () => {
   test('normalizeAnalysis LEGACY', () => {
        const e = evaluationReference();
        const legacy  = [e, e];

        let a;
        a = normalizeAnalysis([]);
        expect(a).toEqual(analysisReference());

        a = normalizeAnalysis(legacy);
        const reference = analysisReference();
        reference.evaluations = [evaluationReference(), evaluationReference()]
        expect(a).toEqual(reference);
   });

   // #352 lts
    test('normalizeEAnalysis insight empty', () => {
        const ref = evaluationReference();
        const e = normalizeEvaluation(ref);

        expect(e.displayResults.conditions.additionalInsights.length).toBe(0);
    });

    test('normalizeEAnalysis insight', () => {
        const ref = evaluationReference();
        ref.displayResults.conditions.additionalInsights = [{
            displayName: 'testName',
            displayText: 'testText'
        }];

        const e = normalizeEvaluation(ref);

        expect(e.displayResults.conditions.additionalInsights.length).toBe(1);
        expect(e.displayResults.conditions.additionalInsights[0]).toEqual({
            displayName: 'testName',
            displayText: 'testText',
        });
    });

   // #352 legacy additional insights
    test('normalizeEAnalysis insight LEGACY (string)', () => {
        const ref = evaluationReference();
        ref.displayResults.conditions.additionalInsights = 'test';

        const e = normalizeEvaluation(ref);

        expect(e.displayResults.conditions.additionalInsights.length).toBe(1);
        expect(e.displayResults.conditions.additionalInsights[0]).toEqual({
            displayName: '',
            displayText: 'test',
        });
    });

    test('normalizeEAnalysis insight LEGACY (string[])', () => {
        const ref = evaluationReference();
        ref.displayResults.conditions.additionalInsights = ['test'];

        const e = normalizeEvaluation(ref);

        expect(e.displayResults.conditions.additionalInsights.length).toBe(1);
        expect(e.displayResults.conditions.additionalInsights[0]).toEqual({
            displayName: '',
            displayText: 'test',
        });

    });

    test('normalizeEAnalysis insight LEGACY (mixed[])', () => {
        const ref = evaluationReference();
        ref.displayResults.conditions.additionalInsights = [
            'test',
            {
                displayName: 'testName',
                displayText: 'testText'
            }
        ];

        const e = normalizeEvaluation(ref);

        expect(e.displayResults.conditions.additionalInsights.length).toBe(2);
        expect(e.displayResults.conditions.additionalInsights[0]).toEqual({
            displayName: '',
            displayText: 'test',
        });
        expect(e.displayResults.conditions.additionalInsights[1]).toEqual({
            displayName: 'testName',
            displayText: 'testText',
        });

    });
});
