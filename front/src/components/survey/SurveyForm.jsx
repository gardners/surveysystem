import React from 'react';
import PropTypes from 'prop-types';

import { Fade } from '../Transitions';
import SurveyButtons from './SurveyButtons';

const SurveyForm = function(props) {
    if(!props.show) {
        return(null);
    }

    return (
        <Fade>
            <form id={ Date.now() /*trickout autofill*/ } className="list-group">
                <input type="hidden" value={ Date.now() /*trick autofill*/ } />
                {
                    /* inputs */
                    props.children
                }
                <SurveyButtons
                    handlePrev={ props.handlePrev }
                    handleNext={ props.handleNext }

                    isFirst={ props.isFirst }
                    hasErrors={ props.hasErrors }
                    hasAnswers={ props.hasAnswers }
                    hasAllAnswers={ props.hasAllAnswers }
                />
            </form>
        </Fade>
    );
};

SurveyForm.defaultProps = {};

SurveyForm.propTypes = {
    show: PropTypes.bool.isRequired,
    handleNext: PropTypes.func.isRequired,
    handlePrev: PropTypes.func.isRequired,

    isFirst: PropTypes.bool.isRequired,
    hasErrors: PropTypes.bool.isRequired,
    hasAnswers: PropTypes.bool.isRequired,
    hasAllAnswers: PropTypes.bool.isRequired,
};

export default SurveyForm;
