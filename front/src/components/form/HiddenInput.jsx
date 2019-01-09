import React, { Component } from 'react';
import PropTypes from 'prop-types';

// no Inputgroup

class HiddenInput extends Component {

    // ! immediately invoke answer callback
    // note: no value and validation! handleChange is invoked immediately with defaultvalue
    componentDidMount() {
        const { question, defaultValue } = this.props;
        this.props.handleChange(null, question, defaultValue);
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
    required: true,
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
        unit: PropTypes.string.isRequired,
    }).isRequired,
    // no "required"prop

    // custom
    defaultValue: PropTypes.string,
};

export default HiddenInput;
