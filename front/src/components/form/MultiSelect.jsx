import React, { Component } from 'react';
import PropTypes from 'prop-types';

import FormControl from '@material-ui/core/FormControl';
import Input from '@material-ui/core/Input';
import MenuItem from '@material-ui/core/MenuItem';
import Select from '@material-ui/core/Select';
import Chip from '@material-ui/core/Chip';
import InputLabel from '@material-ui/core/InputLabel';

import { InputGroup } from '../FormHelpers';
import Question from '../../Question';

const styles = {
    root: {
        display: 'flex',
        flexWrap: 'wrap',
    },
    formControl: {
        minWidth: 300,
    },
};

const ITEM_HEIGHT = 48;
const ITEM_PADDING_TOP = 8;

const MenuProps = {
    PaperProps: {
        style: {
            maxHeight: ITEM_HEIGHT * 4.5 + ITEM_PADDING_TOP,
            width: 250,
        },
    },
};

class MultiSelect extends Component {
    constructor(props) {
        super(props);

        this.state = {
            values: [],
        }
    }

    handleChange(e) {
        const element = e.target;
        const values = element.value;
        this.setState({ values });
        this.props.handleChange(element, this.props.question, values);
    }

    render () {
        const { question, required } = this.props;
        const { id, name, choices, unit } = question;
        const { values } = this.state;

        return (
            <InputGroup prepend={ unit } style={ styles.root }>
                <FormControl style={ styles.formControl }>
                    <InputLabel htmlFor={ id }>Select from...</InputLabel>
                    <Select
                        multiple
                        value={ values }
                        onChange={ this.handleChange.bind(this) }
                        input={
                            <Input id={ id } name= { name }/>
                        }
                        renderValue={
                            selected => (
                                <div>
                                    {
                                        selected.map(value =>
                                            <Chip key={ value } label={ value }/>
                                        )
                                    }
                                </div>
                            )
                        }
                        MenuProps={ MenuProps }
                        required={ required }
                    >
                    {
                        choices.map(choice => (
                            <MenuItem key={ choice } value={ choice }>
                                { choice }
                            </MenuItem>
                        ))
                    }
                    </Select>
                </FormControl>
            </InputGroup>
        );
    }

}

MultiSelect.defaultProps = {
    required: true,
};

MultiSelect.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: Question.propTypes(true).isRequired,
    required: PropTypes.bool,
};

export default  MultiSelect;
