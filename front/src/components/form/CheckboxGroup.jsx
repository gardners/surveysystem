import React, { Component } from 'react';
import PropTypes from 'prop-types';

class CheckboxGroup extends Component {
    constructor(props) {
        super(props);
        this.state = {
            values: [],
        };
    }

    handleChange(e) {
        const { value, checked } = e.target;
        const { question } = this.props;

        let { values } = this.state;

        values = values.filter(v => v !== value);
        if(checked) {
            values.push(value);
        }

        this.setState({
            values,
        });

        this.props.handleChange(question, values);
    }

    render() {
        const { question } = this.props;
        const { choices } = question;

        return (
            <div className="form-group">
                <label> { question.title_text }</label>
                { choices.map((value, index) => {
                    return <div key={index} className="radio">
                        <label>
                            <input
                                type="checkbox"
                                name={ question.name }
                                id={ question.id }
                                value={ value }
                                onChange={ this.handleChange.bind(this)}
                                checked={  this.state.values.indexOf(value) > -1 }
                                />
                            { value }
                        </label>
                    </div>
                }) }
            </div>
        );
    }
}

CheckboxGroup.defaultProps = {
    placeholder: null,
};

CheckboxGroup.propTypes = {
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

export default CheckboxGroup;
