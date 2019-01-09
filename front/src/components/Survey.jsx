import React, { Component } from 'react';

import { Redirect } from 'react-router-dom';

// apis
import api, { BaseUri } from '../api';
import { serializeAnswer, mapTypeToField } from '../serializer';
import { validateAnswer } from '../validator';
import SurveyManager from '../SurveyManager';
import LocalStorage from '../storage/LocalStorage';
import Log from '../Log';

import TextInput from './form/TextInput';
import RadioGroup from './form/RadioGroup';
import CheckboxGroup from './form/CheckboxGroup';
import Checkbox from './form/Checkbox';
import GeoLocation from './form/GeoLocation';
import PeriodRangeSlider from './form/PeriodRangeSlider';
import NumberInput from './form/NumberInput';
import Textarea from './form/Textarea';
import HiddenInput from './form/HiddenInput';
import EmailInput from './form/EmailInput';
import PasswordInput from './form/PasswordInput';

// components
import LoadingSpinner from './LoadingSpinner';
import Alert from './Alert';

import { FormRow, FieldValidator } from './FormHelpers';

// devel
import Dev from './Dev';

// config
const CACHE_KEY = process.env.REACT_APP_SURVEYCACHEKEY;

class Survey extends Component {
    constructor(props) {
        super(props);
        const surveyID = props.match.params.id;

        this.state = {
            survey: new SurveyManager(surveyID, BaseUri),
            answers: {}, // TODO consider moving answer map into SurveyManager as well

            loading: '',
            alerts: [],
            // TODO tmp
            // TODO if ready, include resetting in handle responses
            evaluation: null,  // TODO remove
        };
    }

    componentDidMount() {
        const { survey } = this.state;

        const cached = LocalStorage.get(CACHE_KEY);

        if(!survey.canMerge(cached)) {
            this.initNewSession();
            return;
        }

        survey.merge(cached);
        this.setState({
            loading: '',
            survey,
            answers: {},
        });
    }

    /**
     * Sends a message or an Error to this.state.alerts
     * This method ist tpically used by the request api in case of response or network errors
     * @param {Error} error
     * @returns {void}
     */
    alert(message, severity) {
        const { alerts } = this.state;

        if(severity !== 'error' && message instanceof Error) {
            severity = 'error';
        }

        const entry = Log.add(message, severity);
        alerts.push(entry);

        this.setState({
            loading: '',
            alerts,
        });
    }

    ////
    // form processing
    ////

    handleChange(element, question, ...values) {
        const { answers } = this.state;
        const { id, type } = question;

        let fn = null;
        let error = null;
        let answer = (typeof answers[id] !== 'undefined') ? answers[id].answer : null;

        // check if callback returns an error
        // validate element constraints

        // validate value
        error = validateAnswer(element, question, ...values);

        // get serializer callback
        if(!error)  {
            fn = mapTypeToField(type);
        }

        if (fn && fn instanceof Error) {
            error = fn;
        }

        if (!error && typeof fn === 'function') {
            answer = fn(...values);
        }

        answers[id] = {
            answer,
            values,
            error,
            question,
        };

        this.setState({
            answers,
        });
    }

    getFormErrors() {
        return Object.keys(this.state.answers).filter(answer => typeof answer.error !== 'undefined' && answer.error)
    }

    ////
    // form submissions
    ////

    initNewSession(e) {
        e && e.preventDefault();

        const { survey } = this.state;
        this.setState({ loading: 'Initializing survey...' });

        api.createNewSession(survey.surveyID)
        .then(sessID => survey.init(sessID))
        .then(() => api.nextQuestion(survey.sessionID))
        .then(response => survey.add(response.next_questions))
        .then(() => this.setState({
            loading: '',
            survey,
            answers: {}, // clear answers
            alerts: [], // clear previous alerts
        }))
        .then(() => LocalStorage.set(CACHE_KEY, survey))
        .catch(err => this.alert(err));
    }

