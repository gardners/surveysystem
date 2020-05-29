import React from 'react';
import PropTypes from 'prop-types';

import { normalizeInsight, insightPropTypes } from '../../Analysis';

const InsightItem = function({ item }) {
    const insight = normalizeInsight(item);

    return (
        <div className="mb-2">
            { insight.displayName && <div><strong>{ insight.displayName }</strong></div> }
            <div>{ insight.displayText }</div>
        </div>
    );
}

InsightItem.defaultProps = {};

InsightItem.propTypes = {
    // #352, add complex model, allow simple strings
    item: PropTypes.oneOfType([
        PropTypes.string,
        insightPropTypes(),
    ]),
};

const Insights = function({ insights }) {
    // #352 legacy string support
    const items = (typeof insights === 'string') ? [insights] : insights;

    return (
        <React.Fragment>
            { items.map((item, key) => <InsightItem key={ key } item={ item } />) }
        </React.Fragment>
    );
}

Insights.defaultProps = {
    insights: [],
};

Insights.propTypes = {
    // #352, add complex model, allow simple strings
    insights: PropTypes.arrayOf([
        PropTypes.oneOfType([
            PropTypes.string,
            insightPropTypes(),
        ])
    ]),
};

export default Insights;
