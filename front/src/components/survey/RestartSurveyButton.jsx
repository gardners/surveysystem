import React from 'react';
import PropTypes from 'prop-types';

import LocalStorage from '../../storage/LocalStorage';

// config
const { REACT_APP_SURVEY_CACHEKEY } = process.env;

const restartSurvey = function() {
    // no e.preventDefault(); we DO want to refresh here
    LocalStorage.delete(REACT_APP_SURVEY_CACHEKEY);
    window.location.reload();
};

const RestartSurveyButton = function({ className, children }) {

    if(!LocalStorage.get(REACT_APP_SURVEY_CACHEKEY)) {
        return (null);
    }

    return(
        <button className={ className } onClick={ restartSurvey }>{ children }</button>
    );
};

RestartSurveyButton.propTypes = {
    className: PropTypes.string,
};

export default RestartSurveyButton;
