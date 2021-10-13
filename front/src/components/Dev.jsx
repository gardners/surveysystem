import React, { Component, useContext } from 'react';
import PropTypes from 'prop-types';

import { AppContext, AuthContext } from '../Context';

import RestartSurveyButton from './survey/RestartSurveyButton';

const serialiseJson = function(data) {
    let json = null;
    try {
        json = JSON.stringify(data, null, 4);
    } catch(e) {
        json = '[PARSER ERROR] ' + e.toString();
    }
    return json;
};

const IfDebug = function({ children }) {
    return(
        <AppContext.Consumer>
        {
            ({ debug }) => (
                (debug) ? children : null
            )
        }
        </AppContext.Consumer>
    );
};

class Pretty extends Component {

    constructor(props) {
        super(props);
        this.state = { open: props.open || false };
    }

    toggle(e) {
        e.preventDefault();
        this.setState({
            open: !this.state.open
        });
    }

    render() {
        const { data, label } = this.props;
        const { open } = this.state;

        return (
            <div>
                <span role="menuitem" onClick={ this.toggle.bind(this) }>{ (open) ? '[-]' : '[+]' } { label }</span>
                { open &&<pre>{ serialiseJson(data) }</pre> }
            </div>
        );
    }
}

Pretty.propTypes = {
    data: PropTypes.any,
    label: PropTypes.string,
    open: PropTypes.bool,
};

const SurveyBar = function({ session, loadSessionCallback, className }) {

    if (!session) {
        return (null);
    }
    const { survey_id, session_id } = session;
    const input  = React.createRef();

    return(
        <React.Fragment>
            <pre className={ className }>
                <input type="text" style={ { width: '36ch' } } defaultValue={ session_id } ref={ input } />
                <button className="btn btn-link btn-sm" onClick={
                    (e) => {
                        e && e.preventDefault();
                        loadSessionCallback(survey_id, input.current.value);
                    }
                }>Load Session</button>
                { <RestartSurveyButton session={ session } className="btn btn-link btn-sm">New Session</RestartSurveyButton> }
            </pre>
        </React.Fragment>
    );
};

SurveyBar.propTypes = {
    className: PropTypes.string,
    loadSessionCallback: PropTypes.func.isRequired,
    session: PropTypes.shape({
        session_id: PropTypes.string,
    })
};

const Question = function(props) {
    const { question } = props;

    if (!question) {
        return (null);
    }

    return(
        <pre className={ props.className }>
            id: { question.id }, type: ({ question.type })
        </pre>
    );
};

Question.defaultProps = {
    question: PropTypes.shape({
        id: '',
        type: '',
    })
};

Question.propTypes = {
    className: PropTypes.string,
    question: PropTypes.shape({
        id: PropTypes.string.isRequired,
        type: PropTypes.string.isRequired,
    })
};

const Auth = function(props) {
    const auth = useContext(AuthContext);

    if (!auth) {
        return (null);
    }

    return(
        <pre className={ props.className }> { JSON.stringify(auth) } </pre>
    );
};

Auth.defaultProps = {};
Auth.propTypes = {
    className: PropTypes.string,
};

// @see eslint import/no-anonymous-default-export
const ex = {
    IfDebug,
    Pretty,
    SurveyBar,
    Question,
    Auth,
};
export default ex;
