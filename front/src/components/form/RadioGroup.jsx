import React, { Component } from 'react';
import PropTypes from 'prop-types';

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
                <label> { question.title }</label>
                { choices.map((value, index) => {
                    return <div key={index} className="radio">
                        <label>
                            <input
                                type="radio"
                                name={ question.name }
                                id={ question.id }
                                value={ value }
                                onChange={ this.handleChange.bind(this)}
                                checked={ value === this.state.value }
                                />
                            { value }
                        </label>
                    </div>
                }) }
            </div>
        );
    }
}

RadioGroup.defaultProps = {
};

RadioGroup.propTypes = {
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
};

export default RadioGroup;
