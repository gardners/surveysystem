import React from 'react';
import PropTypes from 'prop-types';

import TextField from '@material-ui/core/TextField';

import { InputGroup } from '../FormHelpers';
import Question from '../../Question';

const Textarea = function({ question, handleChange, placeholder, required }) {
    const { id, name, unit } = question;

    return (
        <InputGroup prepend={ unit }>
            <TextField
                id={ id }
                name={ name }
                multiline
                rows="4"
                className="form-control"
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

Textarea.defaultProps = {
    required: true,
    placeholder: null,
};

Textarea.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: Question.propTypes().isRequired,
    required: PropTypes.bool,
    placeholder: PropTypes.string,
};

export default Textarea;
