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
            <form id={ Date.now() /*trickout autofill*/ } className={ props.className }>
                <input type="hidden" value={ Date.now() /*trick autofill*/ } />
                {
                    /* inputs */
                    props.children
                }
                <SurveyButtons
                    className={ props.rowClassName }
                    handlePrev={ props.handlePrev }
                    handleNext={ props.handleNext }

                    hasQuestions={ props.hasQuestions }
                    hasErrors={ props.hasErrors }
                    hasAnswers={ props.hasAnswers }
                    hasAllAnswers={ props.hasAllAnswers }
                    didAnswerBefore={ props.didAnswerBefore }
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

    hasQuestions: PropTypes.bool.isRequired,
    hasErrors: PropTypes.bool.isRequired,
    hasAnswers: PropTypes.bool.isRequired,
    hasAllAnswers: PropTypes.bool.isRequired,
    didAnswerBefore: PropTypes.bool.isRequired,

    className: PropTypes.string,
    rowClassName:  PropTypes.string,
};

export default SurveyForm;
