/**
 * @module answer object to backend csv row parser
 *
 */
import PropTypes from 'prop-types';


/**
 * @typedef Insight
 * @type {array}
 * @property {string} name
 * @property {string} txt
 */

/**
 * @typedef Condition
 * @type {object}
 * @property {string} Code
 * @property {string} Classification
 * @property {string} Category
 * @property {string} Condition
 * @property {string} Learn More
 * @property {string} Tips
 * @property {string} Recommendation
 * @property {string} Description
 * @property {Insight[]} Insights
 */

/**
 * @typedef Analysis
 * @type {object}
 * @property {string} version
 * @property {string} created
 * @property {string} survey_id
 * @property {string} session_id
 * @property {Condition[]} conditions
 */

const Models = {

    /**
     * @returns {Insight} insights
     */
    insight: function() {
        return ['', ''];
    },

    /**
     * @returns {Condition} condition
     */
    condition: function() {
        return {
            'Code': '',
            'Classification': '',
            'Category':'',
            'Condition': '',
            'Learn More': '',
            'Tips': '',
            'Recommendation': '',
            'Description': '',
            'Insights': [],
        };
    },

    /**
     * @returns {Analysis} analysis
     */
    analysis: function() {
        return {
            version: '',
            created: '',
            survey_id: '',
            session_id: '',
            conditions: [],
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
    return PropTypes.array;
};

/**
* Get Proptypes schema
* @returns {PropTypes}
*/
const conditionPropTypes = function () {
    return PropTypes.shape({
        'Code': PropTypes.string.isRequired,
        'Classification': PropTypes.string.isRequired,
        'Category': PropTypes.string.isRequired,
        'Condition': PropTypes.string.isRequired,
        'Learn More': PropTypes.string.isRequired,
        'Tips': PropTypes.string.isRequired,
        'Recommendation': PropTypes.string.isRequired,
        'Description': PropTypes.string.isRequired,
        'Insights': PropTypes.arrayOf(insightPropTypes())
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
        version: PropTypes.string.isRequired,

        survey_id: PropTypes.string.isRequired,
        session_id: PropTypes.string.isRequired,

        conditions: PropTypes.arrayOf(conditionPropTypes())
    });
};

const mockAnalysis = function() {
    const conditions =  [{
        'Code': 'VogonicFlue',
        'Classification': 'Mild',
        'Category': 'Vogon Nuisances',
        'Condition': 'Vogonic Flue',
        'Learn More': 'Make it totally clear that this gun has a right end and a wrong end. Make it totally clear to anyone standing at the wrong end that things are going badly for them.',
        'Tips': 'Eskimos had over two hundred different words for snow, without which their conversation would probably have got very monotonous.',
        'Recommendation': 'Make it totally clear that this gun has a right end and a wrong end. Make it totally clear to anyone standing at the wrong end that things are going badly for them.',
        'Description': 'It is known that there are an infinite number of worlds, simply because there is an infinite amount of space for them to be in. \n However, not every one of them is inhabited. Therefore, there must be a finite number of inhabited worlds. \n Any finite number divided by infinity is as near to nothing as makes no odds, so the average population of all the planets in the Universe can be said to be zero. \n From this it follows that the population of the whole Universe is also zero, and that any people you may meet from time to time are merely the products of a deranged imagination.',
        'Insights': [[
            'Avoid Vogons breath',
            'Avoid vogons, I that cannot be avoided, turn away in presence of a Vogon.'
        ], [
            'Don\'t panic',
            'There is a theory which states that if ever anyone discovers exactly what the Universe is for and why it is here, it will instantly disappear and be replaced by something even more bizarre and inexplicable. There is another theory which states that this has already happened.'
        ]]
    }, {

        'Code': 'Disorientation',
        'Classification': 'Severe',
        'Category': 'General',
        'Condition': 'Disorientation',
        'Learn More': 'You are lost in space.',
        'Tips': 'Eskimos had over two hundred different words for snow, without which their conversation would probably have got very monotonous.',
        'Recommendation': 'Anyone who is capable of getting themselves made President should on no account be allowed to do the job.',
        'Description': 'Isn\'t it enough to see that a garden is beautiful without having to believe that there are fairies at the bottom of it too?',
        'Insights': [[
            'Towel',
            'A towel is about the most massively useful thing an interstellar hitchhiker can have.'
        ]]

    }];

    return {
        created: new Date().toISOString(),
        version: '9.9.9',

        survey_id: 'DEMO_SURVEY',
        session_id: 'aaaaaaaa-0000-0000-0000-aaaaaaaaaaaa',

        conditions,
    }
};
// console.log(JSON.stringify(mockAnalysis(), null, 4));


/**
 * Sets default values and normalizes an /analyse api insight object
 * @param {object} insight
 * @returns {Insight}
 *
 */
const normalizeInsight = function(insight) {
    return Object.assign(Models.insight(), insight || []);
};

/**
 * Sets default values and normalizes an /analyse api condition object
 * @param {object} condition
 * @returns {Condition}
 */
const normalizeCondition = function(condition) {
    let r = condition|| {};

    r = Object.assign(Models.condition(), r);
    const n = r.Insights.map(item => normalizeInsight(item));
    r.Insights = n

    return r;
}

/**
 * Sets default values and normalizes an /analyse api analysis object
 * @param {object} analysis
 * @returns {Analysis}
 */
const normalizeAnalysis = function(analysis) {
    let r = analysis || {};

    r = Object.assign(Models.analysis(), r);
    const c = r.conditions.map(item => normalizeCondition(item));
    r.conditions = c
    return r;
}

export {
    Models,
    mockAnalysis,

    normalizeInsight,
    normalizeCondition,
    normalizeAnalysis,

    insightPropTypes,
    conditionPropTypes,
    analysisPropTypes
};
