import React from 'react';
import ReactDOM from 'react-dom';

import 'bootstrap/dist/css/bootstrap.css';
import './style/main.css';
import { Configuration } from './conf/config';

// views
import Navbar from "./components/Navbar";
import FlexibleSurvey from "./components/FlexibleSurvey";
import Page404 from "./components/Page404"
import Welcome from "./components/Welcome"

import { BrowserRouter as Router, Route, Switch, Redirect } from 'react-router-dom';

const { surveys, defaultSurveyID } = Configuration;

//rendering the routes
ReactDOM.render(
    <Navbar />,
    document.getElementById("header"));

ReactDOM.render(
    <Router>
        <Switch>

            <Route exact path="/" render={ props => (surveys.length) ? <Redirect to={ `/survey/${surveys[0]}` } /> : <Welcome /> } />
            <Route path="/survey/:id" component={FlexibleSurvey} />
            <Route path="*" component={Page404} />

        </Switch>
    </Router>,
    document.getElementById('app')
)
