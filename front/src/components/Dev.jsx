import React, { Component } from 'react';
import PropTypes from 'prop-types';

import LocalStorage from '../storage/LocalStorage';

// config
const CACHE_KEY = process.env.REACT_APP_SURVEYCACHEKEY;

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
    title: PropTypes.string,
    open: PropTypes.bool,
};

const ClearCachedSurveyButton = function(props) {

    if(!LocalStorage.get(CACHE_KEY)) {
        return (null);
    }

    return(
        <span role="menuitem" className={ props.className } onClick={ () => {
            // no e.preventDefault(); we DO want to refresh here
            LocalStorage.delete(CACHE_KEY);
            window.location.reload();
        } }>Clear LocalStorage</span>
    );
};

ClearCachedSurveyButton.propTypes = {
    className: PropTypes.string,
};

const SurveyBar = function(props) {
    const { survey } = props;

    if(!survey) {
        return (null);
    }

    return(
        <pre className={ props.className }>
            step: { survey.step }, session: { survey.sessionID }, env: { process.env.NODE_ENV }
            { <ClearCachedSurveyButton className="text-primary ml-2" /> }
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
