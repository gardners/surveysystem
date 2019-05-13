import React, { Component } from 'react';
import PropTypes from 'prop-types';

import InputRange from 'react-input-range';
import './PeriodRangeSlider.scss'; //TODO

import Question from '../../Question';
import { prettyHours, DaySeconds } from '../../Utils';

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
        };
    }

    handleChange(value) {
        const { question } = this.props;

        this.setState({
            value
        });

        this.props.handleChange(null, question, value);
    }

    render() {
        const { value } = this.state;
        const { withIcons} = this.props;
        return (
            <div style={ wrapperStyle }>
                {
                    withIcons &&
                    <div className={ this.props.className } style={ tableStyle }>
                        <div style={ cellStyle(0) }><i className="fas fa-moon"></i></div>
                        <div style={ cellStyle(50) }><i className="fas fa-sun"></i></div>
                        <div style={ cellStyle(100) }><i className="fas fa-moon"></i></div>
                    </div>
                }
                <InputRange
                    className=""
                    minValue={ 0 }
                    maxValue={ DaySeconds }
                    value={ value }
                    onChange={ this.handleChange.bind(this) }
                    formatLabel={ val => prettyHours(val) }
                    step= { this.props.step }
                />
            </div>
        );
    }
}

DayTimeSlider.defaultProps = {
    required: true,
    timeBeginLabel: null,
    timeEndLabel: null,

    // react-input-range props
    step:  15 * 60,
    withIcons: true,
};

DayTimeSlider.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: Question.propTypes().isRequired,
    required: PropTypes.bool,
    timeBeginLabel: PropTypes.string,
    timeEndLabel: PropTypes.string,
    withIcons: PropTypes.bool,

    // react-input-range props
    step: PropTypes.number,
};

export default DayTimeSlider;
