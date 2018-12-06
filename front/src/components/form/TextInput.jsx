import React, { Component } from 'react';
import PropTypes from 'prop-types';

class TextInput extends Component {
    constructor(props) {
        super(props);
        this.state = {
            value: '',
        };
    }

    handleChange(e) {
        const { value } = e.target;
        const { question } = this.props;
        const { type } = question;

        this.setState({
            value: value
        });

        this.props.handleChange({
            [type]: value,
        }, question);
    }

    render() {
        const { question, placeholder } = this.props;
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
                    onChange={ this.handleChange.bind(this) }
                />
            </div>
        );
    }
}

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
