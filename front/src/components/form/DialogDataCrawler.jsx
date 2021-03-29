import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';
import { isArray } from '../../Utils';

import { SurveyContext } from '../../Context';

// config
const { REACT_APP_MODULES_ENDPOINT } = process.env;

/**
 * Build a module queue link
 * @see https://github.com/RoboSparrow/fitbit-queue/blob/master/server.js
 */

const buildModuleLink = function(question, session_id) {
    const { id, unit, choices } = question;
    let { href } = window.location;

    if(!session_id) {
        console.error('no session id');
        return null; // wait until props update
    }

    if (!isArray(choices) || !choices.length) {
        console.error(`Invalid question (${id}). Choices missing or empty.`);
        return null;
    }

    href = (href.indexOf('?') === -1) ? `${href}?progress=finished` : `${href}&progress=finished`;
    const state = JSON.stringify({
        session_id,
        href,
    });

    // insert additional units here
    switch (unit) {
        case 'fitbit-module':
            return REACT_APP_MODULES_ENDPOINT + '/fibit/login?t=' + Date.now() + '&state=' + encodeURIComponent(state);

        default:
            console.error(`(${id}) Unrecognised module: ${unit}. This is an internal error. Please proceed with the survey.`);
            return null;
    }
};

const Module = function({ question, session_id, progress, accept, reject }) {

    const moduleLink = buildModuleLink(question, session_id);

    return (
            <React.Fragment>
            {
                (moduleLink && ['finished', 'rejected'].indexOf(progress) === -1) &&
                    <div className="row">
                        <div className="col text-center">
                            <p>
                                <a className="btn btn-success" href={ moduleLink }>Yes</a>
                            </p>
                            <small>I'm happy to provide my data</small>
                        </div>
                        <div className="col text-center">
                            <p>
                                <button className="btn btn-outline-secondary" onClick={ reject }>No</button>
                            </p>
                            <small>I'm skipping this</small>
                        </div>
                    </div>
            }
            {
                (progress === 'finished') &&
                    <div className="row">
                        <div className="col text-center text-success mb-2 mt-2">
                            <strong><i className="fas fa-check-circle"></i> Thank you!</strong>
                        </div>
                    </div>
            }
            {
                (['finished', 'rejected'].indexOf(progress) !== -1) &&
                    <div className="row">
                        <div className="col text-center">
                            <i>Go to next question</i>
                        </div>
                    </div>
            }
            <Field.Error error={ (moduleLink instanceof Error) ? moduleLink : null } grouped= { false }/>
        </React.Fragment>
    );
};

Module.defaultProps = {
    session_id: '',
    progress: '',
};

Module.propTypes = {
    question: QuestionModel.propTypes().isRequired,
    session_id: PropTypes.string,
    progress: PropTypes.string,
    reject: PropTypes.func.isRequired,
};

class DialogDataCrawler extends Component {

    constructor(props) {
        super(props);

        this.state = {
            progress: '',
            value: 'skipped',
        };
    }

    componentDidMount() {
        const params = new URLSearchParams(window.location.search);
        const progress = params.get('progress'); // bar

        // TODO: listening to session_id, so move to componentdidupdate?
        const { question } = this.props;

        // supply either the neutral value (denied) or the error string
        const value = question.choices[0];
        this.setState({
            progress,
        });

        // Immediately invoke answer callback, in order to allow the user to progress without proceeding
        this.props.handleChange(null, question, value);
    }

    reject () {
        const { question } = this.props;
        this.setState({
            progress: 'rejected',
            value: question.choices[0]
        });
    }

    render() {
        const { progress, value } = this.state;
        const { question, grouped, className } = this.props;

        // # 224, don't flag this qtype as required
        const required = false;

        return (
            <SurveyContext.Consumer>
            {
                ({ session_id }) => (
                    <Field.Row className={ className } question={ question } grouped={ grouped } required={ required }>
                        <Field.Title grouped={ grouped } question={ question } required={ required } />
                        { (['finished', 'rejected'].indexOf(progress) === -1) && <Field.Description question={ question } grouped={ grouped } required={ required } /> }
                        <input
                            id={ question.id }
                            name={ question.name }
                            type="hidden"
                            autoComplete="off"
                            value={ value }
                        />
                        <Module
                            question={ question }
                            session_id={ session_id }
                            progress={ progress }
                            reject={ this.reject.bind(this) }
                        />
                    </Field.Row>
                )
            }
            </SurveyContext.Consumer>
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
