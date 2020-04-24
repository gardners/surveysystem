import React from 'react';
import PropTypes from 'prop-types';

import SurveyManager from '../../SurveyManager';

import SurveyHeader from './SurveyHeader';
import RestartSurveyButton from './RestartSurveyButton';

const SurveySection = function({ session, children }) {
    const { sessionID } = session;

    return (
        <section>
            <SurveyHeader session={ session }/>
            { children }
            <pre className="text-right" style={ { fontSize: '.85em' } }>
                session: { (session) ? sessionID : '' } <RestartSurveyButton className="btn btn-sm btn-link">Restart survey</RestartSurveyButton>
            </pre>
        </section>
    );

};

SurveySection.propTypes = {
    session: PropTypes.instanceOf(SurveyManager).isRequired,
};

export default SurveySection;
