import React from 'react';
import { BrowserRouter as Router, Route, Switch, Redirect, withRouter } from 'react-router-dom';

// config
import { Configuration } from '../conf/config';

// scaffolding
import HeaderNav from './HeaderNav';
import Footer from './Footer';

// views
import Survey from './Survey';
import Analysis from './Analysis';
import Page404 from './Page404';
import Surveys from './Surveys';
import Demo from './Demo';

const { surveys } = Configuration;
const Navigation = withRouter(HeaderNav);

const App = function(props) {
    return (
        <Router>
            <div>
                <Navigation />
                <main className="container-fluid" style={{ marginTop: '60px' /*fixed header*/ }}>
                    <Switch>
                        <Route exact path="/" render={ props => (surveys.length) ? <Redirect to={ `/survey/${surveys[0]}` } /> : <Surveys /> } />
                        <Route path="/surveys" component={ Surveys } />
                        <Route path="/demo" component={ Demo } />
                        <Route path="/analyse/:id/" component={ Analysis } />
                        <Route path="/survey/:id/:sessionID?" component={ Survey } />
                        <Route path="*" component={ Page404 } />
                    </Switch>
                </main>
                <Footer />
            </div>
        </Router>
    );
};

App.propTypes = {};

export default App;
