import React from 'react';
import PropTypes from 'prop-types';

import Box from '@material-ui/core/Box';

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
                <Box boxShadow={ 2 } mb={ 2 } px={ 4 } py={ 2 }>
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
                </Box>
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
};

export default SurveyForm;
