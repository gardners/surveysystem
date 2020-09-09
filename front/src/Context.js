import React from 'react';

export const AppContext = React.createContext({
    breakpoint: '',
    matchesBreakpointOrAbove: () => {},
});

export const SurveyContext = React.createContext({
    sessionID: '',
    surveyID: '',
});

export const AuthContext = React.createContext({
    protected: () => {},
    user: '',
});
