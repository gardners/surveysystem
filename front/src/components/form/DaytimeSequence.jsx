import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';

import { prettyHours, DaySeconds } from '../../Utils';
import { matchesBreakpoint } from '../../Media';

import InputRange from 'react-input-range';
import TimePicker from './TimePicker';

import './DaytimeSequence.scss';

const tableStyle = {
    display: 'table',
    width: '100%',
    padding: '.5em',
    tableLayout: 'fixed',    /* For cells of equal size */
};

const cellStyle = function(percent) {
    const textAlign = ((percent === 50) ? 'center' : (percent < 50) ? 'left' : 'right');
    return {
        width: '25%',
        display: 'table-cell',
        textAlign,
    };
};

/**
 * @var {string} MEDIA_BREAKPOINT bootstrap media query breaKpoint who triggers a d=single <Select> column instead separate columns of radio buttons
 */
const MEDIA_BREAKPOINT = 'md';


const timePickerDisabled = function(seconds, scope) {
    if (!seconds) {
        return [];
    }

    const r = [];
    switch (scope) {
        case 'h':
            const h = Math.floor(seconds / 3600);
            for (let i = 0; i < h; i++) {
                r.push(i);
            }
        break;
        case 'm':
            const m = Math.floor(seconds % 3600 / 60);
            for (let i = 0; i < m; i++) {
                r.push(i);
            }
        break;
        case 's':
            const s = Math.floor(seconds % 3600 % 60);
            for (let i = 0; i < s; i++) {
                r.push(i);
            }
        break;
        default:
            // nothing
    }

    return r;
};


const TheadRow = function({ question, expanded }) {

    if(!expanded) {
        return(null);
    }
    return (
        <tr>
            <th className="daytimeslider--firstcol"></th>
            <th className="daytimeslider--secondcol"></th>
            <th>
                <div style={ tableStyle }>
                    <div style={ cellStyle(0) }><i className="fas fa-moon"></i></div>
                    <div style={ cellStyle(50) }><i className="fas fa-sun"></i></div>
                    <div style={ cellStyle(100) }><i className="fas fa-moon"></i></div>
                </div>
            </th>
        </tr>
    );
};

TheadRow.defaultProps = {
};

TheadRow.propTypes = {
    question: QuestionModel.propTypes().isRequired,
    expanded: PropTypes.bool.isRequired,
};

const Row = function({ question, handleChange, value, disabled, minValue, expanded, step }) {

        if(!expanded) {
            return (
                <tr>
                    <td className="daytimeslider--firstcol">{ question.title }</td>
                    <td>
                        <TimePicker
                            key={ question.id }
                            question={ question }
                            handleChange={ handleChange }
                            disabledHours= { timePickerDisabled(value, 'h') }
                            disabledMinutes= { timePickerDisabled(value, 'm') }
                            disabledHSeconds= { timePickerDisabled(value, 's') }
                        />
                    </td>
                </tr>
            );
        }

        return (
            <tr>
                <td className="daytimeslider--firstcol">{ question.title }</td>
                <td className="daytimeslider--secondcol">{ prettyHours(value) }</td>
                <td>
                    <InputRange
                        minValue={ minValue }
                        maxValue={ DaySeconds }
                        value={ value }
                        onChange={ handleChange }
                        disabled= { disabled }
                        step={ step }
                        formatLabel={value => prettyHours(value)}
                    />
                </td>
            </tr>
        );
};

Row.defaultProps = {
    required: false,
    step:  15 * 60,
    minValue: 0,
    disabled: false,
};

Row.propTypes = {
    question: QuestionModel.propTypes().isRequired,
    handleChange: PropTypes.func.isRequired,
    expanded: PropTypes.bool.isRequired,
    required: PropTypes.bool,

    // react-input-range props
    value: PropTypes.number.isRequired,
    minValue: PropTypes.number,
    disabled: PropTypes.bool,
    step: PropTypes.number,
};

const prevValue = function(index, values) {
    if(index === 0 ) {
        return null;
    }
    return values[index - 1];
};

const setValues = function(value, index, values) {
    const length = values.length;
    const prev = prevValue(index, values);
    // set current value
    values[index] = (prev && value < prev) ? prev: value;
    for (let i = index; i < length; i += 1) {
        if(values[i] < value) {
            values[i] = value;
        }
    }
    return values;
}

class DaytimeSequence extends Component {

    constructor(props) {
        super(props);

        this.state = {
            values: props.questions.map(() => 0),
        };
    }

    componentDidMount() {
        const { questions } = this.props;
        const values = questions.map((q) => {
            const num = Number(q.default_value);
            return (!isNaN(num)) ? num : 0;
        });

        this.setState({
            values
        });
    }

    handleChange(value, question, index){
        const values = setValues(value, index, this.state.values);
        this.setState({
            values
        });

        this.props.handleChange(null, question, value);
    }

    render() {
        const { questions, errors, required, grouped, className, expand } = this.props;

        if(!questions.length) {
            return (null);
        }

        const expanded = (expand === null) ? matchesBreakpoint(MEDIA_BREAKPOINT) : expand;
        const first = questions[0];

        return (
            <Field.Row className={ className } question={ first } grouped={ grouped } required={ required }>
                { /* No Field.title */ }
                <Field.Description question={ first } grouped={ grouped } required={ required }>
                    <Field.Unit className="badge badge-secondary ml-1" question={ first } grouped={ grouped } />
                </Field.Description>

                <div className="table-responsive daytimeslider">
                    <table className="table table-sm table-hover daytimeslider--table">
                        <thead>
                            <TheadRow
                                question={ first }
                                expanded={ expanded }
                            />
                        </thead>
                        <tbody>
                            {
                                questions.map((question, index) => {
                                    const prev = prevValue(index, this.state.values);
                                    return (
                                        <Row
                                            key={ index }
                                            question={ question }
                                            handleChange={ (value) => this.handleChange(value, question, index) }
                                            value={ this.state.values[index] }
                                            // minValue={ minValue }
                                            disabled={ index > 0 && !prev }
                                            required={ required }
                                            expanded={ expanded }
                                        />
                                    );
                                })
                            }
                        </tbody>
                    </table>
                </div>

                { Object.keys(errors).map(id => <Field.Error key= { id } error={ errors[id] } grouped={ grouped } />) }
            </Field.Row>
        );
    }
};

DaytimeSequence.defaultProps = {
    grouped: false,
    required: false,

    expand: null, // null!
};

DaytimeSequence.propTypes = {
    handleChange: PropTypes.func.isRequired,
    questions: PropTypes.arrayOf(
        QuestionModel.propTypes(),
    ).isRequired,

    errors: PropTypes.object.isRequired,
    grouped: PropTypes.bool,
    required: PropTypes.bool,

    className: PropTypes.string,
    expand: PropTypes.bool, // force contracted or expanded display
};

export default DaytimeSequence;
