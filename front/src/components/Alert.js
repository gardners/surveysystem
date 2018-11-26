import React, { Component } from 'react';
import PropTypes from 'prop-types';

class Alert extends Component {
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
        const { message, severity } = this.props;
        const isErr = (message instanceof Error);
        const severityClass = (isErr || severity === 'error') ? 'danger' : severity;

        if(this.state.closed) {
            return(null);
        }

        return (
            <div className={ `alert alert-${severityClass}` } role="alert">
                { (isErr) ? message.toString() : message }
                <button type="button" className="close" data-dismiss="alert" aria-label="Close"
                    onClick= { this.toggle.bind(this) }><span aria-hidden="true">&times;</span></button>
            </div>
        );
    }
}

Alert.defaultProps = {
  state: 'danger'
};

Alert.propTypes = {
    message:  PropTypes.oneOfType([
        PropTypes.string,
        PropTypes.instanceOf(Error)
    ]).isRequired,
    severity:PropTypes.string,
};

export default Alert;
