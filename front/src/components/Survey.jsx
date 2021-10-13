import React, { Component } from 'react';

// data
import Api from '../Api';
import  { mapQuestionGroups } from '../Question';
import  { isArray } from '../Utils';
import Answer from '../Answer';
import Session, { load_cached_session, save_cached_session } from '../Session';

// context
import { SurveyContext } from '../Context';

// form components
import SurveySection from './survey/SurveySection';
import SurveyForm from './survey/SurveyForm';
import Question from './survey/Question';
import QuestionGroup from './survey/QuestionGroup';
import FeedbackItem from './survey/FeedbackItem';
import SurveyButtons from './survey/SurveyButtons';
import SurveyMessage from './survey/SurveyMessage';
import SurveyProgress from './survey/SurveyProgress';
import FinishSurvey from './survey/FinishSurvey';

// misc components
import Preloader from './Preloader';
import ApiAlert from './ApiAlert';

// devel
import Dev from './Dev';

class Survey extends Component {
    constructor(props) {
        super(props);
        const { match } = this.props;
        const survey_id = match.params.id;

        this.state = {
            session: new Session(survey_id, null),
            loading: '',
            answers: {},
            errors: {},
            alerts: [],
        };
    }

    componentDidMount() {
        const cached = load_cached_session();
        if (cached) {
            this.initNextQuestion(cached);
        } else {
            const { session } = this.state;
            this.initNewSession(session);
        }
    }

    /**
     * Tear down component, cancel running async processes
     * #441
     */
    componentWillUnmount() {
        // Api.cancelRequests();
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
        alerts.push(message);

        this.setState({
            loading: '',
            alerts,
        });
    }

    ////
    // form processing
    ////

    /**
     * Callback for question form elements,
     * - receive,
     * - validate
     * - serialize answer,
     * - store serialized answer in this.state.answers
     *
     * @param {HTMLElement|null} element
     * @param {Question} question
     * @param {mixed} value
     * @returns {void}
     */
    handleChange(element, question, value) {
        const { answers, errors } = this.state;
        const { id } = question;

        delete(errors[id]);

        // @see https://developer.mozilla.org/en-US/docs/Web/API/ValidityState
        if(element && typeof element.validity !== 'undefined') {
            if (!element.validity.valid) {
                errors[id] = new Error (element.validationMessage);
                this.setState({
                    errors,
                });
                return;
            }
        }

        const answer = Answer.setValue(question, value);
        if (answer instanceof Error) {
            errors[id] = answer;
            this.setState({
                errors,
            });
            return;
        }

        const serialized = Answer.serialize(answer);// can be instanceof Error
        if (serialized instanceof Error) {
            errors[id] = answer;
            this.setState({
                errors,
            });
            return;
        }

        // set answer
        answers[id] = serialized;

        this.setState({
            errors,
            answers,
        });

    }

    ////
    // form submissions
    ////

    /**
     * Create a new session
     *
     * @param {object} session Session instance
     * @returns {void}
     */
    initNewSession(session) {
        this.setState({ loading: 'Initializing survey...' });

        // 1. request a new session id
        // 2. initialize a new survey insatance
        // 3. fetch the first question set andd add it to survey instance

        Api.createNewSession(session.survey_id)
        .then(session_id => session.merge({ session_id }))
        .then(() => save_cached_session(session))
        .then(() => Api.nextQuestion(session.session_id))
        .then(response => session.merge(response))
        .then(() => save_cached_session(session))
        .then(() => this.setState({
            loading: '',
            session,
            answers: session.getDefaultAnswers(),
            errors: {}, // clear errors
            alerts: [], // clear alerts
        }))
        .catch(err => this.alert(err));
    }

    /**
     * Initialize a previously cached session
     *
     * @param {object} session Session instance
     * @returns {void}
     */
    initNextQuestion(session) {
        this.setState({ loading: 'Initializing survey...' });

        // 1. fetch the current question set, providing a previoiusly cached sessionid
        // 2. add question set to survey instance, (survey.add() will internally evaluate if the new question set matches the old one - based on ids - and skip adding in thast case)

        Api.nextQuestion(session.session_id)
        .then(response => session.merge(response))
        .then(() => save_cached_session(session))
        .then(() => this.setState({
            loading: '',
            session,
            answers: session.getDefaultAnswers(),
            errors: {}, // clear errors
            alerts: [], // clear alerts
        }))
        .catch(err => this.alert(err));
    }

    /**
     * Load an (existing) session by survey id and session id
     *
     * @param {string} survey_id
     * @param {string} session_id
     * @returns {void}
     */
    loadSession(survey_id, session_id) {
        this.setState({ loading: 'Load session...' });

        // technically we don't need the survey id to fetch a session
        const session = new Session(survey_id, session_id);
        this.initNextQuestion(session);
    }

    /**
     * Submit answers
     *
     * @param {Event}
     * @returns {void}
     */
    handleAddAnswers(e) {
        e && e.preventDefault();

        const { session, answers, errors } = this.state;
        this.setState({ loading: 'Sending answer...' });

        if(Object.keys(errors).length) {
            this.alert('Send answers: Errors found!', 'error');
            return;
        }

        // create an ordered sequence of answer ID based on the current questions array
        const question_ids = session.getQuestionIds();
        if(question_ids.length !== Object.keys(answers).length) {
            this.alert('Send answers: Missing answers!', 'error');
            return;
        }

        // 1. collect answers csv fragment strings from state
        // 2. send answers (step by step)
        // 3. the last next_questions[] response will be the new survey form state, add it to survey instance

        const csvFragments = question_ids.map(id => answers[id]);

        Api.addAnswers(session.session_id, csvFragments)
        .then(response => session.merge(response))
        .then(() => save_cached_session(session))
        .then(() => this.setState({
            loading: '',
            session,
            answers: session.getDefaultAnswers(),
            errors: {}, // clear errors
            alerts: [], // clear alerts
        }))
        .catch(err => this.alert(err));
    }

