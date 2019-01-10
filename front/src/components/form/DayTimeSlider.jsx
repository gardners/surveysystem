import React, { Component } from 'react';
import PropTypes from 'prop-types';

import InputRange from 'react-input-range';
import './PeriodRangeSlider.scss'; //TODO

const DaySec = 86400; // 24 hours

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

const prettyHours = function(sec) {
    let hours   = Math.floor(sec / 3600);
    let minutes = Math.floor((sec - (hours * 3600)) / 60);
    let seconds = sec - (hours * 3600) - (minutes * 60);

    if (hours   < 10) {hours   = "0" + hours;}
    let t = 'am';
    if (hours > 12) {
        hours -= 12;
        t = 'pm';
    }
    if (minutes < 10) {minutes = "0" + minutes;}
    if (seconds < 10) {seconds = "0" + seconds;}
    return `${hours}:${minutes} ${t}`;
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
        const { question } = this.props;
        const { value } = this.state;

        return (
            <div className="form-group daytime-range">
                <label> { question.title } { question.unit && <em>({ question.unit })</em> }</label>
                <div style={ wrapperStyle }>
                    <div className={ this.props.className } style={ tableStyle }>
                        <div style={ cellStyle(0) }><i className="fas fa-moon"></i></div>
                        <div style={ cellStyle(50) }><i className="fas fa-sun"></i></div>
                        <div style={ cellStyle(100) }><i className="fas fa-moon"></i></div>
                    </div>
                    <InputRange
                        className=""
                        minValue={ 0 }
                        maxValue={ DaySec }
                        value={ value }
                        onChange={ this.handleChange.bind(this) }
                        formatLabel={ val => prettyHours(val) }
                        step= { this.props.step }
                    />
                </div>
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
};

DayTimeSlider.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: PropTypes.shape({
        id: PropTypes.string.isRequired,
        name: PropTypes.string.isRequired,
        title: PropTypes.string.isRequired,
        title_text: PropTypes.string.isRequired,
        type: PropTypes.string.isRequired,
        unit: PropTypes.string.isRequired,

        timeBeginLabel: PropTypes.string,
        timeEndLabel: PropTypes.string,
    }).isRequired,
    required: PropTypes.bool,

    // react-input-range props
    step: PropTypes.number,
};

export default DayTimeSlider;
