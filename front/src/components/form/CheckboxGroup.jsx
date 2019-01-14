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

        this.props.handleChange(e.target, question, values);
    }

    render() {
        const { question, required } = this.props;
        const { choices } = question;

        return (
            <div className="form-group">
                <label> { question.title } { question.unit && <em>({ question.unit })</em> }</label><br />
                { choices.map((value, index) => {
                    return <div key={index} className="radio form-check form-check-inline">
                        <input
                            type="checkbox"
                            className="form-check-input"
                            name={ question.name }
                            id={ question.id }
                            value={ value }
                            onChange={ this.handleChange.bind(this)}
                            checked={  this.state.values.indexOf(value) > -1 }
                            />
                        <label className="form-check-label">{ value }</label>
                    </div>
                }) }
            </div>
        );
    }
}

CheckboxGroup.defaultProps = {
    required: true,
};

CheckboxGroup.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: PropTypes.shape({
        id: PropTypes.string.isRequired,
        name: PropTypes.string.isRequired,
        title: PropTypes.string.isRequired,
        title_text: PropTypes.string.isRequired,
        type: PropTypes.string.isRequired,
        // eunum
        choices: PropTypes.array.isRequired,
    }).isRequired,
    required: PropTypes.bool,
};

export default CheckboxGroup;
