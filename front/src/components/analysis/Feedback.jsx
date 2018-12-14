import React from 'react';
import PropTypes from 'prop-types';

import { DirtyJson } from '../../Utils';
import Toggle from '../Toggle';

const Feedback = function(props) {
    const { feedback } = props;
    const thoughts = (feedback) ? DirtyJson.coerceArray(feedback.thoughts) : [];
    const insights = (feedback) ? DirtyJson.coerceArray(feedback.insights) : [];

    return (
        <div>
            <h4>Thoughts</h4>
            <div className="mb-2">
                { !thoughts.length && <p><i>no thoughts</i></p> }
                { thoughts.map((thought, i) =>
                    Object.keys(thought).map((summary, k) =>
                        <div key={ `${i}-${k}` }>
                            <p className="lead" >{ summary }</p>
                            <p>{ thought[summary] }</p>
                        </div>)) }
            </div>

            <h4>Insights</h4>
            <div className="list-group mb-2">
                { !insights.length && <p><i>no insights</i></p> }
                { insights.map((insight, i) =>
                    Object.keys(insight).map((summary, k) =>
                        <p key={ `${i}-${k}` }>
                            <strong>{ summary }</strong>: { insight[summary] }
                        </p>)) }
            </div>
        </div>
    );
};

Feedback.propTypes = {
    feedback: PropTypes.object,
};

export default Feedback;
