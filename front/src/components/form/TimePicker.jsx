import React, { Component } from 'react';
import PropTypes from 'prop-types';
import moment from 'moment';

import MomentUtils from '@date-io/moment';
import { MuiPickersUtilsProvider } from '@material-ui/pickers';
import { TimePicker as MuiTimePicker }  from "@material-ui/pickers";
import InputAdornment from '@material-ui/core/InputAdornment';
import ScheduleIcon from "@material-ui/icons/Schedule";

import { InputGroup } from '../FormHelpers';
import Question from '../../Question';

const format = 'h:mm a';

const seconds = function(m) {
    const midnight = m.clone().startOf('day');
    return  m.diff(midnight, 'seconds');
};

class TimePicker extends Component {
    constructor(props) {
        super(props);
        this.state = {
            value: moment(0)
        }
    }

    handleChange(m) {
        m = m || moment(0);
        this.setState({
            value: m
        });
        this.props.handleChange(null, this.props.question, seconds(m));
    }

    render() {
        const { question, handleChange } = this.props;
        const { id, name, unit } = question;
        const { value } = this.state;

        return (
            <InputGroup prepend={ unit }>
                <MuiPickersUtilsProvider utils={ MomentUtils }>
                    <MuiTimePicker
                        ampm
                        clearable
                        id={ id }
                        name={ name }
                        seconds={ false }
                        value = { value }
                        minutesStep={ 5 }
                        onChange={ this.handleChange.bind(this) }
                        format={ format }

                        InputProps={{
                            /* TextField props https://material-ui.com/demos/text-fields/#with-icon */
                            startAdornment: (
                                <InputAdornment position="start">
                                    <ScheduleIcon />
                                </InputAdornment>
                            ),
                        }}

                    />
                </MuiPickersUtilsProvider>
            </InputGroup>
        );
    }

}

TimePicker.defaultProps = {
    required: true,
    // rc-time-picker props
    minuteStep: 10,
};

TimePicker.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: Question.propTypes().isRequired,
    required: PropTypes.bool,

    // rc-time-picker props
    minuteStep: PropTypes.number,
};

export default TimePicker;
