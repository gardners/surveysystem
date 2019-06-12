import React from 'react';
import PropTypes from 'prop-types';

import Question from '../../Question';

import { addClassNames, sanitizeKcgiJsonString } from '../../Utils';
import InnerHtml from '../InnerHtml';

/**
 * Question Row (Wrapper)
 */

const Row = function({ question, className, required, grouped, children }) {
    const cls = addClassNames(className, 'col-md-8', question.type, (required) ? 'required' : '');

    return(
        <div className="row justify-content-center align-items-center">
            <div className={ cls }>
                { children }
            </div>
        </div>
    );
};

Row.defaultProps = {
    className: '',
};

Row.propTypes = {
    question: Question.propTypes().isRequired,
    grouped: PropTypes.bool.isRequired,
    required: PropTypes.bool.isRequired,
    className: PropTypes.string,
};

/**
 * Question WDescription
 */

const Description = function({ question, className, required, grouped, children }) {

    if(!question.description) {
        return (null);
    }

    const sanitizedDescription = sanitizeKcgiJsonString(question.description);

    return(
        <div className={ addClassNames('description mb-2', className) }>
            <InnerHtml htmlContent={ sanitizedDescription  } />
            { children }
        </div>
    );
};

Description.defaultProps = {
    className: '',
};

Description.propTypes = {
    question: Question.propTypes().isRequired,
    grouped: PropTypes.bool.isRequired,
    required: PropTypes.bool.isRequired,
    className: PropTypes.string,
};

/**
 * Question unit badge
 */

const Unit = function({ question, grouped, className }) {
    if (!question.unit) {
        return (null);
    }
    return(
        <span className={ className }>{ question.unit }</span> : null
    );
};

Unit.defaultProps = {
    className: '',
};

Unit.propTypes = {
    question: Question.propTypes().isRequired,
    grouped: PropTypes.bool.isRequired,
    className: PropTypes.string,
};

/**
 * Question title
 */

const Title = function({ question, grouped, required, display, className, children}) {
    if(display === 'label') {
        return(
            <label className={ addClassNames('form-check-label', className) } htmlFor={ question.id }>
                { question.title } { children }
            </label>
        );
    }
    return(
        <div className={ addClassNames('mb-2', className) }>
            { question.title } { children }
        </div>
    );
};

Title.defaultProps = {
    className: '',
    display: '',
};

Title.propTypes = {
    question: Question.propTypes().isRequired,
    grouped: PropTypes.bool.isRequired,
    required: PropTypes.bool.isRequired,
    display: PropTypes.oneOf([
        '',
        'label'
    ]),

    className: PropTypes.string,
};

/**
 * Question error
 */

const _Error = function({ error }) {
    return(
        <React.Fragment>
            {
                (error && error instanceof Error) &&
                    <div className="text-danger">
                        { error.toString() }
                    </div>
            }
        </React.Fragment>
    );
};

_Error.defaultProps = {
    error: null,
};

_Error.propTypes = {
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool.isRequired,
};

const InputGroup = function(props) {
    let pre = (typeof props.prepend === 'string') ? [props.prepend] : props.prepend;
    let app = (typeof props.append === 'string') ? [props.append] : props.append;
    pre = pre.filter(entry => !entry === false);
    app = app.filter(entry => !entry === false);

    return(
        <div className="input-group">
            { pre.length > 0 &&
                <div className="input-group-prepend">
                    { pre.map((entry, index) => <span key={ index } className="input-group-text">{ entry }</span>) }
                </div>
            }
            { props.children }
            { app.length > 0 &&
                <div className="input-group-append">
                    { app.map((entry, index) => <span key={ index } className="input-group-text">{ entry }</span>) }
                </div>
            }
        </div>
    );
};

/**
 * Question input group: textual inputs, custom selects, and custom file inputs.
 * @see https://getbootstrap.com/docs/4.3/components/input-group/
 */

InputGroup.defaultProps = {
    append: '',
    prepend: '',
};

InputGroup.propTypes = {
    append: PropTypes.oneOfType([
        PropTypes.string,
        PropTypes.array,
    ]),
    prepend: PropTypes.oneOfType([
        PropTypes.string,
        PropTypes.array,
    ]),
};

const Field = {
    Row,
    Unit,
    Title,
    Error: _Error,
    InputGroup,
    Description,
};

export default Field;
