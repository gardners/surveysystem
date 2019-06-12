import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';

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
 *
 * Question default value is ignored
 */
class DeviceLocation extends Component {
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
            const val = [coords.latitude, coords.longitude];
            this.setState({
                value: val.toString(),
                error: null,
            });

            this.props.handleChange(target, question, val);
        })
        .catch(err => this.setState({
            value: '',
            error: err,
        })); // TODO
    }

    render() {
        const { question, required, grouped, className, withButton } = this.props;
        const error = (this.state.error) ? this.state.error : this.props.error;

        const { value } = this.state;
        return (
            <Field.Row className={ className } question={ question } grouped={ grouped } required={ required }>
                <Field.Title display="" grouped={ grouped } question={ question } required={ required }>
                    <Field.Unit className="badge badge-secondary ml-1" question={ question } grouped={ grouped } />
                </Field.Title>
                <Field.Description question={ question } grouped={ grouped } required={ required } />
                    <input
                        readOnly
                        id={ question.id }
                        name={ question.name }
                        type="text"
                        className="form-control"
                        autoComplete="off"
                        required={ required }
                        placeholder="Allow your device to fetch location"
                        value={ value }
                    />
                    {
                        withButton &&
                            <div className="mt-2">
                                <button className="btn btn-secondary btn-sm" onClick={ this.fetchLocation.bind(this) }>Get Location</button>
                            </div>
                    }
                <Field.Error error={ error } grouped={ grouped } />
            </Field.Row>
        );
    }
}

DeviceLocation.defaultProps = {
    grouped: false,
    required: false,

    withButton: true,
};

DeviceLocation.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    value: QuestionModel.valuePropTypes(),
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    required: PropTypes.bool,

    className: PropTypes.string,
    withButton: PropTypes.bool
};

export default DeviceLocation;
