import React, { Component, useContext } from 'react';
import { BrowserRouter, Route, Switch, Redirect, withRouter } from 'react-router-dom';

import { DEFAULT_BREAKPOINT, testMediaBreakpoint, matchesBreakpointOrAbove } from '../Media';
import { AppContext, AuthContext, createAuthContext } from '../Context';

// scaffolding
import HeaderNav from './HeaderNav';
import Footer from './Footer';
import ApiAlert from './ApiAlert';

// views
import Survey from './Survey';
import Analysis from './Analysis';
import Page404 from './Page404';
import Surveys from './Surveys';
import OAuth2Login from './OAuth2Login';

import Demo from './demo/Demo';
import DemoAnalysis from './demo/DemoAnalysis';
import DemoManifest from './demo/DemoManifest';

const { PUBLIC_URL, NODE_ENV } = process.env;
const Header = withRouter(HeaderNav);

const ProtectedRoute = function ({ component: Component, ...rest }) {
    const auth = useContext(AuthContext);

    if (!auth.protected) {
        return (
            <Route {...rest} render={ props => <Component {...props} /> } />
        );
    }

    if (auth.unexpired()) {
        // local token will be verified against provider bty <App/> and redirected to login on failure
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

    constructor(props) {
        super(props);

        const authContext = createAuthContext();

        this.state = {
            appContext: {
                breakpoint: DEFAULT_BREAKPOINT,
                matchesBreakpointOrAbove,
                debug: (NODE_ENV === 'development')
            },
            authContext,
            error: null,
        };
    }

    componentDidMount() {
        const { authContext } = this.state;

        window.addEventListener('resize', this.onWindowResize.bind(this));
        window.addEventListener('keydown', this.onWindowKeyDown.bind(this));

        authContext.init()
        .then((user) => {
            authContext.user = user;
            this.setState({
                authContext,
                error: null,
            });
        })
        .catch(error => this.setState({
            error
        }));
    }

    componentWillUnmount() {
        window.removeEventListener('resize', this.onWindowResize);
        window.removeEventListener('keydown', this.onWindowKeyDown);
    }

    onWindowResize() {
        const { appContext } = this.state;

        testMediaBreakpoint((bp) => {
            if(bp !== this.state.breakpoint) {
                appContext.breakpoint = bp;
                this.setState({ appContext });
            }
        });
    }

    onWindowKeyDown(e) {
        const { appContext } = this.state;
        const { debug } = appContext;

        if (!e) {
            return;
        }

        // keys: ALT + ;
        if (e.altKey === true && e.keyCode === 59) {
            console.log('toggle debug mode');
            appContext.debug = !debug;
            this.setState({ appContext });
        }
    }

    render() {
        const { appContext, authContext, error } = this.state;

        return (
            <BrowserRouter basename={ PUBLIC_URL }>
                <AppContext.Provider value={ appContext }>
                    <AuthContext.Provider value={ authContext }>

                        <Header />
                        <main className="container" style={ { marginTop: '60px' /*fixed header*/ } }>

                            { error && <ApiAlert error={ error } /> }

                            <Switch>
                                <Route exact path="/" render={ props => <Redirect to={ `/surveys` } /> } />
                                { authContext.protected && <Route path="/login" component={ OAuth2Login } /> }
                                <ProtectedRoute path="/surveys" component={ Surveys } />
                                <ProtectedRoute path="/demo/form/:component?" component={ Demo } />
                                <ProtectedRoute path="/demo/analyse" component={ DemoAnalysis } />
                                <ProtectedRoute path="/demo/manifest" component={ DemoManifest } />
                                <ProtectedRoute path="/analyse/:id/:session_id?" component={ Analysis } />
                                <ProtectedRoute path="/survey/:id/:session_id?" component={ Survey } />
                                <Route path="*" component={ Page404 } />
                            </Switch>
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
