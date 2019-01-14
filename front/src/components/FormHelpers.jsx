import React from 'react';
import PropTypes from 'prop-types';

import InnerHtml from './InnerHtml';

const FormRow = function(props) {
    return(
        <div className={ props.className }>
            { props.legend && (typeof props.legend === 'function') ? props.legend() : <legend>{ props.legend }</legend> }
            { props.description && <InnerHtml htmlContent={ props.description } /> }
            { props.children }
        </div>
    );
};

FormRow.propTypes = {
    className: PropTypes.string,
    legend:  PropTypes.oneOfType([
        PropTypes.string,
        PropTypes.func,
    ]),
    description: PropTypes.string,
};

const InputGroup = function(props) {
    let pre = (typeof props.prepend === 'string') ? [props.prepend] : props.prepend;
    let app = (typeof props.append === 'string') ? [props.append] : props.append;
    pre = pre.filter(entry => !entry === false);
    app = app.filter(entry => !entry === false);

    return(
        <div className="input-group">
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

const FieldError = function(props) {

    if(!props.error) {
        return(null);
    }

    return(
        <div className="text-danger">
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

export { FormRow, InputGroup, FieldError };
