import React from 'react';

import { getMediaBreakpoint } from './Media';

const surveys = process.env.REACT_APP_SURVEY_LIST.split(',').map(name => name.trim());
const surveyProvider = process.env.REACT_APP_SURVEY_PROVIDER.trim();
const siteName = process.env.REACT_APP_SITE_NAME.trim();

// theme
const urlParams = new URLSearchParams(window.location.search);
const themeParam = urlParams.get('theme');

const theme = (themeParam) ? themeParam : (process.env.REACT_APP_SITE_THEME) ? process.env.REACT_APP_SITE_THEME : 'default';

export const AppContext = React.createContext({
    theme,
    surveys,
    surveyProvider,
    siteName,
    breakPoint: getMediaBreakpoint(),
});
