import React from 'react';

import { Models,  normalizeAnalysis, analysisPropTypes } from '../../Analysis';
import { isoDateToLocale } from '../../Utils';


const AnalysisMeta = function({ analysis }) {
    const { survey_id, session_id, version, created } = normalizeAnalysis(analysis);

    return (
        <React.Fragment>
            <h2><span className="text-muted">Survey:</span> { survey_id }</h2>
            <div className="mb-4 ml-4">
                <small>session: <span  className="text-muted">{ session_id }</span></small><br />
                <small>algorithm: <span  className="text-muted">{ (version) ? version : 'n/a' }</span></small><br />
                <small>created: <span  className="text-muted">{ isoDateToLocale(created) }</span></small>
            </div>
        </React.Fragment>
    );
};

AnalysisMeta.defaultProps = {
    analysis: Models.analysis(),
};

AnalysisMeta.propTypes = {
    analysis: analysisPropTypes(),
};

export default AnalysisMeta;
