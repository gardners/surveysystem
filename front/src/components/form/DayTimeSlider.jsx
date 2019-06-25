import React, { Component } from 'react';
import PropTypes from 'prop-types';

import InputRange from 'react-input-range';


import Field from './Field';
import QuestionModel from '../../Question';

import { formatDayTime, DaySeconds } from '../../Utils';

// bg gradient
const wrapperStyle = {};

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
 * range sliders for defining period secondss (seconds) within 24 hours
 */

class DayTimeSlider extends Component {

    constructor(props) {
        super(props);

        this.state = {
            value: 0,
            minValue: 0,
            maxValue: DaySeconds,
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
            maxValue: (!isNaN(maxval) && maxval > minval) ? maxval : DaySeconds,
        });
    }

    handleChange(value){
        const { question, handleChange } = this.props;
        this.setState({
            value,
        });
        handleChange(null, question, value);
    }

    render() {
        const { value, minValue, maxValue } = this.state;
        const { question, error, required, grouped, className, handleChangeComplete, withIcons, step } = this.props;

        return (
            <Field.Row className={ className } question={ question } grouped={ grouped } required={ required }>
                <Field.Title display="" grouped={ grouped } question={ question } required={ required }>
                    <Field.Unit className="badge badge-secondary ml-1" question={ question } grouped={ grouped } />
                </Field.Title>
                <Field.Description question={ question } grouped={ grouped } required={ required } />

                <div style={ wrapperStyle }>
                    {
                        withIcons &&
                        <div className={ className } style={ tableStyle }>
                            <div style={ cellStyle(0) }><i className="fas fa-moon"></i></div>
                            <div style={ cellStyle(50) }><i className="fas fa-sun"></i></div>
                            <div style={ cellStyle(100) }><i className="fas fa-moon"></i></div>
                        </div>
                    }
                    <div style={ { padding: '1.5em 1em' } }>
                        <InputRange
                            value={ value }
                            minValue={ minValue }
                            maxValue={ maxValue }

                            onChange={ this.handleChange.bind(this) }
                            onChangeComplete={ handleChangeComplete }
                            formatLabel={ val => formatDayTime(val) }
                            step= { step }
                        />
                    </div>
                </div>

                <Field.Error error={ error } grouped={ grouped } />
            </Field.Row>
        );
    }
}

DayTimeSlider.defaultProps = {
    grouped: false,
    required: false,
    withIcons: true,

    // react-input-range props
    step: 60, // seconds
    handleChangeComplete: () => {},
};

DayTimeSlider.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    value: QuestionModel.valuePropTypes(),
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    required: PropTypes.bool,

    className: PropTypes.string,
    withIcons: PropTypes.bool,

    // react-input-range props
    step: PropTypes.number.isRequired,
    handleChangeComplete: PropTypes.func,
};

export default DayTimeSlider;
