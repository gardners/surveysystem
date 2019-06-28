import React from 'react';
import PropTypes from 'prop-types';

import Transition from 'react-transition-group/Transition';

const Fade = function({ timeout, className, children }) {
    const transitionStyles = {
        entering: { opacity: 0 },
        entered:  { opacity: 1 },
    };

    const defaultStyle = {
        transition: `opacity ${timeout}ms ease-in-out`,
        opacity: 0,
    };

    return (
        <Transition in={ true } timeout={ timeout } appear>
            {
                state => <div className={ className } style={ { ...defaultStyle, ...transitionStyles[state] } }>{ children }</div>
            }
        </Transition>
  );
};

Fade.defaultProps = {
    timeout: 300,
    className: '',
};

Fade.propTypes = {
    timeout: PropTypes.number,
    className: PropTypes.string,
};

export { Fade };
