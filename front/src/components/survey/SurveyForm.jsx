import React from 'react';
import PropTypes from 'prop-types';

import { Fade } from '../Transitions';
import SurveyButtons from './SurveyButtons';

const SurveyForm = function(props) {
    if(!props.show) {
        return(null);
    }

    return (
        <form id={ Date.now() /*trickout autofill*/ } className="list-group">
            <input type="hidden" value={ Date.now() /*trick autofill*/ } />

            {
                /* question groups */
                props.children.map((component, index) =>  <Fade key={ index }>{ component }</Fade>)
            }

            <SurveyButtons
                handlePrev={ props.handlePrev }
                handleNext={ props.handleNext }
                handleFinish={ props.handleFinish }

                isFirst={ props.isFirst }
                isFinished={ props.isFinished }
                hasErrors={ props.hasErrors }
                hasAnswers={ props.hasAnswers }
            />
        </form>
    );
};

SurveyForm.defaultProps = {};

SurveyForm.propTypes = {
    show: PropTypes.bool.isRequired,
    handleNext: PropTypes.func.isRequired,
    handlePrev: PropTypes.func.isRequired,
    handleFinish: PropTypes.func.isRequired,

    isFirst: PropTypes.bool.isRequired,
    isFinished: PropTypes.bool.isRequired,
    hasErrors: PropTypes.bool.isRequired,
    hasAnswers: PropTypes.bool.isRequired,
};

export default SurveyForm;
