import React from 'react';
import PropTypes from 'prop-types';

import { Fade } from '../Transitions';

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
            </form>
        </Fade>
    );
};

SurveyForm.defaultProps = {};

SurveyForm.propTypes = {
    show: PropTypes.bool.isRequired,
    className: PropTypes.string,
};

export default SurveyForm;
