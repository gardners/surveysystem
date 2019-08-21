import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';

import { formatDayTimeDiff, formatDayTime } from '../../Utils';
import { AppContext } from '../../Context';

import { Gutter, DaytimeIcon, DaytimeLabel } from './DaytimeSequence';

import InputRange from 'react-input-range';
import { DaytimeInput } from './TimePicker';
import { Fade } from '../Transitions';

import './DayTimeSlider.scss';

/**
 * ListItem
 */

const ListItem = function({ index, question, value, minValue, handleProgress, touched, active, children }) {

    return (
        <div className="list-group-item">
            {
                (index <= touched || active) ?
                    <div className="row">
                        <div className="col-md-4">{ question.title }</div>
                        <div className="col-md-4 text-center">
                            <strong className="text-primary" >{ formatDayTime(value) }</strong>
                        </div>
                        <div className="col-md-4 text-right">
                        {
                            (index === 0) ?
                                <small className="text-muted"><DaytimeIcon value={ value } /></small>
                            :
                                <small className="text-muted">{ formatDayTimeDiff(minValue, value) } <DaytimeIcon value={ value } /></small>
                        }
                        </div>
                    </div>
                :
                    <div className="row">
                        <div className="col-md-4">{ question.title }</div>
                    </div>
            }
            <div className="row">
                <div className="col">
                    { children }
                </div>
            </div>
        </div>
    );
};

ListItem.propTypes = {
    index: PropTypes.number.isRequired,
    question: QuestionModel.propTypes().isRequired,
    value: PropTypes.number.isRequired,
    minValue: PropTypes.number.isRequired,
    handleProgress: PropTypes.func.isRequired,
    touched: PropTypes.number.isRequired,
    active: PropTypes.bool.isRequired,
};

/**
 * DaytimeSequence
 */

const minValue = function(index, values) {
    return (index > 0) ? values[index - 1] : 0;
};

const maxValue = function(index, values) {
    return minValue(index, values) + (24 * 3600);
};

/*
 * Update values array beginning from the given index,
 * Ensure that following array values are >= the given value
 * @param {Number} index
 * @param {Number} value the value returned by InputRange drag event
 * @param {Number[]} values see DaytimeSequence => state.values
 *
 * @returns {Number[]}
 */
const setValues = function(index, value, values) {
    const length = values.length;
    const prev = minValue(index, values);
    // set current value
    values[index] = (prev && value < prev) ? prev : value;
    for (let i = index; i < length; i += 1) {
        if(values[i] < value) {
            values[i] = value;
        }
    }
    return values;
};

/*
 * Update values array beginning from the given index,
 * Ensure that following array reset to the given value
 * @param {Number} index
 * @param {Number} value the value returned by InputRange drag event
 * @param {Number[]} values see DaytimeSequence => state.values
 *
 * @returns {Number[]}
 */
const setValuesNext = function(index, value, values) {
    const length = values.length;
    const prev = minValue(index, values);
    // set current value
    values[index] = (prev && value < prev) ? prev : value;
    for (let i = index; i < length; i += 1) {
        values[i] = value;
    }
    return values;
};

class DaytimeQuestionGroup extends Component {

    constructor(props) {
        super(props);

        this.state = {
            current: 0,
            values: [],
            touched: 0,
        };
    }

    /**
     * Map optional default values to state values
     *
     * @returns {void}
     */
    componentDidMount() {
        const { questions } = this.props;

        const values = questions.map((q) => {
            const num = Number(q.default_value);
            return (!isNaN(num)) ? num : 0;
        });

        this.setState({
            values,
        });
    }

    /**
     * Handler for InputRange drag event, updates state values
     * @param {Number} index values index to update
     * @param {Number} value
     *
     * @returns {void}
     */
    handleChange(index, value) {
        let { values, touched } = this.state;

        values = setValues(index, value, values);

        this.setState({
            values,
            touched: (index > touched) ? index : touched,
        });
    }

    /**
     * Handler for next button.
     * Submits answer to survey.
     * Progresses this form to the next choice
     * @param {Number} index values index to update
     * @param {Element} e
     *
     * @returns {void}
     */
    handleNext(index, e) {
        e && e.preventDefault();

        const { questions, handleChange } = this.props;
        let { values, current, touched } = this.state;

        if (current < this.state.values.length - 1) {
            current += 1;
        }

        values = setValuesNext(index, values[index], values);

        this.setState({
            values,
            current,
            touched: (index >= touched) ? index : touched,
        });

        handleChange(null, questions[index], values[index]);
    }

