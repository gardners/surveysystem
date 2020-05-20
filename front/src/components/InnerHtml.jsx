import React, { useContext }  from 'react';
import PropTypes from 'prop-types';

import { SurveyContext } from '../Context';

/**
 *  #346, parses template tags
 */
const template = function(str, vars) {
    const ex = new RegExp(Object.keys(vars).join('|'), 'g');
    return str.replace(ex, (hit) => {
        return vars[hit];
    });
};

/**
 * @see [dangerouslySetInnerHTML](https://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=2&cad=rja&uact=8&ved=2ahUKEwjL5rbCssHfAhUS5bwKHUlQBl0QFjABegQIAxAB&url=https%3A%2F%2Fzhenyong.github.io%2Freact%2Ftips%2Fdangerously-set-inner-html.html&usg=AOvVaw1VyczKc-x1tPkqVrI2XEAx)
 */
const InnerHtml = function ({ htmlContent, className }) {
    const ctx = useContext(SurveyContext);
    const { PUBLIC_URL, REACT_APP_SITE_NAME, REACT_APP_SURVEY_PROVIDER } = process.env;

    if (!htmlContent) {
        return (null);
    }

    // #346, add template tags
    const __html = template(htmlContent, {
        '%SURVEY_ID%': ctx.surveyID,
        '%SESSION_ID%': ctx.sessionID,
        '%PUBLIC_URL%': PUBLIC_URL,
        '%SITE_NAME%': REACT_APP_SITE_NAME,
        '%SURVEY_PROVIDER%': REACT_APP_SURVEY_PROVIDER,
    });

    return (
        <div className={ className } dangerouslySetInnerHTML={ { __html } } />
    );
};

InnerHtml.defaultProps = {
    htmlContent: '',
};

InnerHtml.propTypes = {
    htmlContent: PropTypes.string,
    className: PropTypes.string,
};

export default InnerHtml;
