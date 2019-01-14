import React from 'react';
import PropTypes from 'prop-types';

const HasErrors = function(props) {
    if (!props.has) {
        return (null);
    }

    return (
        <p className="text-danger">Please fix errors above</p>
    );
};

HasErrors.propTypes = {
    has: PropTypes.bool.isRequired,
};


/**
 * Render Previous/Next/Finish buttos
 * The component should not contain survey logic or handle complex data. It merely recieves a number of flags from the parent component
 */
const SurveyButtons = function(props) {
    const nextIconClass = (props.hasErrors) ? 'fas fa-ban' : 'fas fa-arrow-circle-right';
    const prevIconClass = (props.hasErrors) ? 'fas fa-ban' : 'fas fa-arrow-circle-left';

    if (!props.hasAnswers) {
        return (
            <React.Fragment>
                <button type="submit" className="app--btn-arrow btn btn-default btn-primary"
                    disabled={ true }
                    onClick={ props.handleNext }>Next Question <i className={ nextIconClass }></i></button>
            </React.Fragment>
        );
    }

    if (props.isFirst) {
        return (
            <React.Fragment>
                <HasErrors has={ props.hasErrors } />
                <button type="submit" className="app--btn-arrow btn btn-default btn-primary"
                    disabled={ props.hasErrors }
                    onClick={ props.handleNext }>Next Question <i className={ nextIconClass }></i></button>
            </React.Fragment>
        );
    }

    if (props.isFinished) {
        return (
            <React.Fragment>
                <button type="submit" className="btn btn-default btn-primary"
                    onClick={ props.handleFinish }>Finish Survey</button>
            </React.Fragment>
        );
    }

    return (
        <React.Fragment>
            <HasErrors has={ props.hasErrors} />
            <button type="submit" className="app--btn-arrow btn btn-default"
                onClick={ props.handlePrev }>
                <i className={ prevIconClass } /> Previous Question
            </button>
            <button type="submit" className="app--btn-arrow btn btn-default btn-primary"
                disabled={ props.hasErrors }
                onClick={ props.handleNext }>
                Next Question <i className={ nextIconClass } />
            </button>
        </React.Fragment>
    );
};

SurveyButtons.defaultProps = {
};

SurveyButtons.propTypes = {
    handleNext: PropTypes.func.isRequired,
    handlePrev: PropTypes.func.isRequired,
    handleFinish: PropTypes.func.isRequired,

    isFirst: PropTypes.bool.isRequired,
    isFinished: PropTypes.bool.isRequired,
    hasErrors: PropTypes.bool.isRequired,
    hasAnswers: PropTypes.bool.isRequired,
};

export default SurveyButtons;
