import React from 'react';
import ReactDOM from 'react-dom';

import 'bootstrap/dist/css/bootstrap.css';
import './style/main.css';
import { Configuration } from './conf/config';

// views
import Navbar from "./components/Navbar";
import FlexibleSurvey from "./components/FlexibleSurvey";
import Page404 from "./components/Page404"
import Surveys from "./components/Surveys"

import { BrowserRouter as Router, Route, Switch, Redirect, withRouter } from 'react-router-dom';

const { surveys, defaultSurveyID } = Configuration;

const Navigation = withRouter(Navbar);

//rendering the routes
//ReactDOM.render(
//
//    document.getElementById("header"));

ReactDOM.render(
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
    </Router>,
    document.getElementById('app')
);
