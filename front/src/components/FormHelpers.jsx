import React from 'react';
import PropTypes from 'prop-types';

const FormRow = function(props) {
    return(
        <div className={ props.className }>
            { props.legend && <legend>{ props.legend }</legend> }
            { props.children }
        </div>
    );
};

FormRow.propTypes = {
    className: PropTypes.string,
    header:  PropTypes.string,
    title:  PropTypes.string,
    glyphicon:  PropTypes.string,
};

export { FormRow };
