import React from 'react';
import PropTypes from 'prop-types';

import { DirtyJson } from '../../Utils';
import Toggle from '../Toggle';

const Feedback = function(props) {
    const { feedback } = props;
    const thoughts = (feedback) ? DirtyJson.coerceArray(feedback.thoughts) : [];
    const insights = (feedback) ? DirtyJson.coerceArray(feedback.insights) : [];

    return (
        <div className="list-group list-group-flush">
            <div className="list-group-item">
                <h4>Thoughts</h4>
                { !thoughts.length && <i>no entries</i> }
                { thoughts.map((thought, i) =>
                    Object.keys(thought).map((summary, k) =>
                        <Toggle key={ `${i}-${k}` }>
                            <strong>{summary}</strong>{ thought[summary] }
                        </Toggle>)) }
            </div>
            <div className="list-group-item">
            <h4>Insights</h4>
                { !insights.length && <i>no entries</i> }
                { insights.map((insight, i) =>
                    Object.keys(insight).map((summary, k) =>
                        <Toggle key={ `${i}-${k}` }>
                            <strong>{summary}</strong>{ insight[summary] }
                        </Toggle>)) }
            </div>
        </div>
    );
};

Feedback.propTypes = {
    feedback: PropTypes.object,
};

export default Feedback;
