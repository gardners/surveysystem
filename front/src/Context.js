import React from 'react';

import { is_protected, init, exit_authorize, remove_token, get_user, verify_token, callback_authorize } from './OAuth2';

export const AppContext = React.createContext({
    breakpoint: '',
    matchesBreakpointOrAbove: () => {},
});

export const SurveyContext = React.createContext({
    sessionID: '',
    surveyID: '',
});

const authContextModel = function() {
    return {
        protected: false,
        user: null, // token was verified against provider
        init: () => Promise.resolve(null),
        unexpired: () => true,
        login: () => {},
        logout: () => {},
        redirect_callback: () => '',
    }
};

export const AuthContext = React.createContext(authContextModel());

export const createAuthContext = function() {

    const model = authContextModel();
    if(!is_protected()) {
        return model;
    }

    return Object.assign(model, {
        protected: true,
        init,
        unexpired: verify_token,
        login: exit_authorize,
        logout: () => {
            remove_token();
            window.location.reload();
        },
        getUser: get_user,
        redirect_callback: callback_authorize,
    });
};
