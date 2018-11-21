import React from 'react';
import ReactDOM from 'react-dom';
import { BrowserRouter as Router, Route, Switch, Redirect, withRouter } from 'react-router-dom';

// styles
import 'bootstrap/dist/css/bootstrap.css';
import './style/main.css';

// config
import { Configuration } from './conf/config';

// views
import Navbar from "./components/Navbar";
import FlexibleSurvey from "./components/FlexibleSurvey";
import Page404 from "./components/Page404"
import Surveys from "./components/Surveys"

const { surveys } = Configuration;
const Navigation = withRouter(Navbar);

const App = function(props) {
    return (
        <Router>
            <div>
                <Navigation />
                <Switch>
                    <Route exact path="/" render={ props => (surveys.length) ? <Redirect to={ `/survey/${surveys[0]}` } /> : <Surveys /> } />
                    <Route path="/surveys" component={ Surveys } />
                    <Route path="/survey/:id/:sessionID?" component={ FlexibleSurvey } />
                    <Route path="*" component={ Page404 } />
                </Switch>
            </div>
        </Router>
    );
};

App.propTypes = {};

ReactDOM.render(<App />, document.getElementById('app'));
