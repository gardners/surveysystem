import React, { Component } from 'react';
import PropTypes from 'prop-types';

class Select extends Component {
    constructor(props) {
        super(props);
        this.state = {
            value: props.question.defaultValue || '',
        };
    }

    handleChange(e) {
        const { value } = e.target;
        const { question } = this.props;

        this.setState({
            value: value
        });
        this.props.handleChange(value, question);
    }

    render() {
        const { question } = this.props;
        const { choices } = question;
        return (
            <div className="form-group">
                <p><strong> { question.title_text }</strong></p>
                <select
                    id={ question.id }
                    name={ question.name }
                    className="form-control">
                { choices.map((value, index) => {
                    return  <option key={ index } value={ value } selected={ value === this.state.value }>{ value }</option>
                }) }
                </select>
            </div>
        );
    }
}

Select.defaultProps = {
    placeholder: null,
};

Select.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: PropTypes.shape({
        id: PropTypes.string.isRequired,
        name: PropTypes.string.isRequired,
        title: PropTypes.string.isRequired,
        title_text: PropTypes.string.isRequired,
        type: PropTypes.string.isRequired,

        choices: PropTypes.array.isRequired,
    }).isRequired,
    placeholder: PropTypes.string,
};

export default Select;
