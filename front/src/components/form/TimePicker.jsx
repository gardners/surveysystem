import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';
import { parseDayTime, setDaytimeDate } from '../../Utils';

import './TimePicker.scss';

/**
 * Group of inputs to set a date time value in seconds
 * - exported as a small screen option for Daytime slider components
 */

class DaytimeInput extends Component  {

    constructor(props) {
        super(props);
        this.state = {
            hours: 0,
            minutes: 0,
            ampm: 'am',
            errors: {},
        };
    }

    componentDidMount() {
        const parsed = parseDayTime(this.props.value);
        this.setState({
            hours: parsed.hours,
            minutes: parsed.mins,
            ampm: parsed.ampm,
            errors: {},
        });
    }

    componentDidUpdate(prevProps) {
        if (prevProps.value !== this.props.value) {
            const parsed = parseDayTime(this.props.value);
            this.setState({
                hours: parsed.hours,
                minutes: parsed.mins,
                ampm: parsed.ampm,
                errors: {},
            });
        }
    }

    checkError(element, stateProp) {
        const { errors } = this.state;
        delete(errors[stateProp]);
        // @see https://developer.mozilla.org/en-US/docs/Web/API/ValidityState
        if(element && typeof element.validity !== 'undefined') {
            if (!element.validity.valid) {
                errors[stateProp] = new Error(element.validationMessage);
                return errors;
            }
        }
        return errors;
    }

    render () {
        const { handleSubmit, namespace } = this.props;
        const { hours, minutes, ampm, errors } = this.state;

        const hasErrors = Object.keys(errors).length !== 0;

        return (
            <div className="row">
                <div className="col-md-3 mb-1">
                    <input
                        id={ `${namespace}--hours` }
                        name={ `${namespace}--hours` }
                        type="number"
                        placeholder="hours"
                        aria-label="hours"
                        className="form-control"
                        min={ 0 }
                        max={ 12 }
                        step={ 1 }
                        value={ hours }
                        onChange={ (e) => {
                            const { value } = e.target;
                            this.setState({
                                hours: parseInt(value, 10),
                                errors: this.checkError(e.target, 'hours'),
                            });
                        } }
                    />
                    { errors.hours && <div className="text-danger"><small>{ errors.hours.toString() }</small></div> }
                </div>

                <div className="col-md-3 mb-1">
                    <input
                        id={ `${namespace}--minutes` }
                        name={ `${namespace}--minutes` }
                        type="number"
                        placeholder="minutes"
                        aria-label="minutes"
                        className="form-control"
                        min={ 0 }
                        max={ 60 }
                        step={ 5 }
                        value={ minutes }
                        onChange={ (e) => {
                            const { value } = e.target;
                            this.setState({
                                minutes: parseInt(value, 10),
                                errors: this.checkError(e.target, 'minutes'),
                            });
                        } }
                    />
                    { errors.minutes && <div className="text-danger"><small>{ errors.minutes.toString() }</small></div> }
                </div>

                <div className="col-md-2 mb-1">
                    <select
                        id={ `${namespace}--minutes` }
                        name={ `${namespace}--minutes` }
                        value={ ampm }
                        className="form-control"
                        onChange={ (e) => {
                            e.preventDefault();
                            const { value } = e.target;
                            this.setState({ ampm: value });
                        } }
                    >
                        <option value="am">am</option>
                        <option value="pm">pm</option>
                    </select>
                </div>

                <div className="col-md-4">
                    <button
                        className="btn btn-secondary"
                        disabled = { hasErrors }
                        onClick={ (e) => {
                            e.preventDefault();
                            const date = setDaytimeDate(hours, minutes, ampm);
                            const ts = date.getTime();
                            handleSubmit((ts > 0) ? ts / 1000 : 0);
                        } }
                    >OK</button>
                </div>
            </div>
        );
    }
};

DaytimeInput.defaultProps = {
    value: 0,
    minuteStep: 5,
};

DaytimeInput.propTypes = {
    value: PropTypes.number,
    namespace: PropTypes.string.isRequired,
    handleSubmit: PropTypes.func.isRequired,
    minuteStep: PropTypes.number,
};

/**
 * Timepicker question component
 */

class TimePicker extends Component {
    constructor(props) {
        super(props);
        this.state = {
            value: 0,
        };
    }

    componentDidMount() {
        const { question } = this.props;

        let val = Number(question.default_value);

        this.setState({
            value: (!isNaN(val)) ? val : 0,
        });
    }

    handleChange(value) {
        const { question, handleChange } = this.props;

        this.setState({
            value,
        });
        handleChange(null, question, value);
    }

    render() {
        const { value } = this.state;
        const { question, error, required, grouped, className, minuteStep } = this.props;

        return (
            <Field.Row className={ className } question={ question } grouped={ grouped } required={ required }>
                <Field.Description question={ question } grouped={ grouped } required={ required } />
                <Field.Title element="label" grouped={ grouped } question={ question } required={ required }>
                    <Field.Unit className="badge badge-secondary ml-1" question={ question } grouped={ grouped } />
                </Field.Title>

                <DaytimeInput
                    value={ value }
                    handleSubmit={ this.handleChange.bind(this) }
                    namespace={ question.id }
                    minuteStep={ minuteStep }
                />

                <Field.Error error={ error } grouped={ grouped } />
            </Field.Row>
        );
    }
};

TimePicker.defaultProps = {
    grouped: false,
    required: false,
    minuteStep: 5,
};

TimePicker.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    required: PropTypes.bool,

    className: PropTypes.string,
    minuteStep: PropTypes.number,
};

export { TimePicker as default, DaytimeInput };