    handleUpdateAnswers(e) {
        e && e.preventDefault();

        const { survey, answers } = this.state;
        this.setState({ loading: 'Sending answer...' });

        //TODO check if errors returned from serializer
        const csvFragments = Object.keys(answers).map((id) => {
            const entry = answers[id];
            const { answer, question } = entry;
            const type = question.type;
            const unit = question.unit;
            return serializeAnswer(id, answer, type, unit);
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
            alerts: [], // clear previous alerts
        }))
        .then(() => LocalStorage.set(CACHE_KEY, survey))
        .catch(err => this.alert(err));
    }

    handleDelAnswer(e) {
        e && e.preventDefault();

        const { survey } = this.state;
        this.setState({ loading: 'Fetching previous question...' });

        // FIRST go back to previous (does not render) and then get questionIds
        survey.back();
        const questionIds = survey.current().map(question => question.id);

        // delete current answer(s)
        // each answer is a single request, we are bundling them into Promise.all
        Promise.all(
            questionIds.map(id => api.deleteAnswer(survey.sessionID, id))
        )
        .then(responses => responses.pop()) // last ?TODO multi elements mode
        .then(response => survey.add(response.next_questions))
        .then(() => this.setState({
            loading: '',
            survey,
            alerts: [], // clear previous alerts
        }))
        .then(() => LocalStorage.set(CACHE_KEY, survey))
        .catch(err => this.alert(err));
    }

    handleFinishSurvey(e) {
        e && e.preventDefault();

        const { survey } = this.state;
        this.setState({ loading: 'Fetching results...' });

        api.finishSurvey(survey.sessionID)
        .then(evaluation => this.setState({
            loading: '',
            survey,
            answers: {}, // clear previous answers
            alerts: [], // clear previous alerts
            evaluation, // TODO remove
        }))
        .then(() => survey.close())
        .then(() => LocalStorage.delete(CACHE_KEY))
        .catch(err => this.alert(err));
    }

    render() {

        const { survey, answers } = this.state;

        if (survey.isFinished()) { // TODO surveymanager method
            return(
                <Redirect to={ {
                    pathname: `/analyse/${survey.surveyID}`,
                    state: { survey }
                    } }
                />
            );
        }

        const questions = survey.current();
        const errors = this.getFormErrors();
        const countAnswers = Object.keys(answers).length;

        return (
            <section>
                <Dev.SurveyBar survey = { survey } />
                <h1>{ survey.surveyID } <small>Step { this.state.survey.step }</small></h1>

                <LoadingSpinner loading={ this.state.loading } message={ this.state.loading }/>
                { this.state.alerts.map((entry, index) => <Alert key={ index } severity={ entry.severity } message={ entry.message } />) }

                <form id={ Date.now() /*trickout autofill*/ } className="list-group">
                    <input type="hidden" value={ Date.now() /*trick autofill*/ } />

                    {
                        /* show if survey is finished but not closed yet */
                        (survey.finished && !survey.closed) ?
                        <FormRow className="list-group-item text-white bg-success">
                            <h2> <i className="fas fa-check-circle"></i> Survey completed.</h2>
                            Thank you for your time!
                        </FormRow>
                        : null
                    }

                    {
                        !this.state.loading && questions.map((question, index) => {

                            const answer = this.state.answers[question.id] || null;

                            switch(question.type) {
                                case 'MULTICHOICE':
                                    return <FormRow key={ index } className="list-group-item" legend={ question.name } description={ question.title_text }>
                                        <RadioGroup
                                            question={ question }
                                            handleChange={ this.handleChange.bind(this) }
                                            required />
                                        <FieldValidator answer={ answer } />
                                    </FormRow>

                                case 'MULTISELECT':
                                    return <FormRow key={ index } className="list-group-item" legend={ question.name } description={ question.title_text }>
                                        <CheckboxGroup
                                            question={ question }
                                            handleChange={ this.handleChange.bind(this) }
                                            required />
                                        <FieldValidator answer={ answer } />
                                    </FormRow>

                                case 'LATLON':
                                    return <FormRow key={ index } className="list-group-item" legend={ question.name } description={ question.title_text }>
                                        <GeoLocation
                                            value={ (answer) ? answer.values : '' }
                                            question={ question }
                                            handleChange={ this.handleChange.bind(this) }
                                            required />
                                        <FieldValidator answer={ answer } />
                                    </FormRow>

                                // TODO DAYTIME slider/select
                                // TODO RadioMatrix number/text

                                case 'TIMERANGE':
                                    return <FormRow key={ index } className="list-group-item" legend={ question.name } description={ question.title_text }>
                                        <PeriodRangeSlider
                                            value={ (answer) ? answer.values : '' }
                                            question={ question }
                                            handleChange={ this.handleChange.bind(this) }
                                            required />
                                        <FieldValidator answer={ answer } />
                                    </FormRow>

                                case 'INT':
                                case 'FIXEDPOINT':
                                    return <FormRow key={ index } className="list-group-item" legend={ question.name } description={ question.title_text }>
                                        <NumberInput
                                            value={ (answer) ? answer.values[0] : null }
                                            question={ question }
                                            handleChange={ this.handleChange.bind(this) }
                                            required />
                                        <FieldValidator answer={ this.state.answers[question.id] || null } />
                                    </FormRow>

                                case 'TEXTAREA':
                                    return <FormRow key={ index } className="list-group-item" legend={ question.name } description={ question.title_text }>
                                        <Textarea
                                            value={ (answer) ? answer.values[0] : null }
                                            question={ question }
                                            handleChange={ this.handleChange.bind(this) }
                                            required />
                                        <FieldValidator answer={ this.state.answers[question.id] || null } />
                                    </FormRow>

                                // html slide
                                // no value!
                                // no validation!
                                // not required!
                                case 'HIDDEN':
                                    return <FormRow key={ index } className="list-group-item" legend={ question.name } description={ question.title_text }>
                                        <HiddenInput
                                            question={ question }
                                            defaultValue={ question.default_value || 'visited' /* TODO confirm with backend */ }
                                            handleChange={ this.handleChange.bind(this) } />
                                    </FormRow>

                                case 'EMAIL':
                                    return <FormRow key={ index } className="list-group-item" legend={ question.name } description={ question.title_text }>
                                        <EmailInput
                                            value={ (answer) ? answer.values[0] : null }
                                            question={ question }
                                            handleChange={ this.handleChange.bind(this) }
                                            required />
                                        <FieldValidator answer={ this.state.answers[question.id] || null }
                                        />
                                    </FormRow>

                                // not required!
                                case 'CHECKBOX':
                                    return <FormRow key={ index } className="list-group-item" legend={ question.name } description={ question.title_text }>
                                        <Checkbox
                                            question={ question }
                                            handleChange={ this.handleChange.bind(this) }/>
                                    </FormRow>

                                case 'PASSWORD':
                                    return <FormRow key={ index } className="list-group-item" legend={ question.name } description={ question.title_text }>
                                        <PasswordInput
                                            value={ (answer) ? answer.values[0] : null }
                                            question={ question }
                                            handleChange={ this.handleChange.bind(this) }
                                            required />
                                        <FieldValidator answer={ this.state.answers[question.id] || null } />
                                    </FormRow>

                                default:
                                    return  <FormRow key={ index } className="list-group-item" legend={ question.name } description={ question.title_text }>
                                        <TextInput
                                            value={ (answer) ? answer.values[0] : null }
                                            question={ question }
                                            handleChange={ this.handleChange.bind(this) }
                                            required />
                                        <FieldValidator answer={ this.state.answers[question.id] || null } />
                                    </FormRow>
                            }

                        })
                    }

                    <FormRow className="list-group-item bg-light">
                        {
                            (survey.step <= 0 && questions.length) ?
                                <button type="submit" className="app--btn-arrow btn btn-default btn-primary"
                                    disabled={ errors.length || !countAnswers }
                                    onClick={ this.handleUpdateAnswers.bind(this) }>Next Question <i className="fas fa-arrow-circle-right"></i></button>
                            : null
                        }

                        {
                            (!survey.finished && survey.step > 0 && questions.length) ?
                            <div>
                                <button type="submit" className="app--btn-arrow btn btn-default"
                                    onClick={ this.handleDelAnswer.bind(this) }><i className="fas fa-arrow-circle-left"></i> Previous Question</button>
                                <button type="submit" className="app--btn-arrow btn btn-default btn-primary"
                                    disabled={ errors.length || !countAnswers }
                                    onClick={ this.handleUpdateAnswers.bind(this) }>Next Question <i className="fas fa-arrow-circle-right"></i></button>
                            </div>
                            : null
                        }

                        {
                            (survey.finished) ?
                            <button type="submit" className="btn btn-default btn-primary"
                                onClick={ this.handleFinishSurvey.bind(this) }>Finish Survey</button>
                            : null
                        }
                    </FormRow>

                </form>

                <Dev.Pretty label="survey" data={ survey } open={ false }/>
                <Dev.Pretty label="questions" data={ questions } open={ false }/>
                <Dev.Pretty label="answers" data={ answers } open={ false }/>

            </section>
        );
    }
}

Survey.propTypes = {
};

export default Survey;
