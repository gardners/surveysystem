import React, { Component } from 'react';
import { BrowserRouter, Route, Switch, Redirect, withRouter } from 'react-router-dom';

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

const Header = withRouter(HeaderNav);

const { REACT_APP_ROUTER_BASENAME } = process.env;
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
            <BrowserRouter basename={ REACT_APP_ROUTER_BASENAME }>
                <AppContext.Provider value={ appContext }>
                    <Header />
                    <main className="container" style={ { marginTop: '60px' /*fixed header*/ } }>
                        <Switch>
                            <Route exact path="/" render={ props => <Redirect to={ `/surveys` } /> } />
                            <Route path="/surveys" component={ Surveys } />
                            <Route path="/demo/form/:component?" component={ Demo } />
                            <Route path="/demo/analyse" component={ DemoAnalysis } />
                            <Route path="/demo/manifest" component={ DemoManifest } />
                            <Route path="/analyse/:id/:sessionID" component={ Analysis } />
                            <Route path="/survey/:id/:sessionID?" component={ Survey } />
                            <Route path="*" component={ Page404 } />
                        </Switch>
                    </main>
                    <Footer />
                </AppContext.Provider>
            </BrowserRouter>
        );
    }
};

App.propTypes = {};

export default App;