    /**
     * Delete last answers, fetch updated question set (now with default values!) and set this as the current question set
     *
     * @param {Event}
     * @returns {void}
     */
    handleDelAnswer(e) {
        e && e.preventDefault();
        const { session } = this.state;

        if(session.isFirst()) {
            this.alert('No question found to delete', 'error');
            return;
        }

        this.setState({ loading: `Deleting previous answer...` });

        Api.deletePreviousAnswer(session.session_id)
        .then(response => session.merge(response))
        .then(() => save_cached_session(session))
        .then(() => this.setState({
            loading: '',
            session,
            answers: session.getDefaultAnswers(),
            errors: {}, // clear errors
            alerts: [], // clear alerts
        }))
        .catch(err => this.alert(err));
    }

    /**
     * redirect to analysis
     *
     * @param {Event}
     * @returns {void}
     */
    handleGetAnalysis(e) {
        e && e.preventDefault();
        const { session } = this.state;
        const { history } = this.props;

        if (!session.isFinished()) {
            return;
        }
        // the session will be closed in <Analysis>
        history.push(`/analyse/${session.survey_id}/${session.session_id}`);
    }

    render() {
        const { session, errors, answers, loading, alerts } = this.state;

        if (!session) {
            return (null);
        }

        const { session_id, survey_id, next_questions } = session;

        // compile groups
        const withGroups = mapQuestionGroups(next_questions);

        const answersCount = Object.keys(answers).length;
        const errorCount = Object.keys(errors).length;

        const hasQuestions = next_questions.length > 0;
        const hasErrors = errorCount > 0;
        const hasAnswers = answersCount > 0;
        const hasAllAnswers = (answersCount === next_questions.length);

        const isCompleted = session.isCompleted();

        return (
            <React.Fragment>

                <SurveySection session={ session }>
                    <Preloader loading={ loading } message={ loading }/>

                    {
                        (alerts.length) ?
                            <React.Fragment>
                                Unfortunately we encountered an error sending your data.
                                { alerts.map((entry, index) => <ApiAlert key={ index } error={ entry } />) }
                            </React.Fragment> : null
                    }

                    {
                        /* #379 show if survey is finished but not closed yet */
                        (isCompleted) ?
                            <FinishSurvey
                                session = { session }
                                handleGetAnalysis = { this.handleGetAnalysis.bind(this) }
                                handleDelAnswer = { this.handleDelAnswer.bind(this) }
                            />
                        : null
                    }

                    <SurveyContext.Provider value={ {
                        survey_id,
                        session_id,
                    } }>
                        <SurveyMessage session={ session } />
                        <SurveyProgress className="mb-2" session={ session }/>

                        <SurveyForm
                            show={ !isCompleted && !loading }
                            className="list-group"
                        >
                            {
                                withGroups.map((entry, index) => {

                                    if(isArray(entry)) {
                                        return (
                                            <QuestionGroup
                                                key={ index }
                                                className="list-group-item"

                                                handleChange={ this.handleChange.bind(this) }
                                                questions={ entry }
                                                errors={ errors }
                                            />
                                        );
                                    }

                                    return (
                                        <Question
                                            key={ index }
                                            className="list-group-item"

                                            handleChange={ this.handleChange.bind(this) }
                                            question={ entry }
                                            error={ errors[entry.id] || null }
                                        />
                                    );

                                })
                            }

                            {
                                hasErrors && <FeedbackItem className="list-group-item list-group-item-danger">Please fix errors above</FeedbackItem>
                            }

                            {
                                !hasAllAnswers && <FeedbackItem className="list-group-item list-group-item-warning">{ (next_questions.length > 1) ? 'Please answer all questions': 'Answer required' }</FeedbackItem>
                            }

                            <SurveyButtons
                                className="list-group-item pt-4 pb-4"
                                handlePrev={ this.handleDelAnswer.bind(this) }
                                handleNext={ this.handleAddAnswers.bind(this) }

                                hasQuestions={ hasQuestions }
                                hasErrors={ hasErrors }
                                hasAnswers={ hasAnswers }
                                hasAllAnswers={ hasAllAnswers }
                                isFirst={ session.isFirst() }
                            />
                        </SurveyForm>
                    </SurveyContext.Provider>
                </SurveySection>

                <Dev.IfDebug>
                    <Dev.SurveyBar className="bg-light p-1" session={ session } loadSessionCallback={ this.loadSession.bind(this) } />
                    <Dev.Pretty label="cache" data={ localStorage.getItem(process.env.REACT_APP_SURVEY_CACHEKEY) } open={ false } />
                    <Dev.Pretty label="session" data={ session } open={ false } />
                    <Dev.Pretty label="questions" data={ next_questions } open={ false } />
                    <Dev.Pretty label="answers" data={ answers } open={ false } />
                    <Dev.Pretty label="errors" data={ errors } open={ false } />
                </Dev.IfDebug>

            </React.Fragment>
        );
    }
}

Survey.propTypes = {
};

export default Survey;
