import React from 'react';
import PropTypes from 'prop-types';

import Session from '../../Session';

/**
 * #379 finish survey display , fetch analysis or go back
 */
const FinishSurvey = function({ session, handleDelAnswer, handleGetAnalysis }) {
    return (
        <div className="card">
            <div className="card-header">
                <h2 className="card-title m-0"> <i className="fas fa-check-circle"></i> Survey completed.</h2>
            </div>
            <div className="card-body">
                <p className="card-text">
                    Thank you for your time!<br />
                    {
                        (!session.isClosed()) ?
                            <React.Fragment>
                                Close this survey and get your analysis or go back to the <button className="btn btn-link p-0 m-0" onClick={ handleDelAnswer }>last question.</button>
                            </React.Fragment> : ''
                    }
                </p>
                <button className="btn btn-lg btn-primary pl-5 pr-5 mb-2 mt-2" onClick={ handleGetAnalysis }>Get Analysis</button>
            </div>
        </div>
    );
};

FinishSurvey.defaultProps = {};

FinishSurvey.propTypes = {
    session: PropTypes.instanceOf(Session).isRequired,
    handleDelAnswer: PropTypes.func.isRequired,
    handleGetAnalysis: PropTypes.func.isRequired,
};

export default FinishSurvey;
