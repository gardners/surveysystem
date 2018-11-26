import React, { Component } from 'react';
import PropTypes from 'prop-types';

const supports = typeof navigator !== 'undefined' && 'geolocation' in navigator;

const options = {
    enableHighAccuracy: true,
    timeout: 5000,
    maximumAge: 0,
};

const fetchLocation = function() {
    return new Promise(function(resolve, reject) {

        if (!supports){
            reject(new Error('Geolocation is not supported by your device'));
            return;
        }

        navigator.geolocation.getCurrentPosition(
            pos => resolve(pos.coords),
            e => reject(e), //TODO check if err provided generic error
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
            value: null,
        };
    }

    componentDidMount() {
        const { question } = this.props;
        fetchLocation()
        .then(coords => this.setState({ value: coords }))
        .then(this.props.handleChange(this.state.value, question))
        .catch(err => console.error(err)); // TODO
    }

    render() {
        const { question } = this.props;
        const { value } = this.state;
        return (
            <div className="form-group">
                <label htmlFor={ question.id }>{ question.title_text }</label>
                <input
                    readOnly
                    id={ question.id }
                    name={ question.name }
                    type="text"
                    className="form-control"
                    placeholder={ this.props.placeholder }
                    value={ (value) ? [value.latitude, value.longitude].toString() : '' }
                />
            </div>
        );
    }
}

GeoLocation.defaultProps = {
    placeholder: null,
};

GeoLocation.propTypes = {
    handleChange: PropTypes.func.isRequired,
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
