import React from 'react';
import PropTypes from 'prop-types';

const HasErrors = function({ hasErrors }) {
    if (!hasErrors) {
        return (null);
    }

    return (
        <p className="text-danger">
            Please fix errors above
        </p>
    );
};

HasErrors.propTypes = {
    hasErrors: PropTypes.bool.isRequired,
};

/**
 * Render Previous/Next/Finish buttos
 * The component should not contain survey logic or handle complex data. It merely recieves a number of flags from the parent component
 */
const SurveyButtons = function(props) {
    const nextIconClass = (props.hasErrors || !props.hasAllAnswers) ? 'fas fa-ban' : 'fas fa-arrow-circle-right';
    const prevIconClass = (props.hasErrors || !props.hasAllAnswers) ? 'fas fa-ban' : 'fas fa-arrow-circle-left';

    if (!props.hasAnswers) {
        return (
            <div className={ props.className }>
                <button type="submit" className="app--btn-arrow btn btn-default btn-primary"
                    disabled={ true }
                    onClick={ props.handleNext }>Next Question <i className={ nextIconClass }></i></button>
            </div>
        );
    }

    if (props.isFirst) {
        return (
            <div className={ props.className }>
                <HasErrors hasErrors={ props.hasErrors } />
                <button type="submit" className="app--btn-arrow btn btn-default btn-primary"
                    disabled={ props.hasErrors }
                    onClick={ props.handleNext }>Next Question <i className={ nextIconClass }></i></button>
            </div>
        );
    }

    return (
        <div className={ props.className }>
            <HasErrors
                hasErrors={ props.hasErrors }
            />
            <button type="submit" className="app--btn-arrow btn btn-default"
                onClick={ props.handlePrev }>
                <i className={ prevIconClass } /> Previous Question
            </button>
            <button type="submit" className="app--btn-arrow btn btn-default btn-primary"
                disabled={ props.hasErrors || !props.hasAllAnswers }
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

    isFirst: PropTypes.bool.isRequired,
    hasErrors: PropTypes.bool.isRequired,
    hasAnswers: PropTypes.bool.isRequired,
    hasAllAnswers: PropTypes.bool.isRequired,
};

export default SurveyButtons;
