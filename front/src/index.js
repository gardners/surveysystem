import React from 'react';
import ReactDOM from 'react-dom';
import { BrowserRouter as Router, Route, Switch, Redirect, withRouter } from 'react-router-dom';

// styles
import './styles/index.scss';

// icons
import '@fortawesome/fontawesome-free/css/solid.min.css';
import '@fortawesome/fontawesome-free/css/fontawesome.min.css';

// config
import { Configuration } from './conf/config';

// scaffolding
import HeaderNav from './components/HeaderNav';
import Footer from './components/Footer';

// views
import Survey from './components/Survey';
import Page404 from './components/Page404';
import Surveys from './components/Surveys';
import Demo from './components/Demo';

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

ReactDOM.render(<App />, document.getElementById('root'));
