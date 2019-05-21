import React, { Component } from 'react';
import PropTypes from 'prop-types';

import FormGroup from '@material-ui/core/FormGroup';
import Input from '@material-ui/core/Input';
import Button from '@material-ui/core/Button';
import LocationOn from '@material-ui/icons/LocationOn';

import { FieldError, InputGroup } from '../FormHelpers';
import Question from '../../Question';

const supports = typeof navigator !== 'undefined' && 'geolocation' in navigator;

const options = {
    enableHighAccuracy: true,
    timeout: 5000,
    maximumAge: 0,
};

// at least in current Firefox PositionErrors don't seem to be instances of Error
const handlePositionError = function(err = null) {
    if(!err) {
        return 'Unkown position error';
    }

    return new Error(`[${err.code}] ${err.message}`);
};


const fetchLocation = function() {
    return new Promise(function(resolve, reject) {

        if (!supports){
            reject(new Error('Geolocation is not supported by your device'));
            return;
        }

        navigator.geolocation.getCurrentPosition(
            pos => resolve(pos.coords),
            e => reject(handlePositionError(e)), //TODO check if err provided generic error
            options
        );
    });
};

/**
 * Tries to fetch current device location
 */
class GeoLocation extends Component {
    constructor(props) {
        super(props);
        this.state = {
            value: '',
            error: null,
        };
    }

    componentDidMount() {
        if(!this.props.withButton) {
            this.fetchLocation();
        }
    }

    fetchLocation(e) {
        e && e.preventDefault();

        const { question } = this.props;
        const target = (e) ? e.target : null;

        fetchLocation()
        .then((coords) => {

            this.setState({
                value: [coords.latitude, coords.longitude].toString(),
                error: null,
            });

            this.props.handleChange(target, question, coords.latitude, coords.longitude);
        })
        .catch(err => this.setState({
            value: '',
            error: err,
        })); // TODO
    }

    render() {
        const { question } = this.props;
        const { values} = this.state;

        return (
            <React.Fragment>
                <InputGroup prepend={ question.unit }>
                    <FormGroup row>
                        <Input
                            readOnly
                            id={ question.id }
                            name={ question.name }
                            value={ values }
                            variant="outlined"
                            placeholder={ this.props.placeholder }
                        />
                        {
                            this.props.withButton &&
                                    <Button
                                        variant="contained"
                                        aria-label="Location"
                                        onClick={ this.fetchLocation.bind(this) }
                                        style={ { marginLeft: '1rem' } }
                                    >
                                        <LocationOn /> Get Location
                                    </Button>

                        }
                    </FormGroup>
                </InputGroup>
                { <FieldError error={ this.state.error } /> }
            </React.Fragment>
        );
    }
}

GeoLocation.defaultProps = {
    required: true,
    placeholder: null,
    withButton: false,
};

GeoLocation.propTypes = {
    handleChange: PropTypes.func.isRequired,
    withButton: PropTypes.bool,
    question: Question.propTypes().isRequired,
    required: PropTypes.bool,
    placeholder: PropTypes.string,
};

export default GeoLocation;
