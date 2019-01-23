import React from 'react';
import PropTypes from 'prop-types';

import moment from 'moment';

import RcTimePicker from 'rc-time-picker';
import 'rc-time-picker/assets/index.css';

import { InputGroup } from '../FormHelpers';
import './TimePicker.scss';

import Question from '../../Question';

const format = 'h:mm a';

const seconds = function(m) {
    const midnight = m.clone().startOf('day');
    return  m.diff(midnight, 'seconds');
};

const TimePicker = function(props) {

    const { question, handleChange } = props;
    return (
        <InputGroup prepend={ question.unit }>
            <RcTimePicker
                id={ question.id }
                name={ question.name }
                showSecond={ false }
                defaultValue={ moment(0) }
                className="form-control"
                onChange={ (m) => {
                    const value = seconds(m);
                    handleChange(null, question, value);
                } }
                format={ format }
                use12Hours
                inputReadOnly
            />
        </InputGroup>
    );

};

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
