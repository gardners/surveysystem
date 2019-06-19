import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';

import { isArray } from '../../Utils';

class CheckboxGroup extends Component {
    constructor(props) {
        super(props);
        this.state = {
            values: [],
        };
    }

    componentDidMount() {
        const { question } = this.props;
        const values = (isArray(question.default_value)) ? question.default_value : question.default_value.split(',');
        this.setState({
            values,
        });
    }

    handleChange(question, value) {
        const { handleChange } = this.props;
        let { values } = this.state;

        if(values.indexOf(value) > -1) {
            values = values.filter(v => v !== value);
        } else {
            values.push(value);
        }

        this.setState({
            values,
        });

        handleChange(null, question, values);
    }

    render() {
        const { question, error, required, grouped, className } = this.props;
        const { choices } = question;
        const { values } = this.state;

        return (
            <Field.Row className={ className } question={ question } grouped={ grouped } required={ required }>
                <Field.Description question={ question } grouped={ grouped } required={ required } />
                <Field.Title element="label" grouped={ grouped } question={ question } required={ required }>
                    <Field.Unit className="badge badge-secondary ml-1" question={ question } grouped={ grouped } />
                </Field.Title>
                <div className="list-group">
                {
                    choices.map((choice, index) => {
                        const checked = (values.indexOf(choice) > -1);
                        return (
                            <button
                                key={ index }
                                id={ `${question.id}[${index}]` }
                                name={ question.name }
                                className={ (checked) ? 'text-left list-group-item list-group-item-primary' : 'text-left list-group-item' }
                                value={ choice }
                                onClick={
                                    //
                                    (e) => {
                                        e.preventDefault();
                                        this.handleChange(question, choice);
                                    }
                                }
                            >
                                { (checked) ? <i className="mr-2 fas fa-check-square text-primary" /> : <i className="mr-2 far fa-square text-muted" /> }
                                { choice }
                            </button>
                        );
                    })
                }
                </div>
                <Field.Error error={ error } grouped={ grouped } />
            </Field.Row>
        );
    }
}

CheckboxGroup.defaultProps = {
    grouped: false,
    required: false,
};

CheckboxGroup.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    required: PropTypes.bool,
};

export default CheckboxGroup;
