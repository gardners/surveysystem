import React, { Component, useContext } from 'react';
import { BrowserRouter, Route, Switch, Redirect } from 'react-router-dom';

import { DEFAULT_BREAKPOINT, testMediaBreakpoint, matchesBreakpointOrAbove } from '../Media';
import { AppContext } from '../Context';

import { AuthContext, AuthProvider } from './AuthProvider';

// scaffolding
import Header from './Header';
import Main from './Main';
import Footer from './Footer';

// views
import Survey from './Survey';
import Analysis from './Analysis';
import Page404 from './Page404';
import Surveys from './Surveys';
import Login from './Login';

import Demo from './demo/Demo';
import DemoAnalysis from './demo/DemoAnalysis';
import DemoManifest from './demo/DemoManifest';


const { PUBLIC_URL, NODE_ENV } = process.env;


const ProtectedRoute = function ({ component: Component, ...rest }) {
    const auth = useContext(AuthContext);

    if (!auth.is_protected) {
        return (
            <Route {...rest} render={ props => <Component {...props} /> } />
        );
    }

    if (auth.user) {
        return (
            <Route {...rest} render={ props => <Component {...props} /> } />
        );
    }

    return (
        <Redirect to={ {
            pathname: '/login',
            state: { from: rest.location } // remember source
        } } />
    );
};

class App extends Component {

    static contextType = AuthContext;

    constructor(props) {
        super(props);

        this.state = {
            appContext: {
                breakpoint: DEFAULT_BREAKPOINT,
                matchesBreakpointOrAbove,
                debug: (NODE_ENV === 'development')
            },
            error: null,
        };
    }

    componentDidMount() {
        window.addEventListener('resize', this.onWindowResize.bind(this));
        window.addEventListener('keydown', this.onWindowKeyDown.bind(this));
    }

    componentWillUnmount() {
        window.removeEventListener('resize', this.onWindowResize);
        window.removeEventListener('keydown', this.onWindowKeyDown);
    }

    onWindowResize() {
        const appContext = { ...this.state.appContext };

        testMediaBreakpoint((bp) => {
            if(bp !== this.state.breakpoint) {
                appContext.breakpoint = bp;
                this.setState({ appContext });
            }
        });
    }

    onWindowKeyDown(e) {
        if (!e) {
            return;
        }
        const appContext = { ...this.state.appContext };
        const { debug } = appContext;

        // keys: 'ALT' + ';'
        if (e.altKey === true && e.keyCode === 59) {
            console.log('toggle debug mode');
            appContext.debug = !debug;
            this.setState({ appContext });
        }
    }

    render() {
        const { appContext } = this.state;

        return (
            <BrowserRouter basename={ PUBLIC_URL }>
                <AppContext.Provider value={ appContext }>
                    <AuthProvider>
                        <Header />
                        <Main style={ { marginTop: '60px' /*fixed header*/ } }>
                            <Switch>
                                <Route exact path="/" render={ props => <Redirect to={ `/surveys` } /> } />
                                <Route path="/login" component={ Login } />
                                <ProtectedRoute path="/surveys" component={ Surveys } />
                                <ProtectedRoute path="/demo/form/:component?" component={ Demo } />
                                <ProtectedRoute path="/demo/analyse" component={ DemoAnalysis } />
                                <ProtectedRoute path="/demo/manifest" component={ DemoManifest } />
                                <ProtectedRoute path="/analyse/:id/:session_id?" component={ Analysis } />
                                <ProtectedRoute path="/survey/:id/:session_id?" component={ Survey } />
                                <Route path="*" component={ Page404 } />
                            </Switch>
                        </Main>
                        <Footer />
                    </AuthProvider>
                </AppContext.Provider>
            </BrowserRouter>
        );
    }
};

App.propTypes = {};

export default App;
