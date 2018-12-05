import React, { Component } from 'react';
import PropTypes from 'prop-types';

import { FormRow } from './FormHelpers';

// form elements
import GeoLocation from './form/GeoLocation';
import PeriodRange from './form/PeriodRange';
import RadioGroup from './form/RadioGroup';
import TextInput from './form/TextInput';
import Select from './form/Select';

const Question = function(props) {
    const question = {
        id: props.component.name,
        name: props.component.name,
        title: 'title',
        title_text: 'title text',
        type: '',

        defaultValue: props.defaultValue || 'default',
        choices: props.choices || [],
    };

    const mergedProps = Object.assign({
        question,
        handleChange: () => {},
    }, props);

    return (
        <FormRow className="list-group-item" legend={ question.name }>
            <props.component
                { ...mergedProps }
            />
        </FormRow>
    );
};

Question.propTypes = {
    component: PropTypes.func.isRequired,
};

class Demo extends Component {
    render() {
        return (
            <section className="list-group">
                <Question component={ GeoLocation } withButton={ true } />
                <Question component={ PeriodRange } />
                <Question component={ RadioGroup } choices={ ['Yes', 'No', 'Maybe' ] } defaultValue="Maybe"/>
                <Question component={ Select } choices={ ['First', 'Second', 'Third' ] } defaultValue="Second"/>
                <Question component={ TextInput }/>
            </section>
        );
    }
}

Demo.propTypes = {};

export default Demo;
