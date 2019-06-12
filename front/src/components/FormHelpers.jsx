import React from 'react';
import PropTypes from 'prop-types';

const FieldError = function(props) {

    if(!props.error) {
        return(null);
    }

    return(
        <div className="text-danger">
            <div className="text-warning">TODO: REMOVE!</div>
            <small>{ (typeof error === 'string') ? props.error : props.error.toString() }</small>
        </div>
    );
};

FieldError.defaultProps = {
    error: null,
};

FieldError.propTypes = {
    error: PropTypes.oneOfType([
        PropTypes.string,
        PropTypes.instanceOf(Error),
    ]),
};

const InputGroup = function(props) {
    let pre = (typeof props.prepend === 'string') ? [props.prepend] : props.prepend;
    let app = (typeof props.append === 'string') ? [props.append] : props.append;
    pre = pre.filter(entry => !entry === false);
    app = app.filter(entry => !entry === false);

    return(
        <div className="input-group">
            <div className="text-warning">TODO: REMOVE!</div>
            { pre.length > 0 &&
                <div className="input-group-prepend">
                    { pre.map((entry, index) => <span key={ index } className="input-group-text">{ entry }</span>) }
                </div>
            }
            { props.children }
            { app.length > 0 &&
                <div className="input-group-append">
                    { app.map((entry, index) => <span key={ index } className="input-group-text">{ entry }</span>) }
                </div>
            }
        </div>
    );
};

InputGroup.defaultProps = {
    append: '',
    prepend: '',
};

InputGroup.propTypes = {
    append: PropTypes.oneOfType([
        PropTypes.string,
        PropTypes.array,
    ]),
    prepend: PropTypes.oneOfType([
        PropTypes.string,
        PropTypes.array,
    ]),
};

export { FieldError, InputGroup };
