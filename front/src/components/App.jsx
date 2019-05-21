import React, { useState } from 'react';

import { BrowserRouter as Router, Route, Switch, Redirect, withRouter } from 'react-router-dom';
import CssBaseline from '@material-ui/core/CssBaseline';
import Container from '@material-ui/core/Container';

import { createMuiTheme } from '@material-ui/core/styles';
import { ThemeProvider } from '@material-ui/styles';

import LocalStorage from '../storage/LocalStorage';

// scaffolding
import HeaderNav from './HeaderNav';
import Footer from './Footer';

// views
import Survey from './Survey';
import Analysis from './Analysis';
import Page404 from './Page404';
import Surveys from './Surveys';

import DemoForm from './demo/DemoForm';
import DemoAnalysis from './demo/DemoAnalysis';
import DemoManifest from './demo/DemoManifest';

const Navigation = withRouter(HeaderNav);

// config

const surveys = process.env.REACT_APP_SURVEY_LIST.split(',').map(name => name.trim());
const surveyProvider = process.env.REACT_APP_SURVEY_PROVIDER.trim();
const siteName = process.env.REACT_APP_SITE_NAME.trim();

let themeType = LocalStorage.get('ss-theme-type');

//@see https://next.material-ui.com/customization/default-theme/?expend-path=$.typography
const baseTheme = createMuiTheme({
    palette: {
        type: (themeType && (themeType === 'light' || themeType === 'dark')) ? themeType : 'light',
    },
    typography: {
        h1: {
            fontSize: '4rem' //scale down
        }
    }
});

const App = function(props) {

    const [theme, setTheme] = useState(baseTheme);

    return (
        <Router>
            <ThemeProvider theme={ theme }>
                <CssBaseline />
                <Navigation
                    surveys={ surveys }
                    siteName={ siteName }
                    surveyProvider={ surveyProvider }
                    themeType={ theme.palette.type }
                    toggleThemeType={ () => {
                        const type = (theme.palette.type === 'light') ? 'dark' : 'light';
                        const updated = createMuiTheme({
                            palette: {
                                type,
                            }
                        });
                        LocalStorage.set('ss-theme-type', type);
                        setTheme(updated);
                    } }
                />
                <Container maxWidth="lg">
                    <main className="container" style={{ marginTop: '60px' /*fixed header*/ }}>
                        <Switch>
                            <Route exact path="/" render={ props => (surveys.length) ? <Redirect to={ `/survey/${surveys[0]}` } /> : <Surveys /> } />
                            {
                                surveys.length > 1 &&
                                    <Route path="/surveys" render={ () => <Surveys surveys={ surveys } surveyProvider={ surveyProvider } /> } />
                            }
                            <Route path="/demo/form/:component?" component={ DemoForm } />
                            <Route path="/demo/analyse" component={ DemoAnalysis } />
                            <Route path="/demo/manifest" component={ DemoManifest } />
                            <Route path="/analyse/:id/:sessionID" component={ Analysis } />
                            <Route path="/survey/:id/:sessionID?" component={ Survey } />
                            <Route path="*" component={ Page404 } />
                        </Switch>
                    </main>
                    <Footer surveyProvider={ surveyProvider } />
                </Container>
            </ThemeProvider>
        </Router>
    );
};

App.propTypes = {};

export default App;
