import React from 'react';
import PropTypes from 'prop-types';

const canNext = function ({ hasErrors, hasAnswers, hasAllAnswers }) {
    return (!hasErrors && hasAnswers && hasAllAnswers);
};

/**
 * Render Previous/Next/Finish buttos
 * The component should not contain survey logic or handle complex data. It merely recieves a number of flags from the parent component
 */
const SurveyButtons = function(props) {
    const nextIconClass = (props.hasErrors || !props.hasAllAnswers) ? 'fas fa-ban' : 'fas fa-arrow-circle-right';
    const prevIconClass = (props.isFirst) ? 'fas fa-ban' : 'fas fa-arrow-circle-left';

    return (
        <div className={ props.className }>
            <button type="submit" className="app--btn-arrow btn btn-default"
                disabled={ props.isFirst }
                onClick={ props.handlePrev }>
                <i className={ prevIconClass } /> Previous Question
            </button>

            <button type="submit" className="app--btn-arrow btn btn-default btn-primary"
                disabled={ !canNext(props) }
                onClick={ props.handleNext }>
                Next Question <i className={ nextIconClass } />
            </button>
        </div>
    );
};

SurveyButtons.defaultProps = {
};

SurveyButtons.propTypes = {
    handleNext: PropTypes.func.isRequired,
    handlePrev: PropTypes.func.isRequired,

    hasQuestions: PropTypes.bool.isRequired,
    hasErrors: PropTypes.bool.isRequired,
    hasAnswers: PropTypes.bool.isRequired,
    hasAllAnswers: PropTypes.bool.isRequired,
    isFirst: PropTypes.bool.isRequired,
};

export default SurveyButtons;
