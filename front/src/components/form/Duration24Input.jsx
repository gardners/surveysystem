import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';

import { DaySeconds } from '../../Utils';

const minuteOpts = [0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60];
const hourOpts = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24];

class Duration24Input extends Component {

    constructor(props) {
        super(props);

        this.state = {
            value: 0,
            minValue: 0,
            maxvalue:  DaySeconds,
        };
    }

    componentDidMount() {
        const { question } = this.props;
        const val = Number(question.default_value);
        const minval = Number(question.min_value);
        const maxval = Number(question.max_value);


        this.setState({
            value: (!isNaN(val)) ? val : 0,
            minValue: (!isNaN(minval)) ? minval : 0,
            maxValue: (!isNaN(maxval) && (maxval > minval && maxval <= DaySeconds)) ? maxval : DaySeconds,
        });
    }

    handleChange(unit, e){
        const { question, handleChange } = this.props;
        let value = parseInt(e.target.value, 10);

        if (unit === 'hours') {
            const minutes = Math.floor(this.state.value % 3600 / 60);
            value = (value * 3600) + (minutes * 60);
        } else {
            const hours = Math.floor(this.state.value / 3600);
            value = (hours * 3600) + (value * 60);
        }

        if (value > this.state.maxValue) {
            value = this.state.maxValue;
        }

        if (value < this.state.minValue) {
            value = this.state.minValue;
        }

        this.setState({
            value,
        });

        handleChange(e.target, question, value);
    }

    render() {
        const { value } = this.state;
        const { question, error, required, grouped, className } = this.props;

        const hours = Math.floor(value / 3600);
        const minutes = Math.floor(value % 3600 / 60);

        return (
            <Field.Row className={ className } question={ question } grouped={ grouped } required={ required }>
                <Field.Description question={ question } grouped={ grouped } required={ required } />
                <Field.Title element="label" grouped={ grouped } question={ question } required={ required }>
                    <Field.Unit className="badge badge-secondary ml-1" question={ question } grouped={ grouped } />
                </Field.Title>

                <div className="form-inline">
                    <div className="input-group mb-1 mr-3">
                        <select
                            multiple={ false }
                            className="custom-select"
                            autoComplete="off"
                            name={ `${question.name}--hours` }
                            id={ `${question.id}--hours` }
                            value={ hours }
                            onChange={ this.handleChange.bind(this, 'hours')}
                        >
                        { hourOpts.map(v => <option key={ v } value={ v }>{ v }</option>) }
                        </select>
                        <div className="input-group-append">
                            <label className="input-group-text" htmlFor={ `${question.id}--hours`}>hours</label>
                        </div>
                    </div>

                    <div className="input-group mb-1">
                        <select
                            multiple={ false }
                            className="custom-select"
                            autoComplete="off"
                            name={ `${question.name}--minutes` }
                            id={ `${question.id}--minutes` }
                            value={ minutes }
                            onChange={ this.handleChange.bind(this, 'minutes')}
                            disabled={ hours >= 24 }
                        >
                        { minuteOpts.map(v => <option key={ v } value={ v }>{ v }</option>) }
                        </select>
                        <div className="input-group-append">
                            <label className="input-group-text" htmlFor={ `${question.id}--minutes`}>minutes</label>
                        </div>
                    </div>
                </div>

                <Field.Error error={ error } grouped={ grouped } />
            </Field.Row>
        );
    }
};

Duration24Input.defaultProps = {
    grouped: false,
    required: false,

    step: 1,
};

Duration24Input.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    value: QuestionModel.valuePropTypes(),
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    required: PropTypes.bool,

    step: PropTypes.number,
    className: PropTypes.string,
};

export default Duration24Input;
