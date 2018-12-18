import React from 'react';
import PropTypes from 'prop-types';

const TextArea = function(props) {
    const { question } = props;

    return (
        <div className="form-group">
            <label htmlFor={ question.id }>{ question.title_text }</label>
            <textarea
                id={ question.id }
                name={ question.name }
                type="text"
                className="form-control"
                autoComplete="off"
                onChange={ (e) => {
                    const { value } = e.target;
                    props.handleChange(question, value);
                } }
            />
        </div>
    );
};

TextArea.defaultProps = {
};

TextArea.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: PropTypes.shape({
        id: PropTypes.string.isRequired,
        name: PropTypes.string.isRequired,
        title: PropTypes.string.isRequired,
        title_text: PropTypes.string.isRequired,
        type: PropTypes.string.isRequired,
    }).isRequired,
};

export default TextArea;
