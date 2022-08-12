import React, { Component } from 'react';
import PropTypes from 'prop-types';

import { ApiError } from '../Api';
import Toggle from './Toggle';

class ApiAlert extends Component {
    constructor(props) {
        super(props);
        this.state = {
            closed: false,
        };
    }

    toggle(e) {
        e.preventDefault();
        this.setState({
            closed: !this.state.closed,
        });
    }

    render() {
        if(this.state.closed) {
            return(null);
        }

        const { error } = this.props;

        let reason =  (error instanceof Error) ? error.message : String(error);
        let details = '';
        let url = '';
        let status = '';
        let statusText = '';

        // @see fcgmain.c: quick_error()
        try {
            const json = JSON.parse(reason);

            if (json.hasOwnProperty('message')) {
                reason = json.message;
            }
            if (json.hasOwnProperty('detail')) {
                reason = json.detail;
            }

            details = json.trace || '';
        } catch (e) {
            // nothing
        }

        // @see Api.js
        if (error instanceof ApiError) {
            ({ url, status, statusText } = error);
        }

        return (
            <div className="alert alert-danger" role="alert">
                <button type="button" className="close" data-dismiss="alert" aria-label="Close" onClick= { this.toggle.bind(this) }><span aria-hidden="true">&times;</span></button>

                <div><strong>SurveyError:</strong> { reason }</div>
                {
                    (error instanceof ApiError) &&
                        <Toggle className="d-block text-default p-2" title="More info...">
                            <ul style={ { fontSize: '.85em' } }>
                                { statusText && <li><strong>status:</strong> { statusText } ({ status })</li> }
                                { url && <li><strong>url:</strong> { url }</li> }
                                { details && <li><strong>details:</strong> { details }</li> }
                            </ul>
                        </Toggle>
                }
            </div>
        );
    }
}

ApiAlert.defaultProps = {
  state: 'danger'
};

ApiAlert.propTypes = {
    error:  PropTypes.oneOfType([
        PropTypes.instanceOf(Error),
        PropTypes.string,
    ]).isRequired,
};

export default ApiAlert;
