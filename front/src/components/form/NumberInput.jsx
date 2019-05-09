import React from 'react';
import PropTypes from 'prop-types';

import { InputGroup } from '../FormHelpers';
import Question from '../../Question';

const NumberInput = function(props) {
    const { question, placeholder } = props;

    return (
        <InputGroup prepend={ question.unit }>
            <input
                id={ question.id }
                name={ question.name }
                type="number"
                className="form-control"
                placeholder={ placeholder }
                autoComplete="off"
                onChange={ (e) => {
                    const { value } = e.target;
                    props.handleChange(e.target, question, value);
                } }
                onKeyDown={ (e) => {
                    if (!e.target.checkValidity()) {
                        const msg = e.target.validationMessage || 'Please enter an number!';
                        props.handleChange(e.target, question, new Error(msg));
                    }
                } }
            />
        </InputGroup>
    );
};

NumberInput.defaultProps = {
    required: true,
    placeholder: null,
};

NumberInput.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: Question.propTypes().isRequired,
    required: PropTypes.bool,
    placeholder: PropTypes.string,
};

export default NumberInput;
