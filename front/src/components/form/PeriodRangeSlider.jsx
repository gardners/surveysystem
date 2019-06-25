import React, { Component } from 'react';
import PropTypes from 'prop-types';

import InputRange from 'react-input-range';
import { Gutter, DaytimeIcon, DaytimeLabel } from './DaytimeSequence';

import Field from './Field';
import QuestionModel from '../../Question';

import { formatDayTime, DaySeconds } from '../../Utils';

import './DayTimeSlider.scss';

/**
 * range sliders for defining period secondss (seconds) within 24 hours
 */
class PeriodRangeSlider extends Component {
    constructor(props) {
        super(props);
        this.state = {
            value: {
                min: 0,
                max: DaySeconds,
            },
            minValue: 0,
            maxValue: DaySeconds,
        };
    }

    componentDidMount() {
        const { question } = this.props;

        let minValue = Number(question.min_value);
        let maxValue = Number(question.max_value);
        minValue = (!isNaN(minValue)) ? minValue : 0;
        maxValue = (!isNaN(maxValue) && maxValue > minValue) ? maxValue : DaySeconds;

        const splitted = question.default_value.split(',');
        const min = (splitted.length === 2) ? Number(splitted[0]) : Math.floor(maxValue / 3);
        const max = (splitted.length === 2) ? Number(splitted[1]) : 2 * Math.floor(maxValue / 3);

        this.setState({
            value: {
                min,
                max
            },
            minValue,
            maxValue,
        });
    }

    handleChange(value) {
        const { question } = this.props;

        this.setState({
            value,
        });

        this.props.handleChange(null, question, [value.min, value.max]);
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

                <div className="row">
                    <div className="col daytime-slider">
                        <Gutter className="mb-4" component={ DaytimeIcon } min={ 0 } max={ DaySeconds } />
                        <InputRange
                            value={ value }
                            minValue={ minValue }
                            maxValue={ maxValue }

                            onChange={ this.handleChange.bind(this) }
                            onChangeComplete={ handleChangeComplete }
                            formatLabel={ val => formatDayTime(val) }
                            step= { step }
                        />
                        <Gutter className="mb-4" component={ DaytimeLabel } min={ 0 } max={ DaySeconds } />
                    </div>
                </div>

                <Field.Error error={ error } grouped={ grouped } />
            </Field.Row>
        );
    }
}

PeriodRangeSlider.defaultProps = {
    grouped: false,
    required: false,
    withIcons: true,

    // react-input-range props
    step: 60, // seconds
    handleChangeComplete: () => {},
};

PeriodRangeSlider.propTypes = {
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


export default PeriodRangeSlider;
