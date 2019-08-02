import React from 'react';

export const AppContext = React.createContext({
    breakpoint: '',
    isBreakpointAbove: () => {},
});

export const SurveyContext = React.createContext({
    session_id: '',
    survey_id: '',
});
