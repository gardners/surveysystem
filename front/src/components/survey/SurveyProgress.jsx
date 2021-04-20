import React from 'react';
import PropTypes from 'prop-types';

import Session from '../../Session';
import { addClassNames } from '../../Utils';

const SurveyProgress = function({ session, className }) {
    const { progress } = session;

    if (session.isCompleted()) {
        return (null);
    }

    if (progress.length !== 2 || progress[0] < 0) {
        console.warn('invalid progress:', progress.toString());
        return (null);
    }

    const percent = Math.round((progress[0] / progress[1]) * 100);
    const cls = addClassNames('progress', className);

    return (
        <div className={ cls }>
            <div className="progress-bar" role="progressbar" style = { {
                width: `${percent}%`,
            } } aria-valuenow={ percent } aria-valuemin="0" aria-valuemax="100">{ progress[0] }/{ progress[1] }</div>
        </div>
    );

};

SurveyProgress.defaultProps = {
    className: '',
};

SurveyProgress.propTypes = {
    session: PropTypes.instanceOf(Session).isRequired,
    className: PropTypes.string,
};

export default SurveyProgress;
