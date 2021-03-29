import React from 'react';
import PropTypes from 'prop-types';

import Session from '../../Session';

import SurveyHeader from './SurveyHeader';
import RestartSurveyButton from './RestartSurveyButton';

const SurveySection = function({ session, children }) {
    const { session_id } = session;

    return (
        <section>
            <SurveyHeader session={ session }/>
            { children }
            <pre className="text-right" style={ { fontSize: '.85em' } }>
                session: { session_id } <RestartSurveyButton className="btn btn-sm btn-link">Restart survey</RestartSurveyButton>
            </pre>
        </section>
    );

};

SurveySection.propTypes = {
    session: PropTypes.instanceOf(Session).isRequired,
};

export default SurveySection;
