import React from 'react';


const AppContext = React.createContext({
    breakpoint: '',
    matchesBreakpointOrAbove: () => {},
    debug: false,
});

const SurveyContext = React.createContext({
    session_id: '',
    survey_id: '',
});

const AuthContext = React.createContext({});

export {
    AppContext,
    SurveyContext,
    AuthContext,
};
