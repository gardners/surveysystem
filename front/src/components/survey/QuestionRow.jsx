import React from 'react';
import PropTypes from 'prop-types';

import QuestionModel from '../../Question';
import { addClassNames } from '../../Utils';

import InnerHtml from '../InnerHtml';
import { sanitizeKcgiJsonString } from '../../Utils';

import './question.scss';

const QuestionRow = function({ question, appearance, className, grouped, children }) {

    const { title, type, description } = question;

    const legend = ''; // TODO not used currently du to no matching question field: remove?
    const sanitizedDescription = sanitizeKcgiJsonString(description);

    let groupClass = 'form-group';
    let descriptionClass = 'form-text';
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
            { sanitizedDescription && <InnerHtml className={ descriptionClass } htmlContent={ sanitizedDescription } /> }
            <div className={ groupClass }>
                { (type !== 'HIDDEN') ? <label className={ labelClass }>{ title }</label> : null }
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
