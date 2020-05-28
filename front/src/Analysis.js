/**
 * @module answer object to backend csv row parser
 *
 */
import PropTypes from 'prop-types';

import { isArray } from './Utils';

/**
 * @typedef Constraint
 * @type {object}
 * @property {string} key
 * @property {string} message
 */

/**
 * @typedef Insight
 * @type {object}
 * @property {string} displayName
 * @property {string} displayText
 */

/**
 * @typedef Condition
 * @type {object}
 * @property {string} condition
 * @property {string} subcondition
 * @property {string} mainText
 * @property {string} learnMore
 * @property {string} mainRecommendation
 * @property {string} mandatoryTips
 * @property {string|Insight[]} additionalInsights, includes legacy string format (#351)
 */

/**
 * @typedef EvaluationDisplayResult
 * @type {object}
 * @property {Insight[]} additionalInsights
 * @property {Condition[]} conditions
 */

/**
 * @typedef Evaluation
 * @type {object}
 * @property {string} category
 * @property {string} classification
 * @property {number} rank
 * @property {string} recommendation
 * @property {number} riskRating
 * @property {EvaluationDisplayResult[]} displayResults
 */

/**
 * @typedef Analysis
 * @type {object}
 * @property {string} version
 * @property {string} created
 * @property {Constraint[]} constraints
 * @property {Evaluation[]} evaluations
 */

const Models = {

    /**
     * @returns {Evaluation} evaluation
     */
    constraint: function() {
        return {
            key: '',
            message: '',
        };
    },

    /**
     * @returns {Insight} additionalInsights
     */
    insight: function() {
        return {
            displayName: '',
            displayText: '',
        };
    },

    /**
     * @returns {Condition} condition
     */
    conditions: function() {
        return {
            condition: '',
            subcondition: '',
            mainText: '',
            learnMore: '',
            mainRecommendation: '',
            mandatoryTips: '',
            additionalInsights: [], // #351
        };
    },

    displayResults: function() {
        return {
            additionalInsights: [],
            conditions: Models.conditions(),
        };
    },

    /**
     * @returns {Evaluation} evaluation
     */
    evaluation: function() {
        return {
            category: '',
            classification: '',
            displayResults: Models.displayResults(),
            rank: 0,
            recommendation: '',
            riskRating: 0
        };
    },

    /**
     * @returns {Evaluation} evaluation
     */
    analysis: function() {
        return {
            version: '',
            created: '',
            constraints: [],
            evaluations: [],
        };
    },

};

/**
* Get Proptypes schema
* @returns {PropTypes}
*
* #350, add meta data wrapper
*/
const insightPropTypes = function () {
    return PropTypes.shape({
        displayName: PropTypes.string,
        displayText: PropTypes.string,
    });
};

/**
* Get Proptypes schema
* @returns {PropTypes}
*/
const evaluationPropTypes = function () {
    return PropTypes.shape({
        category: PropTypes.string,
        classification: PropTypes.string,
        rank: PropTypes.number,
        recommendation: PropTypes.string,
        riskRating: PropTypes.number,
        displayResults: PropTypes.shape({
            additionalInsights: PropTypes.arrayOf(
                insightPropTypes()
            ),
            conditions: PropTypes.shape({
                condition: PropTypes.string,
                subcondition: PropTypes.string,
                mainText: PropTypes.string,
                learnMore: PropTypes.string,
                mainRecommendation: PropTypes.string,
                mandatoryTips: PropTypes.string,
                // #352, add complex model, allow simple strings
                additionalInsights: PropTypes.oneOf([
                    PropTypes.string,
                    PropTypes.arrayOf(insightPropTypes()),
                ])
            })
        }),
    });
};

/**
* Get Proptypes schema
* @returns {PropTypes}
*
* #350, add meta data wrapper
*/
const analysisPropTypes = function () {
    return PropTypes.shape({
        created: PropTypes.string.isRequired,
        version: PropTypes.string,
        constraints: PropTypes.arrayOf(PropTypes.shape({
            key: PropTypes.string,
            message: PropTypes.string,
        })),
        evaluations: PropTypes.arrayOf(evaluationPropTypes())
    });
};

