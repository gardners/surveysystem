import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';


class Checkbox extends Component {

    constructor(props) {
        super(props);

        this.state = {
            checked: false,
        };
    }

    componentDidMount() {
        const { question, handleChange } = this.props;
        const { default_value, choices } = question;

        // two defined choices in the following order: [OFF-value, ON-value]
        const checked = default_value && choices.indexOf(default_value) === 1;
        this.setState({
            checked,
        });

        // !immediately invoke answer callback, toggle question types must setting answer values on init, regardless if a default_value was set
        handleChange(null, question, (checked) ? choices[1] : choices[0]);
    }

    handleChange() {
        const { question, handleChange } = this.props;
        const { choices } = question;
        const checked = !this.state.checked;
        this.setState({
            checked,
        });

        // two defined choices in the following order: [OFF-value, ON-value]
        handleChange(null, question, (checked) ? choices[1] : choices[0]);
    }

    render() {
        const { checked  } = this.state;
        const { question, error, /*required,*/ grouped, className } = this.props;

        // # 224
        const required = false;

        return (
            <Field.Row className={ className } question={ question } grouped={ grouped } required={ required }>
                <Field.Description question={ question } grouped={ grouped } required={ required } />
                <div className="form-check">
                    <button
                        id={ question.id}
                        name={ question.name }
                        className={ (checked) ? 'btn' : 'btn' }
                        onClick={
                            (e) => {
                                e.preventDefault();
                                this.handleChange();
                            }
                        }
                    >
                        { (checked) ? <i className="mr-2 fas fa-check-square text-primary" /> : <i className="mr-2 far fa-square text-muted" /> }
                        <Field.Title display="span" grouped={ grouped } question={ question } required={ required }>
                            <Field.Unit className="badge badge-secondary ml-1" question={ question } grouped={ grouped } />
                        </Field.Title>
                    </button>
                </div>
                <Field.Error error={ error } grouped={ grouped } />
            </Field.Row>
        );
    }
};

Checkbox.defaultProps = {
    grouped: false,
    required: false,
};

Checkbox.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    value: QuestionModel.valuePropTypes(),
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    required: PropTypes.bool,

    className: PropTypes.string,
};

export default Checkbox;
