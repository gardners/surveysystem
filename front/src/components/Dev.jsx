import React, { Component, useContext } from 'react';
import PropTypes from 'prop-types';

import { AuthContext } from '../Context';

import RestartSurveyButton from './survey/RestartSurveyButton';

const { NODE_ENV } = process.env;

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

        // don't display in production mode
        if (NODE_ENV === 'production') {
            return (null);
        }

        const { data, label } = this.props;
        let json = null;

        try {
            json = JSON.stringify(data, null, 4);
        } catch(e) {
            json = '[PARSER ERROR] ' + e.toString();
        }

        return (
            <div>
                <span role="menuitem" onClick={ this.toggle.bind(this) }>{ (this.state.open) ? '[-]' : '[+]' } { label }</span>
            {
                this.state.open &&

                    <pre>{ json }</pre>

            }
            </div>
        );
    }
}

Pretty.propTypes = {
    data: PropTypes.any,
    label: PropTypes.string,
    open: PropTypes.bool,
};

const SurveyBar = function(props) {
    const { survey } = props;

    // don't display in production mode
    if (NODE_ENV === 'production') {
        return (null);
    }

    if(!survey) {
        return (null);
    }

    return(
        <pre className={ props.className }>
            session: { survey.sessionID }, env: { process.env.NODE_ENV }
            { <RestartSurveyButton className="btn  btn-link btn-sm">Clear LocalStorage</RestartSurveyButton> }
        </pre>
    );
};

SurveyBar.propTypes = {
    className: PropTypes.string,
    survey: PropTypes.shape({
        sessionID: PropTypes.string,
        step: PropTypes.number,
    })
};

const Question = function(props) {
    const { question } = props;

    // don't display in production mode
    if (NODE_ENV === 'production') {
        return (null);
    }

    if(!question) {
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

    if (NODE_ENV === 'production') {
        return (null);
    }

    return(
        <pre className={ props.className }>
            { JSON.stringify(auth) }
        </pre>
    );
};

Auth.defaultProps = {};
Auth.propTypes = {
    className: PropTypes.string,
};

export default {
    Pretty,
    SurveyBar,
    Question,
    Auth,
};
