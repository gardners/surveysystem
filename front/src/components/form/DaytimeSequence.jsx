import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';

import { parseDayTime, formatDayTimeDiff, formatDayTime } from '../../Utils';
import { matchesBreakpoint } from '../../Media';

import InputRange from 'react-input-range';
import { DaytimeInput } from './TimePicker';
import { Fade } from '../Transitions';

import './DayTimeSlider.scss';

/**
 * @var {string} MEDIA_BREAKPOINT bootstrap media query breaKpoint who triggers a d=single <Select> column instead separate columns of radio buttons
 */

const MEDIA_BREAKPOINT = 'md';

/**
 * DaytimeIcon
 */

const DaytimeIcon = function({ seconds, style }) {

    const dt = parseDayTime(seconds);
    const icon = (dt.hours24 < 8 || dt.hours24 > 18) ? 'moon' : 'sun';

    return (<i className = { `fas fa-${icon}` } style={ style } />);
};

DaytimeIcon.defaultProps = {
    style: {},
};

DaytimeIcon.propTypes = {
    seconds: PropTypes.number.isRequired,
    style: PropTypes.object,
};

/**
 * DaytimeLabel
 */

const DaytimeLabel = function({ seconds }) {
    return (
        <span className="daytimeslider--label">
            <span className="inner tick" />
            <span className="inner text">
                { formatDayTime(seconds) }
            </span>
        </span>
    );
};

DaytimeLabel.defaultProps = {
    style: {},
};

DaytimeLabel.propTypes = {
    seconds: PropTypes.number.isRequired,
    style: PropTypes.object,
};

/**
 * Inputrange Gutter
 */

const computeSteps = function(min, max, period) {
    const ticks = [];

    let t = min;
    while (t < max) {
       ticks.push(t);
       t += period;
    }
    ticks.push(max);
    return ticks;
};

const Gutter = function({ min, max, className, component }) {
    const steps = computeSteps(min, max, 6 * 3600);
    const { length } = steps;
    const Component = component;
    return (
        <div className={ className }>
            <div style={ { position: 'relative', width: '100%' } }>
                <div style={ { position: 'absolute', width: '100%' } }>
                {
                    steps.map((step, index) => {
                        const style = {
                            position: 'absolute',
                            left: ((index / (length - 1)) * 100) + '%' ,
                        };
                        return (
                            <span key={ index } style={ style } ><Component seconds={ step } style={ { marginLeft: '-50%' } } /></span>
                        );
                    })
                }
                </div>
            </div>
        </div>
    );
};

Gutter.defaultProps = {
    step: 6 * 3600, // 6 hours
};

Gutter.propTypes = {
    min: PropTypes.number.isRequired,
    max: PropTypes.number.isRequired,
    className: PropTypes.string,
    component: PropTypes.func.isRequired,
    step: PropTypes.number,
};

/**
 * ListItem
 */

