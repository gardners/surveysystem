import React from 'react';
import PropTypes from 'prop-types';

import { DirtyJson } from '../../Utils';
import Toggle from '../Toggle';

const Insight = function(props) {
    const { insight } = props;

    const displayName = DirtyJson.get(insight, 'displayName', '');
    const displayText = DirtyJson.get(insight, 'displayText', '');

    return (
        <div className="list-group list-group-flush">
            <Toggle><strong>{ displayName }</strong>{ displayText }</Toggle>
        </div>
    );
};

Insight.propTypes = {
    insight: PropTypes.object.isRequired,
};

const Condition = function(props) {
    const { condition } = props;

    const displayResults = DirtyJson.get(condition, 'displayResults', {});
    const additionalInsights = DirtyJson.get(displayResults, 'additionalInsights', []);

    return (
        <div className="list-group list-group-flush">
            <h5>Additional Insights</h5>
            { additionalInsights.map((insight, index) => <Insight key={ index } insight={ additionalInsights[index] } />) }
        </div>
    );
};

Condition.propTypes = {
    condition: PropTypes.object.isRequired,
};


const Evaluation = function(props) {
    const { name, evaluation } = props;
    const conditions = DirtyJson.get(evaluation, 'conditions', []);
    console.log(conditions);
    return (
        <div className="list-group list-group-flush">
            <h4>{ name } </h4>
            { conditions.map((condition, index) => <Condition key={ index } condition={ condition } />) }
        </div>
    );
};

Evaluation.propTypes = {
    name: PropTypes.string.isRequired,
    evaluation: PropTypes.object.isRequired,
};

export default Evaluation;
