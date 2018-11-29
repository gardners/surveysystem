import React from 'react';
import PropTypes from 'prop-types';

const FormRow = function(props) {
    return(
        <div className={ props.className }>
            { props.legend && <legend>{ props.legend }</legend> }
            { props.children }
        </div>
    );
};

FormRow.propTypes = {
    className: PropTypes.string,
    legend:  PropTypes.string,
};

const FieldValidator = function(props) {
    const error = (props.answer) ? props.answer.error : '';

    if(!error) {
        return(null);
    }

    return(
        <div className="text-danger">
            { error }
        </div>
    );
};

FieldValidator.propTypes = {
    answer: PropTypes.shape({
        answer: PropTypes.any,
        error: PropTypes.string,
        question: PropTypes.object,
    }),
};

export { FormRow, FieldValidator };
