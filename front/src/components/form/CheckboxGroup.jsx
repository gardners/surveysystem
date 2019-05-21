import React, { Component } from 'react';
import PropTypes from 'prop-types';

import FormControlLabel from '@material-ui/core/FormControlLabel';
import Checkbox from '@material-ui/core/Checkbox';

import Question from '../../Question';

class CheckboxGroup extends Component {

    constructor(props) {
        super(props);
        this.state = {
            values: [],
        };
    }

    handleChange(e) {
        const { value, checked } = e.target;
        const { question } = this.props;

        let { values } = this.state;

        values = values.filter(v => v !== value);
        if(checked) {
            values.push(value);
        }

        this.setState({
            values,
        });

        this.props.handleChange(e.target, question, values);
    }

    render() {
        const { question } = this.props;
        const { choices } = question;

        return (
            <React.Fragment>
                {
                    choices.map((value, index) =>
                        <FormControlLabel
                            key={ index }
                            control={
                                <Checkbox
                                    color="primary"
                                    name={ question.name }
                                    id={ question.id }
                                    value={ value }
                                    onChange={ this.handleChange.bind(this)}
                                    checked={  this.state.values.indexOf(value) > -1 }
                                />
                            }
                            label={ value }
                        />)
                }
            </React.Fragment>
        );
    }
}

CheckboxGroup.defaultProps = {
    required: true,
};

CheckboxGroup.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: Question.propTypes(true).isRequired,
    required: PropTypes.bool,
};

export default CheckboxGroup;
