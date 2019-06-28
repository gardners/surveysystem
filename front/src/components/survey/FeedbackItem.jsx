import React from 'react';
import PropTypes from 'prop-types';

import { Fade } from '../Transitions';
import { addClassNames } from '../../Utils';

/**
 * FeedbackItem
 */

const FeedbackItem = function({ status, className, classNamePrefix, children }) {
    let statusClass = '';

    switch (status) {
        case '':
        break;

        case 'error':
        case 'danger':
            statusClass = classNamePrefix + 'danger';
        break;

        case 'success':
            statusClass = classNamePrefix + 'success';
        break;

        default:
            statusClass = classNamePrefix + status;
    }

    const cls = addClassNames(className, statusClass);

    return (
        <Fade className={ cls }>
            { children }
        </Fade>
    );
};

FeedbackItem.defaultProps = {
    status: '',
    className: '',
    classNamePrefix: '',
};

FeedbackItem.propTypes = {
    className: PropTypes.string,
    classNamePrefix: PropTypes.string,
    status: PropTypes.string,
};

export default FeedbackItem;
