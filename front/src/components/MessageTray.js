import React from 'react';
import PropTypes from 'prop-types';

const btClass = function(severity){
    switch(severity) {
        case 'error':
            return 'text-danger';

        case 'warn':
        case 'warning':
            return 'text-warning';

        case 'success':
            return 'text-success';

        default:
            return 'text-info';
    }

};

const MessageTray = function (props, index) {

    const filtered = props.entries.filter((entry) => {
        if(entry.severity !== 'log' || entry.severity !== 'debug') {
            return true;
        }
        return (props.tail > -1 && index < props.tail);
    });

    return (
        <div className="app-message-trail">
        {
            filtered.map((entry, index) => {
                return <div key={ index } className={ 'entry ' + btClass(entry.severity) }>[{ entry.severity }] { entry.message }</div>
            })
        }
        </div>
    );
};

MessageTray.defaultProps = {
  tail: 5,
};

MessageTray.propTypes = {
    entries: PropTypes.array.isRequired,
    tail: PropTypes.number
};

export default MessageTray;
