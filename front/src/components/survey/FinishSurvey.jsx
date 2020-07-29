import React from 'react';
import PropTypes from 'prop-types';

import { Link } from 'react-router-dom';

import SurveyManager from '../../SurveyManager';

/**
 * #379 finish survey display , fetch analysis or go back
 */
const FinishSurvey = function({ session, handleDelAnswer }) {
    return (
        <div className="card">
            <div className="card-header">
                <h2 className="card-title m-0"> <i className="fas fa-check-circle"></i> Survey completed.</h2>
            </div>
            <div className="card-body">
                <p className="card-text">Thank you for your time!
                <br />Close this survey and get your analysis or go back to the <button className="btn btn-link p-0 m-0" onClick= { handleDelAnswer }>last question.</button></p>
                <Link className="btn btn-lg btn-primary pl-5 pr-5 mb-2 mt-2" to={ `/analyse/${session.surveyID}/${session.sessionID}` }>Get Analysis</Link>
            </div>
        </div>
    );
};

FinishSurvey.defaultProps = {};

FinishSurvey.propTypes = {
    session: PropTypes.instanceOf(SurveyManager).isRequired,
    handleDelAnswer: PropTypes.func.isRequired,
};

export default FinishSurvey;
