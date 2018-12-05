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
        const { type } = question;

        this.setState({
            value: value
        });

        this.props.handleChange({
            [type]: this.state.value,
        }, question);
    }

    render() {
        console.log(this);
        const { question } = this.props;
        const { choices } = question;
        return (
            <div className="form-group">
                <label> { question.title_text }</label>
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
    placeholder: null,
};

RadioGroup.propTypes = {
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

export default RadioGroup;
