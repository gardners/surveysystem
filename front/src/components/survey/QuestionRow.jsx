import React from 'react';
import PropTypes from 'prop-types';

import QuestionModel from '../../Question';
import { addClassNames } from '../../Utils';

import InnerHtml from '../InnerHtml';
import { sanitizeKcgiJsonString } from '../../Utils';

import './question.scss';

const QuestionRow = function({ question, appearance, className, grouped, children }) {

    const { type } = question;

    let legend = '';
    let description = sanitizeKcgiJsonString(question.title_text);

    let groupClass = 'form-group';
    let descClass = 'form-text';
    let labelClass = '';
    let controlClass = '';

    switch (appearance) {

        case 'horizontal':
            if(type !== 'HIDDEN') {
                groupClass = addClassNames(groupClass, 'row');
                labelClass = 'col-sm-3 col-form-label'
                controlClass = 'col-sm-9'
            }
        break;

        case 'default':
        break;

        default:
            //nothing
    }

    groupClass = addClassNames(groupClass, `question--${appearance}`);
    // TODO grouped

    return (
        <div className={ addClassNames(className, 'question') }>
            { legend && (typeof legend === 'function') ? legend() : <legend>{ legend }</legend> }
            { description && <InnerHtml className={ descClass } htmlContent={ description } /> }
            <div className={ groupClass }>
                { (type !== 'HIDDEN') ? <label className={ labelClass }>{ question.title }</label> : null }
                <div className={ controlClass }>{ children }</div>
            </div>
        </div>
    );
};

QuestionRow.defaultProps = {
    appearance: 'default',
    grouped: false,
    className: '',
};

QuestionRow.propTypes = {
    className: PropTypes.string,
    question: QuestionModel.propTypes().isRequired,
    appearance: PropTypes.oneOf([
        'default',
        'horizontal',
        'inline',
        'matrix'
    ]),
    grouped: PropTypes.bool,
};

export default QuestionRow;
