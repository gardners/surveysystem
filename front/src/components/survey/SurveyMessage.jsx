import React from 'react';
import PropTypes from 'prop-types';

import SurveyManager from '../../SurveyManager';
import InnerHtml from '../InnerHtml';

/**
 * match status class from response status
 * @see #332
 * @see survey.h: struct next_questions { .. enum { STATUS_INFO, STATUS_WARN, STATUS_ERROR } status; ..}
 */
const getStatusClass = function(status) {
    switch (status) {
        case 1:
            return 'warning';
        case 2:
            return 'danger';
        default:
            return 'primary';
    }
};

const SurveyMessage = function({ session }) {
    const { status, message } = session;

    if (!message) {
        return (null);
    }
    const cls = getStatusClass(status);
    return (
        <div className={ `list-group-item list-group-item-${cls}` }><InnerHtml htmlContent={ message } /></div>
    );

};

SurveyMessage.propTypes = {
    session: PropTypes.instanceOf(SurveyManager).isRequired,
};

export default SurveyMessage;
