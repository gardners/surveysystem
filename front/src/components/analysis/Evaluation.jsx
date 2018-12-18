import React from 'react';
import PropTypes from 'prop-types';

import { DirtyJson, camelToNormal } from '../../Utils';
import Toggle from '../Toggle';

const SleepCondition = function(props) {
    const { name, condition } = props;

    return (
        <div className="list-group-item mb-1">
            <Toggle>{ name }{ condition }</Toggle>
        </div>
    );
};

SleepCondition.propTypes = {
    condition: PropTypes.string.isRequired,
    name: PropTypes.string.isRequired,
};

const Insight = function(props) {
    const { insight } = props;

    const displayName = DirtyJson.get(insight, 'displayName', '');
    const displayText = DirtyJson.get(insight, 'displayText', '');

    return (
        <div className="list-group-item mb-1">
            <Toggle>{ displayName }{ displayText }</Toggle>
        </div>
    );
};

Insight.propTypes = {
    insight: PropTypes.object.isRequired,
};

const Condition = function(props) {
    const { condition } = props;

    const rank = DirtyJson.get(condition, 'rank');
    const riskRating = DirtyJson.get(condition, 'riskRating');
    const category = DirtyJson.get(condition, 'category');
    const classification = DirtyJson.get(condition, 'classification');
    const recommendation = DirtyJson.get(condition, 'recommendation');
console.log(recommendation);
    const displayResults = DirtyJson.get(condition, 'displayResults', {});
    const additionalInsights = DirtyJson.get(displayResults, 'additionalInsights', []);
    const sleepConditions = DirtyJson.get(displayResults, 'sleepConditions', {});

    return (
        <div className={ props.className }>
            <div className="table-responsive">
                <table className="table table-sm">
                    <tbody>
                        <tr>
                            <th>Rank</th>
                            <td>{ rank }</td>
                        </tr>
                        <tr>
                            <th>Risk Rating</th>
                            <td>{ riskRating }</td>
                        </tr>
                        <tr>
                            <th>Category</th>
                            <td>{ category }</td>
                        </tr>
                        <tr>
                            <th>Classification</th>
                            <td>{ classification }</td>
                        </tr>
                    </tbody>
                </table>
            </div>

            <h4>Recommendation</h4>
            { (recommendation) ? <p>{ recommendation }</p> : <i>No recommendation</i> }

            <h4>Sleep Conditions</h4>
            <div className="list-group mb-2">
                { Object.keys(sleepConditions).map((key) => <SleepCondition key={ key } name={ camelToNormal(key) } condition={ sleepConditions[key]  || 'n/a' } />) }
            </div>

            <h4>Additional Insights</h4>
            <div className="list-group mb-2">
                { !additionalInsights.length && <div className="list-group-item mb-1"><i>No additional insights</i></div> }
                { additionalInsights.map((insight, index) => <Insight key={ index } insight={ additionalInsights[index] || 'n/a' } />) }
            </div>
        </div>
    );
};

Condition.propTypes = {
    condition: PropTypes.object.isRequired,
    className: PropTypes.string,
};


const Evaluation = function(props) {
    const { name, evaluation } = props;

    const conditions = DirtyJson.get(evaluation, 'conditions', []);
    const algorithm = DirtyJson.get(evaluation, 'algorithm', '');

    return (
        <div>
            <h3>{ name }</h3>
            { conditions.map((condition, index) => <Condition className="mb-2" key={ index } condition={ condition } />) }
            <p className="text-right"><small><i>Algorithm { algorithm }</i></small></p>
        </div>
    );
};

Evaluation.propTypes = {
    name: PropTypes.string.isRequired,
    evaluation: PropTypes.object.isRequired,
};

export default Evaluation;
