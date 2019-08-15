import React from 'react';
import PropTypes from 'prop-types';

import SurveyManager from '../../SurveyManager';
import SurveyList from '../../SurveyList';
import Modal from '../Modal';

const SurveySection = function({ survey, children }) {
  const info = SurveyList.getById(survey.surveyID);
  const name = (info.name) ? info.name : survey.surveyID;
  
  return (
      <section>
          <h1>{ name }</h1>
          { children }
      </section>
  );
  
};

SurveySection.propTypes = {
    survey: PropTypes.instanceOf(SurveyManager).isRequired,
};

export default SurveySection;
