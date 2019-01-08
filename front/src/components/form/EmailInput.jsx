import React from 'react';
import PropTypes from 'prop-types';

import { InputGroup } from '../FormHelpers';

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
    placeholder: null,
};

EmailInput.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: PropTypes.shape({
        id: PropTypes.string.isRequired,
        name: PropTypes.string.isRequired,
        title: PropTypes.string.isRequired,
        title_text: PropTypes.string.isRequired,
        type: PropTypes.string.isRequired,
        unit: PropTypes.string.isRequired,
    }).isRequired,
    placeholder: PropTypes.string,
};

export default EmailInput;
