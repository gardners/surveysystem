import React, { Component } from 'react';
import PropTypes from 'prop-types';

import { ApiError } from '../api';

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

        const { message } = this.props;
        const isErr = (message instanceof Error);

        const status = (message instanceof ApiError) ? message.status : 0;
        const statusText = (message instanceof ApiError) ? message.statusText : '';
        const url = (message instanceof ApiError) ? message.url : '';

        return (
            <div className="alert alert-danger" role="alert">
                <button type="button" className="close" data-dismiss="alert" aria-label="Close" onClick= { this.toggle.bind(this) }><span aria-hidden="true">&times;</span></button>
                { statusText && <h4 className="alert-heading">{ statusText } ({ status })</h4> }
                <div><small>{ (isErr) ? message.toString() : message }</small></div>
                { (url) ? <small><hr /> • { url }</small> : null }
            </div>
        );
    }
}

ApiAlert.defaultProps = {
  state: 'danger'
};

ApiAlert.propTypes = {
    message:  PropTypes.oneOfType([
        PropTypes.instanceOf(Error),
        PropTypes.string,
    ]).isRequired,
};

export default ApiAlert;
