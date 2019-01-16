import React from 'react';
import PropTypes from 'prop-types';

import { InputGroup } from '../FormHelpers';
import Question from '../../Question';

const EmailInput = function(props) {
    const { question, placeholder } = props;

    return (
        <div className="form-group">
            <label htmlFor={ question.id }>{ question.title }</label>
            <InputGroup prepend={ question.unit }>
                <input
                    id={ question.id }
                    name={ question.name }
                    type="email"
                    className="form-control"
                    placeholder={ placeholder }
                    autoComplete="off"
                    onChange={ (e) => {
                        const { value } = e.target;
                        props.handleChange(e.target, question, value);
                    } }
                />
            </InputGroup>
        </div>
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
