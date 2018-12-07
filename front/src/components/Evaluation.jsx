import React from 'react';
import PropTypes from 'prop-types';

import { Link } from 'react-router-dom';

// devel
import Dev from './Dev';

const Evaluation = function(props) {
    const state = props.location.state || null;
    const evaluation = (state) ? state.evaluation : null;
    const surveyID = props.match.params.id;

    if(!evaluation) {
        return (
            <section>
                <h1>{ surveyID }: Evaluation</h1>
                <div className="text-danger">
                    <p><i className="fas fa-exclamation-circle"></i> This survey is not finished yet!</p>
                    <Link to={ `/survey/${surveyID}` } className="btn bn-lg btn-primary">Continue survey</Link>
                </div>
            </section>
        );
    }

    return (
        <section>
            <h1>{ surveyID } <small>Evaluation</small></h1>
            <Dev label="survey" data={ evaluation } open={ true }/>
        </section>
    );

}

Evaluation.propTypes = {
};

export default Evaluation;
