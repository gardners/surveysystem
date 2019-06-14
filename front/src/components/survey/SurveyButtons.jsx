import React from 'react';
import PropTypes from 'prop-types';

const HasErrors = function({ hasErrors, hasAllAnswers, hasAnswers }) {
    if (!hasAnswers) {
        return null;
    }

    return (
        <ul className="list-group mb-1">
            { hasErrors && <li className="list-group-item list-group-item-danger">Please fix errors above</li> }
            { !hasAllAnswers && <li className="list-group-item list-group-item-warning">Please answer all questions</li> }
        </ul>
    );
};

HasErrors.propTypes = {
    hasErrors: PropTypes.bool.isRequired,
    hasAllAnswers: PropTypes.bool.isRequired,
    hasAnswers: PropTypes.bool.isRequired,
};

const canNext = function ({ hasErrors, hasAnswers, hasAllAnswers }) {
    return (!hasErrors && hasAnswers && hasAllAnswers);
};

/**
 * Render Previous/Next/Finish buttos
 * The component should not contain survey logic or handle complex data. It merely recieves a number of flags from the parent component
 */
const SurveyButtons = function(props) {
    const nextIconClass = (props.hasErrors || !props.hasAllAnswers) ? 'fas fa-ban' : 'fas fa-arrow-circle-right';
    const prevIconClass = (props.didAnswerBefore) ? 'fas fa-arrow-circle-left' :  'fas fa-ban';

    return (
        <div className={ props.className }>
            <HasErrors
                hasErrors={ props.hasErrors }
                hasAllAnswers={ props.hasAllAnswers }
                hasAnswers={ props.hasAnswers }
            />

            <button type="submit" className="app--btn-arrow btn btn-default"
                disabled={ !props.didAnswerBefore }
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
    didAnswerBefore: PropTypes.bool.isRequired,
};

export default SurveyButtons;
