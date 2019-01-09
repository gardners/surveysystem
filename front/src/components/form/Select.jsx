import React, { Component } from 'react';
import PropTypes from 'prop-types';

import { InputGroup } from '../FormHelpers';

class Select extends Component {
    constructor(props) {
        super(props);
        this.state = {
            value: '',
        };
    }

    handleChange(e) {
        const { value } = e.target;
        const { question } = this.props;

        this.setState({
            value: value
        });

        this.props.handleChange(e.target, question, value);
    }

    render() {
        const { question } = this.props;
        const { choices } = question;
        return (
            <div className="form-group">
                <label htmlFor={ question.id }>{ question.title }</label>
                <InputGroup prepend={ question.unit }>
                    <select
                        onChange={ this.handleChange.bind(this) }
                        id={ question.id }
                        name={ question.name }
                        className="form-control">
                    { choices.map((value, index) => {
                        return <option key={ index } value={ value }>{ value }</option>
                    }) }
                    </select>
                </InputGroup>
            </div>
        );
    }
}

Select.defaultProps = {
    required: true,
};

Select.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: PropTypes.shape({
        id: PropTypes.string.isRequired,
        name: PropTypes.string.isRequired,
        title: PropTypes.string.isRequired,
        title_text: PropTypes.string.isRequired,
        type: PropTypes.string.isRequired,
        unit: PropTypes.string.isRequired,
        // eunum
        choices: PropTypes.array.isRequired,
    }).isRequired,
    required: PropTypes.bool,
};

export default Select;