const ListItem = function({ index, choice, value, handleProgress, expanded, touched, active, children }) {

    return (
        <div className="list-group-item">
            {
                (index <= touched || active) ?
                    <div className="row">
                        <div className="col-md-4">{ choice }</div>
                        <div className="col-md-4 text-center">
                            <strong className="text-primary" >{ formatDayTime(value) }</strong>
                        </div>
                        <div className="col-md-4 text-right">
                            <small className="text-muted">{ formatDayTimeDiff(minValue(minValue, value), value) } <DaytimeIcon seconds={ value } /></small>
                        </div>
                    </div>
                :
                    <div className="row">
                        <div className="col-md-4">{ choice }</div>
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
    choice: PropTypes.string.isRequired,
    value: PropTypes.number.isRequired,
    minValue: PropTypes.number.isRequired,
    handleProgress: PropTypes.func.isRequired,
    touched: PropTypes.number.isRequired,
    active: PropTypes.bool.isRequired,
    expanded: PropTypes.bool.isRequired,
};

/**
 * DaytimeSequence
 */

const minValue = function(index, values) {
    return (index > 0) ? values[index - 1] : 0;
};

const maxValue = function(index, values) {
    return minValue(index, values) + (24 * 3600);// todo "period" prop
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

class DaytimeSequence extends Component {

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
        const { question } = this.props;
        const { choices } = question;

        const defaultValues = question.default_value.split(',');

        const values = choices.map((q, index) => {
            let val = 0;
            if (typeof defaultValues[index] !== 'undefined') {
                const num = Number(defaultValues[index]);
                val = (!isNaN(num)) ? num : 0;
            }
            return val;
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
     * Handler for submit (confirmation) button. Submits answer to survey
     * @param {Number} index values index to update
     * @param {Number} value
     * @param {Element} e
     *
     * @returns {void}
     */
    handleSubmit(index, value, e) {
        e && e.preventDefault();

        const { question, handleChange } = this.props;
        const { values } = this.state;

        handleChange(null, question, values);
    }

    /**
     * Handler for next button. Progresses this form to the next choice
     * @param {Number} index values index to update
     * @param {Element} e
     *
     * @returns {void}
     */
    handleNext(index, e) {
        e && e.preventDefault();

        let { current, touched } = this.state;

        if (current < this.state.values.length - 1) {
            current += 1;
        }

        this.setState({
            current,
            touched: (index > touched) ? index : touched,
        });
    }

    /**
     * Handler for prev button. Returns this form to the previous choice
     * @param {Element} e
     *
     * @returns {void}
     */
    handlePrev(e) {
        e && e.preventDefault();

        let { current } = this.state;

        if (current > 0) {
            current -= 1;
        }

        this.setState({
            current,
        });
    }

    render() {
        const { question, error, required, grouped, className, expand, step } = this.props;
        const { values, current, touched } = this.state;

        if (!values.length) {
            return (null);
        }

        const expanded = (expand === null) ? matchesBreakpoint(MEDIA_BREAKPOINT) : expand;

        return (
            <Field.Row className={ className } question={ question } grouped={ grouped } required={ required }>
                <Field.Description question={ question } grouped={ grouped } required={ required } />
                <Field.Title element="label" grouped={ grouped } question={ question } required={ required }>
                    <Field.Unit className="badge badge-secondary ml-1" question={ question } grouped={ grouped } />
                </Field.Title>

                <div className="list-group">
                    {
                        question.choices.map((choice, index) =>
                            (index === current) ?
                                <ListItem
                                    key={ index }

                                    index={ index }
                                    choice={ choice }
                                    handleProgress={ () => this.handleProgress(1) }
                                    value={ values[index] }
                                    minValue={ minValue(index, values) }
                                    touched={ touched }
                                    active={ true }
                                    expanded={ expanded }
                                >
                                    <Fade key={ index } timeout={ 250 }>
                                        <div className="row p-5">
                                        {
                                            (expanded) ?
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
                                                    seconds={ values[index] }
                                                    handleSubmit={ this.handleSubmit.bind(this, index) }
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
                                                            onClick={ this.handleSubmit.bind(this) }
                                                        >
                                                            <i className="fas fa-check" /> Submit
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
                                    choice={ choice }
                                    handleProgress={ this.handlePrev.bind(this) }
                                    value={ values[index] }
                                    minValue={ minValue(index, values) }
                                    expanded={ expanded }
                                    active= { false }
                                    touched={ touched }
                                />
                        )
                    }

                </div>
                <Field.Error error={ error } grouped={ grouped } />
            </Field.Row>
        );
    }
};

DaytimeSequence.defaultProps = {
    grouped: false,
    required: false,

    expand: null, // null!

    // react-input-range props
    step:  15 * 60,
};

DaytimeSequence.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    value: QuestionModel.valuePropTypes(),
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    required: PropTypes.bool,

    className: PropTypes.string,
    expand: PropTypes.bool, // force contracted or expanded display

    // react-input-range props
    step: PropTypes.number,
};

export { DaytimeSequence as default, DaytimeIcon, DaytimeLabel, Gutter };
