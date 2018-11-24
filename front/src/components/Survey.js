import React, { Component } from 'react';
import PropTypes from 'prop-types';

// configuration
import { Configuration } from '../conf/config';

// apis
import api from '../api';
import { serializeAnswer } from '../payload-serializer';
import SurveyManager from '../SurveyManager';

import TextInput from './form/TextInput';
import RadioGroup from './form/RadioGroup';
import GeoLocation from './form/GeoLocation';
import PeriodRange from './form/PeriodRange';

// components
import LoadingSpinner from './LoadingSpinner';
import Alert from './Alert';

// devel
import Dev from './Dev';

class Survey extends Component {
    constructor(props) {
        super(props);
        const surveyID = props.match.params.id;

        this.state = {
            survey: new SurveyManager(surveyID),
            answers: {}, // TODO consider moving answer map into SurveyManager as well

            loading: '',
            messages: [],
        };
    }

    componentDidMount() {
        const { survey } = this.state;
        this.setState({ loading: 'Initializing survey...' });

        api.createNewSession(survey.surveyID)
        .then(sessID => survey.init(sessID))
        .then(() => api.nextQuestion(survey.sessionID))
        .then(response => survey.add(response.next_questions))
        .then(() => this.setState({
            loading: '',
            survey,
        }))
        .catch(err => this.alert(err));
    }

    /**
     * Sends an Error to this.state.alerts
     * This method ist tpically used by the request api in case of response or network errors
     * @param {Error} error
     * @returns {void}
     */
    notify(message, status) {
        if(status === 'error'  && !message instanceof Error) {
            message = new Error(message);
        }

        if(status !== 'error' && message instanceof Error) {
            status = 'error';
        }

        const { messages } = this.state;
        messages.push({
            status,
            message,
        });

        this.setState({
            messages,
        });
    }

    ////
    // form processing
    ////

    handleChange(answer, question) {
        const { answers } = this.state;
        const { id } = question;
        this.setState({
            answers: Object.assign(answers, {
                [id]: {
                    answer,
                    question,
                },
            })
        });
    }

    ////
    // form submissions
    ////

    handleUpdateAnswers(e) {
        e.preventDefault();

        const { survey, answers } = this.state;
        this.setState({ loading: 'Sending answer...' });

        const csvFragments = Object.keys(answers).map((id) => {
            const entry = answers[id];
            const { answer, question } = entry;
            const type = question.type;
            return serializeAnswer(id, answer, type);
        });

        // send current answer(s)
        // loading while the questions are sent and the next question is not displayed
        // each answer is a single request, we are bundling them into Promise.all
        Promise.all(
            Object.keys(csvFragments).map(id => api.updateAnswer(survey.sessionID, csvFragments[id]))
        )
        .then(responses => responses.pop()) // last
        .then(response => survey.add(response.next_questions))
        .then(() => this.setState({
            loading: '',
            survey,
            answers: {}, // TODO
        }))
        .catch(err => this.alert(err));
    }

    handleDelAnswer(e) {
        e.preventDefault();

        const { survey } = this.state;
        this.setState({ loading: 'Fetching previous question...' });

        api.deleteAnswer(survey.sessionID)
        .then(response => survey.add(response.next_questions))
        .then(() => this.setState({
            loading: '',
            survey,
        }))
        .catch(err => this.alert(err));
    }

    handleFinishSurvey(e) {
        e.preventDefault();

        const { survey } = this.state;
        this.setState({ loading: 'Fetching previous question...' });

        Promise.resolve('TODO ANALYTICS')
        .then(() => this.setState({
            loading: '',
            survey,
        }))
        .catch(err => this.alert(err));
    }

    render() {
        const { survey, answers } = this.state;
        const questions = survey.current();


        return (
            <section>
                <pre>step: { this.state.survey.step }, session: { this.state.survey.sessionID }, env: { process.env.NODE_ENV }</pre>

                <pre>{ JSON.stringify(questions) }</pre>

                <div className="jumbotron jumbotron-fluid">
                    <div className="container">

                        <LoadingSpinner loading={ this.state.loading } message={ this.state.loading }/>

                        <form id={ Date.now() /*trickout autofill*/ }>
                            <input type="hidden" value={ Date.now() /*trick autofill*/ } />

                            { this.state.messages.map((entry, index) => <Alert key={ index } status={ entry.status } message={ entry.message } />) }

                            {
                                questions.map((question, index) => {
                                    switch(question.type) {
                                        case 'radiogroup':
                                            return <RadioGroup
                                                key={ index }
                                                question={ question }
                                                handleChange={ this.handleChange.bind(this) } />

                                        case 'geolocation':
                                            return <GeoLocation
                                                key={ index }
                                                question={ question }
                                                handleChange={ this.handleChange.bind(this) } />

                                        case 'datetime':
                                            return <PeriodRange
                                                key={ index }
                                                question={ question }
                                                handleChange={ this.handleChange.bind(this) } />

                                        default:
                                            return <TextInput
                                                key={ index }
                                                question={ question }
                                                handleChange={ this.handleChange.bind(this) } />

                                    }

                                })
                            }

                            { (survey.step <= 0 && questions.length) ?
                                <div>
                                    <button type="submit" className="btn btn-default btn-primary btn-arrow-right"
                                        onClick={ this.handleUpdateAnswers.bind(this) }>Next Question</button>
                                </div>
                            : null }

                            { (!survey.finished && survey.step > 0 && questions.length) ?
                                <div>
                                    <button type="submit" className="btn btn-default btn-arrow-left"
                                        onClick={ this.handleDelAnswer.bind(this) }>Previous Question</button>
                                    <button type="submit" className="btn btn-default btn-primary btn-arrow-right"
                                        onClick={ this.handleUpdateAnswers.bind(this) }>Next Question</button>
                                </div>
                            : null }

                            { (survey.finished) ?
                                <div>
                                    <button type="submit" className="btn btn-default btn-primary"
                                        onClick={ this.handleFinishSurvey.bind(this) }>Finish Survey</button>
                                </div>
                            : null }

                        </form>

                    </div>

                    <Dev label="survey" data={ survey } open={ true }/>
                    <Dev label="questions" data={ questions } open={ true }/>
                    <Dev label="answers" data={ answers } open={ true }/>

                </div>
            </section>
        );
    }
}

Survey.propTypes = {
};

export default Survey;
