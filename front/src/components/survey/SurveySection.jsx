import React from 'react';
import PropTypes from 'prop-types';

import SurveyManager from '../../SurveyManager';
import SurveyHeader from './SurveyHeader';

const SurveySection = function({ session, children }) {

  return (
      <section>
          <SurveyHeader session={ session }/>
          { children }
      </section>
  );
  
};

SurveySection.propTypes = {
    session: PropTypes.instanceOf(SurveyManager).isRequired,
};

export default SurveySection;
