
const normalizeAnalysis = function(response) {
    const r = response || {};

    return Object.assign({
        category: 'No category',
        classification: 'No classification',
        displayResults: {
            additionalInsights: [],
            sleepConditions: {
                condition: 'No condition',
                subcondition: '',
                mainText: '',
                learnMore: '',
                mainRecommendation: '',
                mandatoryTips: '',
                additionalInsights: ''
            }
        },
        rank: 0,
        recommendation: '',
        riskRating: 0
    }, r);
}

export { normalizeAnalysis };
