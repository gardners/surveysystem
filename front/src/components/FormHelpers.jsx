import React from 'react';
import PropTypes from 'prop-types';

const FormRow = function(props) {
    return(
        <div className={ props.className }>
            { props.legend && (typeof props.legend === 'function') ? props.legend() : <legend>{ props.legend }</legend> }
            { props.children }
        </div>
    );
};

FormRow.propTypes = {
    className: PropTypes.string,
    legend:  PropTypes.oneOfType([
        PropTypes.string,
        PropTypes.func,
    ]),
};

const FieldError = function(props) {

    if(!props.error) {
        return(null);
    }

    return(
        <div className="text-danger">
            { (typeof error === 'string') ? props.error : props.error.toString() }
        </div>
    );
};

FieldError.propTypes = {
    error: PropTypes.oneOf([
        PropTypes.string,
        PropTypes.instanceOf(Error),
        PropTypes.array,
    ]),
};

const FieldValidator = function(props) {
    const error = (props.answer) ? props.answer.error : '';

    if(!error) {
        return (null);
    }

    return(
        <FieldError error={error}/ >
    );
};

FieldValidator.propTypes = {
    answer: PropTypes.shape({
        answer: PropTypes.any,
        error: PropTypes.string,
        question: PropTypes.object,
    }),
};

export { FormRow, FieldError, FieldValidator };
