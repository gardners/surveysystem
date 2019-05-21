import React from 'react';
import PropTypes from 'prop-types';

import TextField from '@material-ui/core/TextField';

import { InputGroup } from '../FormHelpers';
import Question from '../../Question';

const EmailInput = function({ question, required, handleChange, placeholder }) {
    const { id, name } = question;

    return (
        <InputGroup prepend={ question.unit }>
            <TextField
                id={ id }
                name={ name }
                type="email"
                placeholder={ placeholder }
                autoComplete="off"
                onChange={ (e) => {
                    const { value } = e.target;
                    handleChange(e.target, question, value);
                } }
            />
        </InputGroup>
    );
};

EmailInput.defaultProps = {
    required: true,
    placeholder: null,
};

EmailInput.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: Question.propTypes().isRequired,
    required: PropTypes.bool,
    placeholder: PropTypes.string,
};

export default EmailInput;
