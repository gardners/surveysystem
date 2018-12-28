import React from 'react';
import PropTypes from 'prop-types';
/**
 * @see [dangerouslySetInnerHTML](https://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=2&cad=rja&uact=8&ved=2ahUKEwjL5rbCssHfAhUS5bwKHUlQBl0QFjABegQIAxAB&url=https%3A%2F%2Fzhenyong.github.io%2Freact%2Ftips%2Fdangerously-set-inner-html.html&usg=AOvVaw1VyczKc-x1tPkqVrI2XEAx)
 */
const InnerHtml = function (props) {

    if (!props.htmlContent) {
        return (null);
    }

    return (
        <div className={ props.className } dangerouslySetInnerHTML={ { __html: props.htmlContent } } />
    );
};

InnerHtml.defaultProps = {
    htmlContent: '',
};

InnerHtml.propTypes = {
    className: PropTypes.string,
    htmlContent: PropTypes.string,
};

export default InnerHtml;
