import React from 'react';
import { BrowserRouter as Router, Route, Switch, Redirect, withRouter } from 'react-router-dom';

// scaffolding
import HeaderNav from './HeaderNav';
import Footer from './Footer';

// views
import Survey from './Survey';
import Analysis from './Analysis';
import Page404 from './Page404';
import Surveys from './Surveys';

import Demo from './demo/Demo';
import DemoAnalysis from './demo/DemoAnalysis';
import DemoManifest from './demo/DemoManifest';

const Navigation = withRouter(HeaderNav);

// config

const surveys = process.env.REACT_APP_SURVEY_LIST.split(',').map(name => name.trim());
const surveyProvider = process.env.REACT_APP_SURVEY_PROVIDER.trim();
const siteName = process.env.REACT_APP_SITE_NAME.trim();

const App = function(props) {
    return (
        <Router>
            <React.Fragment>
                <Navigation surveys={ surveys } siteName={ siteName } surveyProvider={ surveyProvider }/>
                <main className="container" style={{ marginTop: '60px' /*fixed header*/ }}>
                    <Switch>
                        <Route exact path="/" render={ props => (surveys.length) ? <Redirect to={ `/survey/${surveys[0]}` } /> : <Surveys /> } />
                        {
                            surveys.length > 1 &&
                                <Route path="/surveys" render={ () => <Surveys surveys={ surveys } surveyProvider={ surveyProvider } /> } />
                        }
                        <Route path="/demo/form/:component?" component={ Demo } />
                        <Route path="/demo/analyse" component={ DemoAnalysis } />
                        <Route path="/demo/manifest" component={ DemoManifest } />
                        <Route path="/analyse/:id/:sessionID" component={ Analysis } />
                        <Route path="/survey/:id/:sessionID?" component={ Survey } />
                        <Route path="*" component={ Page404 } />
                    </Switch>
                </main>
                <Footer surveyProvider={ surveyProvider } />
            </React.Fragment>
        </Router>
    );
};

App.propTypes = {};

export default App;
