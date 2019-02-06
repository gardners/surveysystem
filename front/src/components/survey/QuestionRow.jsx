import React from 'react';
import PropTypes from 'prop-types';

import QuestionModel from '../../Question';
import { addClassNames } from '../../Utils';

import InnerHtml from '../InnerHtml';
import { sanitizeKcgiJsonString } from '../../Utils';

import './question.scss';

const QuestionRow = function({ question, className, grouped, componentName, children }) {

    const { title, type, description } = question;

    const legend = ''; // TODO not used currently du to no matching question field: remove?
    const sanitizedDescription = sanitizeKcgiJsonString(description);

    let colClass = '';
    let labelClass = '';

    switch (componentName) {
        case 'HiddenInput':
        case 'PeriodRangeSlider':
        case 'DayTimeSlider':
        case 'RadioMatrix':
            colClass = 'col';
        break;

        default:
            colClass = 'col-md-6';
    }

    // TODO grouped

    return (
        <section className={ addClassNames(className, 'question') }>
            { legend && (typeof legend === 'function') ? legend() : <legend>{ legend }</legend> }
            { (type === 'HIDDEN') ? <label className="d-block">{ title }</label> : null }
            { sanitizedDescription && <InnerHtml className="form-text" htmlContent={ sanitizedDescription } /> }

            <div className="row justify-content-center align-items-center">
                <div className={ addClassNames(colClass, 'form-group') }>
                    { (type !== 'HIDDEN') ? <label className="d-block">{ title }</label> : null }
                    { children }
                </div>
            </div>
        </section>
    );
};

QuestionRow.defaultProps = {
    grouped: false,
    className: '',
};

QuestionRow.propTypes = {
    className: PropTypes.string,
    question: QuestionModel.propTypes().isRequired,
    componentName: PropTypes.string.isRequired, // form input component name
    grouped: PropTypes.bool,
};

export default QuestionRow;
