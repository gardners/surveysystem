import React, { Component } from 'react';
import PropTypes from 'prop-types';

import LocalStorage from '../../storage/LocalStorage';

import Field from './Field';
import QuestionModel from '../../Question';
import { isArray } from '../../Utils';

// config
const {
    REACT_APP_SURVEY_CACHEKEY,
    REACT_APP_MODULES_ENDPOINT,
} = process.env;

/**
 * @see https://github.com/RoboSparrow/fitbit-queue/blob/master/server.js
 */
const getModule = function(question) {
    const { id, unit, choices } = question;

    const survey = LocalStorage.get(REACT_APP_SURVEY_CACHEKEY);

    if (!isArray(choices) || !choices.length) {
        return new Error(`Invalid question (${id}). Choices missing or empty.`);
    }

    const state = JSON.stringify({
        session_id: survey.sessionID,
        href: window.location.href,
    });

    switch (unit) {
        case 'fitbit-module':
            return {
                name: 'FitBit Sleep data',
                href: REACT_APP_MODULES_ENDPOINT + '/fibit/login?t=' + Date.now() + '&state=' + encodeURIComponent(state),
            }
        default:
            return new Error(`(${id}) Unrecognised module: ${unit}. This is an internal error. Please proceed with the survey.`);
    }
};

class DialogDataCrawler extends Component {

    constructor(props) {
        super(props);

        this.state = {
            progress: '',
            value: 'skipped',
            module: null,
        };
    }

    componentDidMount() {
        const { question } = this.props;
        const module = getModule(question);

        // supply either the neutral value (denied) or the error
        const value = (module instanceof Error) ? module.toString() : question.choices[0];
        this.setState({
            value,
            module,
        });

        // Immediately invoke answer callback, in order to allow the user to progress without proceeding
        this.props.handleChange(null, question, value);

    }

    render() {
        const { progress, value, module } = this.state;
        const { question, grouped, className } = this.props;

        // # 224, don't flag this qtype as required
        const required = false;
        const withModule = module && !(module instanceof Error);
        const showButton = progress !== 'finished' && withModule;

        return (
            <Field.Row className={ className } question={ question } grouped={ grouped } required={ required }>
                <Field.Title grouped={ grouped } question={ question } required={ required } />
                <Field.Description question={ question } grouped={ grouped } required={ required } />
                <input
                    id={ question.id }
                    name={ question.name }
                    type="hidden"
                    autoComplete="off"
                    value={ value }
                />
                {
                    (showButton) ?
                        <a className="btn btn-primary btn-sm" href={ module.href }>
                            <strong>Yes</strong>, <small>I am happy to provide my data</small>
                        </a>
                        : <p><strong><i className="fas fa-check-circle"></i> Thank you!</strong></p>
                }
                <Field.Error error={ (module instanceof Error) ? module : null } grouped={ grouped } />
            </Field.Row>
        );
    }
};

DialogDataCrawler.defaultProps = {
    grouped: false,
    required: false,
};

DialogDataCrawler.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    value: QuestionModel.valuePropTypes(),
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    required: PropTypes.bool,

    className: PropTypes.string,
};

export default DialogDataCrawler;
