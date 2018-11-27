import React, { Component } from 'react';
import PropTypes from 'prop-types';

// form elements
import GeoLocation from './form/GeoLocation';
import PeriodRange from './form/PeriodRange';
import RadioGroup from './form/RadioGroup';
import TextInput from './form/TextInput';
import Select from './form/Select';

const Question = function(props) {
    const question = Object.assign({
        id: props.component.name,
        name: props.component.name,
        title: 'title',
        title_text: 'title text',
        type: '',

        defaultValue: props.defaultValue || 'default',

    }, props);

    return (
        <div className="panel panel-default">
            <div className="panel-heading">Component: { props.component.name }</div>
            <div className="panel-body">
                <props.component
                    question={ question }
                    handleChange={ () => {} }
                />
            </div>
        </div>
    );
};

Question.propTypes = {
    component: PropTypes.func.isRequired,
};

class Demo extends Component {
    render() {
        return (
            <section className="container">
                <div>
                    <Question component={ GeoLocation } />
                </div>
                <div>
                    <Question component={ PeriodRange } />
                </div>
                <div>
                    <Question component={ RadioGroup } choices={ ['Yes', 'No', 'Maybe' ] } defaultValue="Maybe"/>
                </div>
                <div>
                    <Question component={ Select } choices={ ['First', 'Second', 'Third' ] } defaultValue="Second"/>
                </div>
                <div>
                    <Question component={ TextInput }/>
                </div>
            </section>
        );
    }
}

Demo.propTypes = {};

export default Demo;
