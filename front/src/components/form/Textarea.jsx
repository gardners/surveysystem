import React from 'react';
import PropTypes from 'prop-types';

const Textarea = function(props) {
    const { question } = props;

    return (
        <div className="form-group">
            <label htmlFor={ question.id }>{ question.title }</label>
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

Textarea.defaultProps = {
};

Textarea.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: PropTypes.shape({
        id: PropTypes.string.isRequired,
        name: PropTypes.string.isRequired,
        title: PropTypes.string.isRequired,
        title_text: PropTypes.string.isRequired,
        type: PropTypes.string.isRequired,
    }).isRequired,
};

export default Textarea;
