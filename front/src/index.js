import React from 'react';
import ReactDOM from 'react-dom';
import 'react-app-polyfill/ie11';

import App from './components/App';

// styles
import './styles/main.scss';

// icons
import '@fortawesome/fontawesome-free/css/solid.min.css';
import '@fortawesome/fontawesome-free/css/regular.min.css';
import '@fortawesome/fontawesome-free/css/fontawesome.min.css';

// theme
const { REACT_APP_SITE_THEME } = process.env;
const theme = (REACT_APP_SITE_THEME) ? REACT_APP_SITE_THEME : 'default';

// webpack dynamic imports
import(`./styles/${theme}/index.scss`).then(() => {
    /* ... */
});


ReactDOM.render(<App />, document.getElementById('root'));
