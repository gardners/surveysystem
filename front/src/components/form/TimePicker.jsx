import React, { Component } from 'react';
import PropTypes from 'prop-types';

import moment from 'moment';

import RcTimePicker from 'rc-time-picker';
import 'rc-time-picker/assets/index.css';

import Field from './Field';
import QuestionModel from '../../Question';
import './TimePicker.scss';

const format = 'h:mm a';

const seconds = function(m) {
    const midnight = m.clone().startOf('day');
    return  m.diff(midnight, 'seconds');
};

class TimePicker extends Component {
    constructor(props) {
        super(props);
        this.state = {
            value: 0,
        };
    }

    componentDidMount() {
        const { question } = this.props;
        this.setState({
            value: question.default_value,
        });
    }

    handleChange(m) {
        const { question, handleChange } = this.props;

        m = m || moment(0); // reset button provides null
        const value = seconds(m);

        this.setState({
            value,
        });
        handleChange(null, question, value);
    }

    render() {
        const { value } = this.state;
        const { handleChange, question, error, required, grouped, className } = this.props;

        return (
            <Field.Row className={ className } question={ question } grouped={ grouped } required={ required }>
                <Field.Description question={ question } grouped={ grouped } required={ required } />
                <Field.Title element="label" grouped={ grouped } question={ question } required={ required }>
                    <Field.Unit className="badge badge-secondary ml-1" question={ question } grouped={ grouped } />
                </Field.Title>

                <RcTimePicker
                    id={ question.id }
                    name={ question.name }
                    showSecond={ false }
                    defaultValue={ moment(0) }
                    className="form-control"
                    onChange={ this.handleChange.bind(this) }
                    format={ format }
                    use12Hours
                    inputReadOnly
                />

                <Field.Error error={ error } grouped={ grouped } />
            </Field.Row>
        );
    }
};

TimePicker.defaultProps = {
    grouped: false,
    required: false,
    // rc-time-picker props
    minuteStep: 10,
};

TimePicker.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    required: PropTypes.bool,

    className: PropTypes.string,
    // rc-time-picker props
    minuteStep: PropTypes.number,
};

export default TimePicker;
