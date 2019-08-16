import React, { Component } from 'react';
import { BrowserRouter as Router, Route, Switch, Redirect, withRouter } from 'react-router-dom';

import { DEFAULT_BREAKPOINT, testMediaBreakpoint, isBreakpointAbove } from '../Media';
import { AppContext } from '../Context';

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

const {
    REACT_APP_SURVEY_PROVIDER,
    REACT_APP_SITE_NAME,
    REACT_APP_SURVEY_LIST,
} = process.env;
// config

const surveyProvider = REACT_APP_SURVEY_PROVIDER.trim();
const siteName = REACT_APP_SITE_NAME.trim();

// get surveys TODO: context
const surveyIds = REACT_APP_SURVEY_LIST.split(',').map(name => name.trim());

////
//
////

class App extends Component {

    constructor(props) {
        super(props);
        this.state = {
            appContext: {
                breakpoint: DEFAULT_BREAKPOINT,
                isBreakpointAbove,
            },
        }
        window.addEventListener('resize', this.onWindowResize.bind(this));
    }

    onWindowResize() {
        testMediaBreakpoint(bp => {
            if(bp !== this.state.breakpoint) {
                this.setState({
                    appContext: {
                        breakpoint: bp,
                        isBreakpointAbove,
                    },
                });
            }
        });
    }

    render() {
        const { appContext } = this.state;

        return (
            <Router>
                <AppContext.Provider value={ appContext }>
                    <Navigation siteName={ siteName } surveyProvider={ surveyProvider }/>
                    <main className="container" style={{ marginTop: '60px' /*fixed header*/ }}>
                        <Switch>
                            <Route exact path="/" render={ () => <Redirect to={ `/surveys` } /> } />
                            <Route path="/surveys" render={ () => <Surveys surveyIds={ surveyIds } /> } />
                            <Route path="/demo/form/:component?" component={ Demo } />
                            <Route path="/demo/analyse" component={ DemoAnalysis } />
                            <Route path="/demo/manifest" component={ DemoManifest } />
                            <Route path="/analyse/:id/:sessionID" component={ Analysis } />
                            <Route path="/survey/:id/:sessionID?" component={ Survey } />
                            <Route path="*" component={ Page404 } />
                        </Switch>
                    </main>
                    <Footer surveyProvider={ surveyProvider } />
                </AppContext.Provider>
            </Router>
        );
    }
};

App.propTypes = {};

export default App;
