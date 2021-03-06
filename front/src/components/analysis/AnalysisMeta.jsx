import React from 'react';
import PropTypes from 'prop-types';

import { Models,  normalizeAnalysis, analysisPropTypes } from '../../Analysis';

const parseISODate = function(str) {

    if(!str) {
        return 'n/a';
    }

    let d = NaN;
    try {
        d = new Date(str);
    } catch (e) {
        // nothing
    }

    return (!d || isNaN(d)) ? 'n/a' : d.toLocaleString();
};

const Constraints = function({ constraints }) {
    if (!constraints.length) {
        return (null);
    }

    return(
        <div className="alert alert-warning">
            <strong className="text-danger">Constraints:</strong> The analysis encountered following unexpected survey results:
            <ul>
            {
                constraints.map((item, index) => <li key={ index }><strong>{ item.key }</strong>: { item.message }</li>)
            }
            </ul>
        </div>
    );
};

Constraints.defaultProps = {
    constraints: [],
};

Constraints.propTypes = {
    contstraints: PropTypes.array,
};


const AnalysisMeta = function({ survey_id, session_id, analysis }) {
    const data = normalizeAnalysis(analysis);
    const { version, created, constraints } = data;

    return (
        <React.Fragment>
            <h2>Survey: <span className="text-success">{ survey_id }</span></h2>
            <p>
                session: <small className="text-muted">{ session_id } </small><br />
                algorithm: <small className="text-muted">version: { (version) ? version : 'n/a' }, created: { parseISODate(created) }</small>
            </p>
            <Constraints constraints= { constraints } />
        </React.Fragment>
    );
};

AnalysisMeta.defaultProps = {
    analysis: Models.analysis(),
};

AnalysisMeta.propTypes = {
    survey_id: PropTypes.string.isRequired,
    session_id: PropTypes.string.isRequired,
    analysis: analysisPropTypes(),
};

export default AnalysisMeta;
