import React from 'react';


export const AppContext = React.createContext({
    breakpoint: '',
    isBreakpointAbove: () => {},
});