    /**
     * Handler for prev button. Returns this form to the previous choice
     * @param {Element} e
     *
     * @returns {void}
     */
    handlePrev(index, e) {
        e && e.preventDefault();

        let { current, values } = this.state;

        if (current > 0) {
            current -= 1;
        }

        values = setValues(index, 0, values);

        this.setState({
            values,
            current,
            touched: current,
        });
    }

    render() {
        const { questions, errors, required, grouped, className, step } = this.props;
        const { values, current, touched } = this.state;

        if (!values.length) {
            return (null);
        }

        const first = questions[0];

        return (
            <Field.Row className={ className } question={ first } grouped={ grouped } required={ required }>
                { /* No Field.title */ }
                <Field.Description question={ first } grouped={ grouped } required={ required }>
                    <Field.Unit className="badge badge-secondary ml-1" question={ first } grouped={ grouped } />
                </Field.Description>

                <AppContext.Consumer>
                {
                    ({ breakpoint, isBreakpointAbove }) => (
                        <div className="list-group">
                            {
                                questions.map((question, index) =>
                                    (index === current) ?
                                        <ListItem
                                            key={ index }

                                            index={ index }
                                            question={ question }
                                            handleProgress={ () => this.handleProgress(1) }
                                            value={ values[index] }
                                            minValue={ minValue(index, values) }
                                            touched={ touched }
                                            active={ true }
                                        >
                                            <Fade key={ index } timeout={ 250 }>
                                                <div className="row p-5">
                                                {
                                                    (isBreakpointAbove('md')) ?
                                                        <div className="col daytime-slider">
                                                            <Gutter className="mb-4" component={ DaytimeIcon } min={ minValue(index, values) } max={ maxValue(index, values) } />
                                                            <InputRange
                                                                minValue={ minValue(index, values) }
                                                                maxValue={ maxValue(index, values) }
                                                                value={ values[index] }
                                                                onChange={ this.handleChange.bind(this, index) }

                                                                step={ step }
                                                                formatLabel={ val => formatDayTime(val) }
                                                            />
                                                            <Gutter className="mt-2" component={ DaytimeLabel } min={ minValue(index, values) } max={ maxValue(index, values) } />
                                                        </div>
                                                    :
                                                        <DaytimeInput
                                                            namespace={ question.id }
                                                            value={ values[index] }
                                                            handleSubmit={ this.handleChange.bind(this, index) }
                                                        />
                                                }
                                                </div>

                                                <div className="row">
                                                    <div className="col text-center">
                                                        {
                                                            index > 0 &&
                                                                <button
                                                                    className="btn btn-outline-secondary btn-sm"
                                                                    onClick={ this.handlePrev.bind(this, index) }
                                                                >
                                                                    <i className="fas fa-angle-left" /> Back
                                                                </button>
                                                        }
                                                        {
                                                            index < values.length - 1 &&
                                                                <button
                                                                    className="btn btn-outline-primary btn-sm ml-4"
                                                                    onClick={ this.handleNext.bind(this, index) }
                                                                >
                                                                    Next <i className="fas fa-angle-right" />
                                                                </button>
                                                        }
                                                        {
                                                            index === values.length - 1 &&
                                                                <button
                                                                    className="btn btn-primary btn-sm ml-4"
                                                                    onClick={ this.handleNext.bind(this, index) } // for the case the handle was not moved (default_value given), otherwise the survey would display the incomplete warning and not proceed
                                                                >
                                                                    <i className="fas fa-check" /> OK
                                                                </button>
                                                        }
                                                    </div>
                                                </div>
                                            </Fade>
                                        </ListItem>
                                    :
                                        <ListItem
                                            key={ index }

                                            index={ index }
                                            question={ question }
                                            handleProgress={ this.handlePrev.bind(this) }
                                            value={ values[index] }
                                            minValue={ minValue(index, values) }
                                            active= { false }
                                            touched={ touched }
                                        />
                                )
                            }
                        </div>
                    )
                }
                </AppContext.Consumer>

                { Object.keys(errors).map(id => <Field.Error key= { id } error={ errors[id] } grouped={ grouped } />) }
            </Field.Row>
        );
    }
};

DaytimeQuestionGroup.defaultProps = {
    grouped: false,
    required: false,

    // react-input-range props
    step:  15 * 60,
};

DaytimeQuestionGroup.propTypes = {
    handleChange: PropTypes.func.isRequired,
    questions: PropTypes.arrayOf(
        QuestionModel.propTypes(),
    ).isRequired,

    value: QuestionModel.valuePropTypes(),
    errors: PropTypes.object.isRequired,
    grouped: PropTypes.bool,
    required: PropTypes.bool,

    className: PropTypes.string,

    // react-input-range props
    step: PropTypes.number,
};

export default DaytimeQuestionGroup;
