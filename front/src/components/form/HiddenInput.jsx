import React, { Component } from 'react';
import PropTypes from 'prop-types';

class HiddenInput extends Component {

    // ! immediately invoke answer callback
    // note: no value and validation! handleChange is invoked immediately with defaultvalue
    componentDidMount() {
        const { question, defaultValue } = this.props;
        this.props.handleChange(question, defaultValue);
    }

    render() {
        const { question, defaultValue } = this.props;
        return (
            <div className="form-group">
                <input
                    id={ question.id }
                    name={ question.name }
                    type="hidden"
                    autoComplete="off"
                    defaultValue={ defaultValue }
                />
            </div>
        );
    }
};

HiddenInput.defaultProps = {
    defaultValue: null,
};

HiddenInput.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: PropTypes.shape({
        id: PropTypes.string.isRequired,
        name: PropTypes.string.isRequired,
        title: PropTypes.string.isRequired,
        title_text: PropTypes.string.isRequired,
        type: PropTypes.string.isRequired,
    }).isRequired,

    // custom
    defaultValue: PropTypes.string,
};

export default HiddenInput;
