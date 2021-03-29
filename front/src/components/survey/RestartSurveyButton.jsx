import React from 'react';
import PropTypes from 'prop-types';

import Session, { delete_cached_session } from '../../Session';

const restartSurvey = function() {
    // no e.preventDefault(); we DO want to refresh here
    delete_cached_session();
    window.location.reload();
};

const RestartSurveyButton = function({ session, className, children }) {

    if(!session) {
        return (null);
    }

    return(
        <button className={ className } onClick={ restartSurvey }>{ children }</button>
    );
};

RestartSurveyButton.defaultProps = {
    session: null,
};

RestartSurveyButton.propTypes = {
    session: PropTypes.instanceOf(Session),
    className: PropTypes.string,
};

export default RestartSurveyButton;
