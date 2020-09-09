import React, { Component, useContext } from 'react';
import { BrowserRouter, Route, Switch, Redirect, withRouter } from 'react-router-dom';

import { DEFAULT_BREAKPOINT, testMediaBreakpoint, matchesBreakpointOrAbove } from '../Media';
import { AppContext, AuthContext } from '../Context';

// scaffolding
import HeaderNav from './HeaderNav';
import Footer from './Footer';

// views
import Survey from './Survey';
import Analysis from './Analysis';
import Page404 from './Page404';
import Surveys from './Surveys';
import OAuth2Session from './OAuth2Session';

import Demo from './demo/Demo';
import DemoAnalysis from './demo/DemoAnalysis';
import DemoManifest from './demo/DemoManifest';

// devs
import Dev from './Dev';

const Header = withRouter(HeaderNav);

const { PUBLIC_URL } = process.env;

const ProtectedRoute = function ({ component: Component, ...rest }) {
    const auth = useContext(AuthContext);

    if (!auth.protected()) {
        return (
            <Route {...rest} render={ props => <Component {...props} /> } />
        );
    }

    if (auth.user) {
        return (
            <Route {...rest} render={ props => <Component {...props} /> } />
        );
    }

    return(
        <Redirect to={ {
            pathname: '/login',
            state: { from: rest.location } // remember source
        } } />
    );
};

class App extends Component {

    constructor(props) {
        super(props);

        this.updateUser = (user) => {
            this.setState((state) => {
                const { authContext } = state;
                authContext.user = user;
                return {
                    authContext
                };
            });
        };

        this.state = {
            appContext: {
                breakpoint: DEFAULT_BREAKPOINT,
                matchesBreakpointOrAbove,
            },
            authContext: {
                protected: function() {
                    const id = process.env.REACT_APP_AUTH_CLIENT_ID || null;
                    return !!id;
                },
                user: '',
                updateUser: this.updateUser,
            }
        };
        window.addEventListener('resize', this.onWindowResize.bind(this));
    }

    onWindowResize() {
        testMediaBreakpoint(bp => {
            if(bp !== this.state.breakpoint) {
                this.setState({
                    appContext: {
                        breakpoint: bp,
                        matchesBreakpointOrAbove,
                    },
                });
            }
        });
    }

    render() {
        const { appContext, authContext } = this.state;

        return (
            <BrowserRouter basename={ PUBLIC_URL }>
                <AppContext.Provider value={ appContext }>
                    <AuthContext.Provider value={ authContext }>

                        <Header />
                        <main className="container" style={ { marginTop: '60px' /*fixed header*/ } }>
                            <Switch>
                                <Route exact path="/" render={ props => <Redirect to={ `/surveys` } /> } />
                                { authContext.protected() && <Route path="/login" component={ OAuth2Session } /> }
                                <ProtectedRoute path="/surveys" component={ Surveys } />
                                <ProtectedRoute path="/demo/form/:component?" component={ Demo } />
                                <ProtectedRoute path="/demo/analyse" component={ DemoAnalysis } />
                                <ProtectedRoute path="/demo/manifest" component={ DemoManifest } />
                                <ProtectedRoute path="/analyse/:id/:sessionID" component={ Analysis } />
                                <ProtectedRoute path="/survey/:id/:sessionID?" component={ Survey } />
                                <Route path="*" component={ Page404 } />
                            </Switch>
                            <Dev.Auth />
                        </main>
                        <Footer />

                    </AuthContext.Provider>
                </AppContext.Provider>
            </BrowserRouter>
        );
    }
};

App.propTypes = {};

export default App;
