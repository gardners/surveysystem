import React, { Component } from 'react';
import PropTypes from 'prop-types';

import LocalStorage from '../storage/LocalStorage';
import RestartSurveyButton from './survey/RestartSurveyButton';

// config
const CACHE_KEY = process.env.REACT_APP_SURVEY_CACHEKEY;
const NODE_ENV = process.env.NODE_ENV;

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

export default {
    Pretty,
    SurveyBar,
};
