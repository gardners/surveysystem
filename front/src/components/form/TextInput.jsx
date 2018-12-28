import React from 'react';
import PropTypes from 'prop-types';

const TextInput = function(props) {
    const { question, placeholder } = props;

    return (
        <div className="form-group">
            <label htmlFor={ question.id }>{ question.title }</label>
            <input
                id={ question.id }
                name={ question.name }
                type="text"
                className="form-control"
                placeholder={ placeholder }
                autoComplete="off"
                onChange={ (e) => {
                    const { value } = e.target;
                    props.handleChange(question, value);
                } }
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
    }).isRequired,
    placeholder: PropTypes.string,
};

export default TextInput;