const mockAnalysis = function() {
    const evaluations =  [{
        category: '',
        classification: 'Mild',
        displayResults: {
            additionalInsights: [{
                displayName: 'Avoid Vogons breath',
                displayText: 'Avoid vogons, I that cannot be avoided, turn away in presence of a Vogon.'
            }],
            conditions: {
                condition: 'Vogonic Flue',
                subcondition: 'Confusion',
                mainText: 'It is known that there are an infinite number of worlds, simply because there is an infinite amount of space for them to be in. \n However, not every one of them is inhabited. Therefore, there must be a finite number of inhabited worlds. \n Any finite number divided by infinity is as near to nothing as makes no odds, so the average population of all the planets in the Universe can be said to be zero. \n From this it follows that the population of the whole Universe is also zero, and that any people you may meet from time to time are merely the products of a deranged imagination.',
                learnMore: 'Make it totally clear that this gun has a right end and a wrong end. Make it totally clear to anyone standing at the wrong end that things are going badly for them.',
                mainRecommendation: 'Make it totally clear that this gun has a right end and a wrong end. Make it totally clear to anyone standing at the wrong end that things are going badly for them.',
                mandatoryTips: 'Eskimos had over two hundred different words for snow, without which their conversation would probably have got very monotonous.',
                additionalInsights: [
                    // #352, legacy format
                    'A common mistake that people make when trying to design something completely foolproof is to underestimate the ingenuity of complete fools.',
                    // #352, insight format
                    {
                        displayName: 'Don\'t panic',
                        displayText: 'There is a theory which states that if ever anyone discovers exactly what the Universe is for and why it is here, it will instantly disappear and be replaced by something even more bizarre and inexplicable. There is another theory which states that this has already happened.'
                    }
                ]
            }
        },
        rank: 2,
        recommendation: '',
        riskRating: 0
    }, {
        category: '',
        classification: 'Severe',
        displayResults: {
            additionalInsights: [],
            conditions: {
                condition: 'Disorientation',
                subcondition: 'You are lost in space',
                mainText: 'It is known that there are an infinite number of worlds, simply because there is an infinite amount of space for them to be in. \n However, not every one of them is inhabited. Therefore, there must be a finite number of inhabited worlds. \n Any finite number divided by infinity is as near to nothing as makes no odds, so the average population of all the planets in the Universe can be said to be zero. \n From this it follows that the population of the whole Universe is also zero, and that any people you may meet from time to time are merely the products of a deranged imagination.',
                learnMore: 'Make it totally clear that this gun has a right end and a wrong end. Make it totally clear to anyone standing at the wrong end that things are going badly for them.',
                mainRecommendation: 'Make it totally clear that this gun has a right end and a wrong end. Make it totally clear to anyone standing at the wrong end that things are going badly for them.',
                mandatoryTips: 'Eskimos had over two hundred different words for snow, without which their conversation would probably have got very monotonous.',
                // #352, legacy format
                additionalInsights: 'The Total Perspective Vortex derives its picture of the whole Universe on the principle of extrapolated matter analyses.To explain — since every piece of matter in the Universe is in some way affected by every other piece of matter in the Universe, it is in theory possible to extrapolate the whole of creation — every sun, every planet, their orbits, their composition and their economic and social history from, say, one small piece of fairy cake. The man who invented the Total Perspective Vortex did so basically in order to annoy his wife.'
            }
        },
        rank: 2,
        recommendation: '',
        riskRating: 0
    }];

    return {
        created: new Date().toISOString(),
        version: '9.9.9',
        constraints: [{
            key: 'Warning',
            message: 'Don\'t Panic',
        }],
        evaluations,
    }
};
// console.log(JSON.stringify(mockAnalysis(), null, 4));


/**
 * Sets default values and normalizes an /analyse api response object
 * @param {object} response
 * @returns {object}
 *
 * #352, legacy support allow 'string', 'string[]'
 */
const normalizeInsight = function(insight) {
    let r = insight || {}
    if (typeof r === 'string') {
        r  = {
            displayText: r
        };
    }
    return Object.assign(Models.insight(), r);
};

/**
 * Sets default values and normalizes an /analyse api response object
 * @param {object} response
 * @returns {object}
 */
const normalizeEvaluation = function(response) {
    let r = response || {};
    const d = r.displayResults || {};

    r = Object.assign(Models.evaluation(), r);
    r.displayResults = Object.assign(Models.displayResults(), d);

    const a = r.displayResults.additionalInsights;
    const c = r.displayResults.conditions;

    // #352
    let ci = c.additionalInsights || [];
    ci = (typeof ci == 'string') ? [ci] : ci;
    c.additionalInsights = ci.map(item => normalizeInsight(item))

    r.displayResults.additionalInsights = a.map(item => normalizeInsight(item));
    r.displayResults.conditions = Object.assign(Models.conditions(), c);
    return r;
}

/**
 * Sets default values and normalizes an /analyse api response object
 * @param {object} response
 * @returns {object}
 *
 * #350, add meta data wrapper, deal with legacy format
 */
const normalizeAnalysis = function(response) {
    let r = response || {};

    let c = [];
    let e = [];

    // #350 transform legacy analysis: array of evaluations
    if (isArray(response)) {
        r = {};
        e = response;
    } else {
        c = response.constraints || [];
        e = response.evaluations || [];
    }

    r = Object.assign(Models.analysis(), r);
    r.constraints =  c.map(item => Object.assign(Models.constraint(), item));
    r.evaluations =  e.map(item => normalizeEvaluation(item));
    return r;
}

export {
    Models,
    mockAnalysis,
    normalizeInsight,
    normalizeEvaluation,
    normalizeAnalysis,
    insightPropTypes,
    evaluationPropTypes,
    analysisPropTypes
};
