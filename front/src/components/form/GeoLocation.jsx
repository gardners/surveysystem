import React, { Component } from 'react';
import PropTypes from 'prop-types';

import { FieldError } from '../FormHelpers';

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

        fetchLocation()
        .then((coords) => {

            this.setState({
                value: [coords.latitude, coords.longitude].toString(),
                error: null,
            });

            this.props.handleChange(question, coords.latitude, coords.longitude);
        })
        .catch(err => this.setState({
            value: '',
            error: err,
        })); // TODO
    }

    render() {
        const { question } = this.props;
        const { value } = this.state;
        return (
            <div className="form-group">
                <label htmlFor={ question.id }>{ question.title_text }</label>

                <div className="input-group">
                    <input
                        readOnly
                        id={ question.id }
                        name={ question.name }
                        type="text"
                        className="form-control"
                        placeholder={ this.props.placeholder }
                        value={ value }
                    />
                    {
                        this.props.withButton &&
                            <div className="input-group-append">
                                <button className="btn btn-secondary btn-sm" onClick={ this.fetchLocation.bind(this) }>Get Location</button>
                            </div>
                    }
                </div>

                { <FieldError error={ this.state.error } /> }

            </div>
        );
    }
}

GeoLocation.defaultProps = {
    placeholder: null,
    withButton: false,
};

GeoLocation.propTypes = {
    handleChange: PropTypes.func.isRequired,
    withButton: PropTypes.bool,
    question: PropTypes.shape({
        id: PropTypes.string.isRequired,
        name: PropTypes.string.isRequired,
        title: PropTypes.string.isRequired,
        title_text: PropTypes.string.isRequired,
        type: PropTypes.string.isRequired,
    }).isRequired,
    placeholder: PropTypes.string,
};

export default GeoLocation;
