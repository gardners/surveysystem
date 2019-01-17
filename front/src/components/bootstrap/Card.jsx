import React from 'react';
import PropTypes from 'prop-types';

const Card = function(props) {
    const cls = props.className || '';
    const Icon = () => props.icon || null;
    return(
        <div className={ `card ${cls}` }>
            { props.header && <div className="card-header">
                { props.header }
            </div> }
            <div className="card-body">
                { props.title && <h4 className="card-title">{ props.icon && <Icon /> } { props.title }</h4> }
                { props.children }
            </div>
        </div>
    );
};

Card.propTypes = {
    className: PropTypes.string,
    header:  PropTypes.string,
    title:  PropTypes.string,
    icon:  PropTypes.object, //component
};

export default Card;
