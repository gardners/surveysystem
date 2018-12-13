import React from 'react';
import PropTypes from 'prop-types';

const TextInput = function(props) {
    const { question, handleChange, placeholder } = props;

    return (
        <div className="form-group">
            <label htmlFor={ question.id }>{ question.title_text }</label>
            <input
                id={ question.id }
                name={ question.name }
                type="text"
                className="form-control"
                placeholder={ placeholder }
                autoComplete="off"
                onChange={ (e) => handleChange(e.target.value, question) }
                defaultValue={ question.defaultValue || null }
            />
        </div>
    );
};

TextInput.defaultProps = {
    placeholder: null,
};

TextInput.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: PropTypes.shape({
        id: PropTypes.string.isRequired,
        name: PropTypes.string.isRequired,
        title: PropTypes.string.isRequired,
        title_text: PropTypes.string.isRequired,
        type: PropTypes.string.isRequired,

        defaultValue: PropTypes.string,
    }).isRequired,
    placeholder: PropTypes.string,
};

export default TextInput;
