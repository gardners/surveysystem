import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Question from '../../Question';

class RadioGroup extends Component {
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
                <label> { question.title } { question.unit && <em>({ question.unit })</em> }</label><br />
                { choices.map((value, index) => {
                    return <div key={index} className="radio form-check form-check-inline">
                            <input
                                type="radio"
                                name={ question.name }
                                className="form-check-input"
                                id={ question.id }
                                value={ value }
                                onChange={ this.handleChange.bind(this)}
                                checked={ value === this.state.value }
                            />
                            <label className="form-check-label">{ value }</label>
                    </div>
                }) }
            </div>
        );
    }
}

RadioGroup.defaultProps = {
    required: true,
};

RadioGroup.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: Question.propTypes(true).isRequired,
    required: PropTypes.bool,
};

export default RadioGroup;
