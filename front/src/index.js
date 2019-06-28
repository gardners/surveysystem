import React from 'react';
import ReactDOM from 'react-dom';

import App from './components/App';

// styles
import './styles/main.scss';

// icons
import '@fortawesome/fontawesome-free/css/solid.min.css';
import '@fortawesome/fontawesome-free/css/regular.min.css';
import '@fortawesome/fontawesome-free/css/fontawesome.min.css';

// theme
const urlParams = new URLSearchParams(window.location.search);
const themeParam = urlParams.get('theme');
const theme = (themeParam) ? themeParam : (process.env.REACT_APP_SITE_THEME) ? process.env.REACT_APP_SITE_THEME : 'default';

// webpack dynamic imports
import(`./styles/${theme}/index.scss`).then(() => {
    /* ... */
});


ReactDOM.render(<App />, document.getElementById('root'));
